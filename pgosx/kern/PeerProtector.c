/*
    Copyright 2005,2006 Brian Bergstrand

    This program is free software; you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program;
    if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include <mach/mach_types.h>
#include <mach/task.h> // semaphore_create/destroy
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/mbuf.h>
#include <sys/sysctl.h>
#include <sys/kern_control.h>

/* OSMalloc vs. IOMallocAligned

Both of these allocators use the general kalloc map which is only 16MB in size.
The difference is that IOMallocAligned does so only for allocations < PAGE_SIZE.
Anything larger, and it goes to the kernel map. We use this distinction
when allocating large tables so we don't eat a large portion of the
general kalloc map possibly starving ourself or other clients.

*/
#define PP_TABLE_OSMALLOC_MAX (128 * 1024)
#include <libkern/OSMalloc.h>
#include <IOKit/IOLib.h>

#include <libkern/OSAtomic.h>
#include <kern/locks.h>
#include <kern/task.h>

#define PG_MAKE_TABLE_ID // allocate storage
#include "ppmsg.h"
#include "pp_session.h"
#include "pplog.h"
#include "ppk.h"

__private_extern__ lck_grp_t *pp_spin_grp = NULL;
__private_extern__ lck_grp_t *pp_mtx_grp = NULL;

__private_extern__ kern_ctl_ref pp_ctl_ref = (kern_ctl_ref)NULL;
__private_extern__ OSMallocTag pp_malloc_tag = (OSMallocTag)NULL;

__private_extern__ pp_msg_stats_t pp_stats = {0};

__private_extern__ struct pp_iptable *pp_tables = NULL;
__private_extern__ struct pp_iptable *rootPortsBlockTable = NULL;

static errno_t
ctl_connect(kern_ctl_ref r, struct sockaddr_ctl *cap, void **uipp)
{
    pp_session_t cookie;
    errno_t err = pp_session_alloc(&cookie, cap->sc_unit);
    if (!err)
        *uipp = cookie;
    
    return (err);
}

static errno_t
ctl_disconnect(kern_ctl_ref r, u_int32_t unit, void *uip)
{    
    pp_session_free((pp_session_t)uip);
    return (0);
}

static errno_t
ctl_incoming_data(kern_ctl_ref r, u_int32_t unit, void *uip, mbuf_t m, int flags)
{
    pp_msg_t msgp;
    int locked = 0, ref = 0;
    size_t mlen = mbuf_len(m);
    errno_t err = 0;
    if (mlen < sizeof(*msgp)) {
        err = EINVAL;
        goto incoming_exit;
    }
    msgp = (__typeof__(msgp))mbuf_data(m);
    
    if (PP_MSG_MAGIC != msgp->ppm_magic || PP_MSG_VERSION != msgp->ppm_version) {
        if (PP_MSG_MAGIC == msgp->ppm_magic)
            err = ENOTSUP;
        else
            err = EINVAL;
        goto incoming_exit;
    }
    
    pp_session_t cookie = (pp_session_t)uip;
    pp_session_lock(cookie);
    locked = 1;
    
    if (PP_SESSION_EOF == cookie->sequence) {
        err = ESHUTDOWN;
        goto incoming_exit;
    }
    
    if (cookie->sequence != msgp->ppm_sequence) {
        cookie->sequence = PP_SESSION_EOF; // session is bad
        err = EINVAL;
        goto incoming_exit;
    }
    cookie->sequence++;
    
    pp_session_ref(cookie);
    ref = 1;
    pp_session_unlock(cookie);
    locked = 0;
    
    __typeof__(msgp->ppm_size) sz = 0;
    if (mlen < msgp->ppm_size) {
        // Data is on a chain, copy it out
        sz = msgp->ppm_size;
        msgp = (__typeof__(msgp))OSMalloc(sz, pp_malloc_tag);
        if (!msgp) {
            err = ENOMEM;
            goto incoming_exit;
        }
        err = mbuf_copydata(m, 0, sz, msgp);
        if (err) {
            OSFree(msgp, sz, pp_malloc_tag);
            goto incoming_exit;
        }
    }
    
    switch (msgp->ppm_type) {
        case PP_MSG_TYPE_LOAD:
            err = pp_session_load(cookie, msgp);
        break;
        
        case PP_MSG_TYPE_LOG: {
            typeof(cookie->flags) flags = PP_SESSION_LOG;
            (void)OSIncrementAtomic((SInt32*)&pp_logclients);
            if (msgp->ppm_flags & PP_LOG_ALLOWED) {
                flags |= PP_SESSION_LOG_ALLOWED;
                (void)OSIncrementAtomic((SInt32*)&pp_logallowed);
            }
            
            pp_session_lock(cookie);
            cookie->flags |= flags;
            pp_session_unlock(cookie);
            #ifndef DOLOGWAKE
            if (pp_logclients)
                pp_log_wakeup(); // let the log thread know there is now a client
            #endif
        } break;
        
        case PP_MSG_TYPE_REM: {
            if ((err = pp_tables_acquire_mod(NULL)))
                break;
            
            pp_tables_lock();
            
            if (0 == pg_table_id_cmp(PG_TABLE_ID_ALL, pg_msg_cast(pg_msg_rem_t*, msgp)->pgr_tableid)) {
                uint32_t total = 0, addrct;
                while (pp_tables) {
                    addrct = pp_tables->addrct;
                    if (0 != (err = pp_table_free(pp_tables, 1)))
                        break;
                    
                    total += addrct;
                }
                if (0 == err)
                    pp_stats.pps_blocked_addrs = 0;
                else
                    pp_stats.pps_blocked_addrs -= total;
            } else {
                struct pp_iptable *tp = tp = pp_find_table_byid(pg_msg_cast(pg_msg_rem_t*, msgp)->pgr_tableid, 1);
                if (tp)
                    err = pp_table_free(tp, 1);
                else
                    err = ENOENT;
            }
            
            pp_tables_unlock();
            
            pp_tables_relse_mod();
        } break;
        
        case PP_MSG_TYPE_STAT:
            err = pp_session_send_stats(cookie);
        break;
        
        case PP_MSG_TYPE_TBL_INFO: {
            err = pp_session_send_table_info(cookie);
        } break;
        
        default:
            err = ENOTSUP;
            break;
    };
    
    if (msgp != mbuf_data(m) && sz)
        OSFree(msgp, sz, pp_malloc_tag);
    
incoming_exit:
    if (locked)
        pp_session_unlock(cookie);
    if (ref)
        pp_session_relse(cookie);
    
    mbuf_freem(m);
    return (err);
}

static errno_t
ctl_setopt(kern_ctl_ref r, u_int32_t unit, void *uip, int opt, void *datap, size_t len)
{
    return (ENOTSUP);
}

static errno_t
ctl_getopt(kern_ctl_ref r, u_int32_t unit, void *uip, int opt, void *datap, size_t *lenp)
{
    return (ENOTSUP);
}

__private_extern__ lck_mtx_t *pp_tableslck = NULL;
__private_extern__ int32_t pp_tablesusect = 0;
__private_extern__ int32_t pp_tablesloadwanted = 0;
__private_extern__ SInt32 pp_tables_count = 0;

#ifdef PPDBG
static // requires lock to be held
void pp_tables_verify()
{
    struct pp_iptable *cur;
    
    cur = pp_tables;
    PPASSERT(cur ? !cur->prev : 1, "corrupt list!");
    while (cur) {
        PPASSERT(cur->next != cur && cur->prev != cur, "corrupt list!");
        if (cur->next)
            PPASSERT(cur->next->prev == cur, "corrupt forward link!");
        if (cur->prev)
            PPASSERT(cur->prev->next == cur, "corrupt back link!");
        if (0 == (cur->flags & kTableAlwaysPass) && cur->next)
            PPASSERT(0 == (cur->next->flags & kTableAlwaysPass), "corrupt list!");
        cur = cur->next;
        PPASSERT(cur != pp_tables, "corrupt list!");
    }
}
#else
#define pp_tables_verify()
#endif // PPDBG

__private_extern__ int
pp_table_alloc(struct pp_iptable **tblpp, int32_t rangect)
{
    int iokit;
    struct pp_iptable *tbl;
    u_int32_t size = sizeof(*tbl) + (sizeof(tbl->ipranges[0]) * rangect);
    if (size < PP_TABLE_OSMALLOC_MAX) {
        tbl = OSMalloc(size, pp_malloc_tag);
        iokit = 0;
    } else {
        size = round_page_32(size);
        tbl = IOMallocAligned(size, PAGE_SIZE);
        iokit = 1;
    }
    if (!tbl)
        return (ENOMEM);
    
    bzero(tbl, size);
    tbl->size = size;
    tbl->rangect = rangect;
    if (iokit)
        tbl->flags |= kTableIOKitAlloc;
    
    *tblpp = tbl;
    return (0);
}

__private_extern__
struct pp_iptable* pp_find_table_byid(const pg_table_id_t tid, int locked)
{
    if (!locked)
        pp_tables_lock();
    
    struct pp_iptable *tp = pp_tables;
    while (tp) {
        if (0 == pg_table_id_cmp(tid, tp->tableid))
            break;
        tp = tp->next;
    }
    
    pp_tables_verify();
    
    if (!locked)
        pp_tables_unlock();
    
    return (tp);
}

static
void pp_table_log(const pg_table_id_t tableid, u_int32_t type)
{
    pp_log_entry_t *lep;
    typeof(lep->pplg_timestamp) ts;
    mach_timespec_t now;
    clock_get_calendar_nanotime(&now.tv_sec, (u_int32_t*)&now.tv_nsec);
    ts = ((typeof(ts))now.tv_sec) * 1000000ULL;
    ts += ((typeof(ts))now.tv_nsec) / 1000ULL;
    
    pid_t pid = proc_selfpid();
    
    pp_logentries_lock();
    
    if ((lep = pp_log_get_entry(1))) {
        lep->pplg_timestamp = ts;
        pg_table_id_copy(tableid, lep->pplg_tableid);
        lep->pplg_flags |= type;
        lep->pplg_pid = pid;
        
        lep->pplg_name_idx = -1;
    }
#if DOLOGWAKE
    else
        ts = 0;
#endif
    
    pp_logentries_unlock();

#if DOLOGWAKE
    if (ts)
        pp_log_wakeup();
#endif
}

// requires lock
__private_extern__ int
pp_table_auth(struct pp_iptable *tp)
{
    proc_t p = proc_self();
    if (p) {
        int suser = proc_suser(p);
        uid_t me = (proc_ucred(p))->cr_uid;
        proc_rele(p);
        
        if (0 != suser && tp->owner != (typeof(tp->owner))-1) {
            if (tp->owner != me) {
                return (EACCES);
            }
        }
    }
    return (0);
}

__private_extern__ int
pp_table_insert(struct pp_iptable *ntblp)
{
    u_int32_t loadtype;
    pp_tables_lock();
    
    // look for an existing table with the same id
    struct pp_iptable *tp = pp_find_table_byid(ntblp->tableid, 1);
    struct pp_iptable *relse = NULL;
    int autherr;
    if (tp) {
        autherr = pp_table_auth(tp);
        if (autherr) {
            pp_tables_unlock();
            return (autherr);
        }
        
        loadtype = PP_LOG_TBL_RLD;
        relse = tp;
        if ((tp->flags & kTableAlwaysPass) == (ntblp->flags & kTableAlwaysPass)) {
            ntblp->next = tp->next; // unlink the old table
            ntblp->prev = tp->prev;
            if (ntblp->next)
                ntblp->next->prev = ntblp;
            if (ntblp->prev)
                ntblp->prev->next = ntblp;
            if (tp == pp_tables)
                pp_tables = ntblp;
            relse->next = relse->prev = NULL; 
        } else {
            // unlink the old table, and then jump to a new insertion
            if (tp->next)
                tp->next->prev = tp->prev;
            if (tp->prev)
                tp->prev->next = tp->next;
            if (tp == pp_tables)
                pp_tables = tp->next;
            relse->next = relse->prev = NULL; 
            goto table_insert_new;
        }
    } else {
        loadtype = PP_LOG_TBL_LD;

table_insert_new:
        tp = NULL;
        // link the new tbl into the list
        if (pp_tables) {
            PPASSERT(!pp_tables->prev, "corrupt list!");
            
            // Pass tables must always be first in the list
            if (0 == (ntblp->flags & kTableAlwaysPass) && 0 != (pp_tables->flags & kTableAlwaysPass)) {
                tp = pp_tables->next;
                while (tp) {
                    if (0 == (tp->flags & kTableAlwaysPass))
                        break;
                    tp = tp->next;
                }
            }
            
            if (tp) {
                // first block table
                ntblp->next = tp;
                ntblp->prev = tp->prev;
                tp->prev = ntblp;
                if (ntblp->prev)
                    ntblp->prev->next = ntblp;
            } else {
                // no block tables found
                if (0 != (ntblp->flags & kTableAlwaysPass)) {
                    // insert allow tables on head, this makes sure they are checked before all block tables
                    pp_tables->prev = ntblp;
                    ntblp->next = pp_tables;
                    pp_tables = ntblp;
                } else {
                    // block tables go on tail, this makes sure they are checked after all allow tables
                    tp = pp_tables;
                    while (tp->next)
                        tp = tp->next;
                    
                    tp->next = ntblp;
                    ntblp->prev = tp;
                    ntblp->next = NULL;
                }
            }
        } else // empty list
        	pp_tables = ntblp;
    }
    
    u_int32_t ns;
    clock_get_calendar_nanotime(&ntblp->loadtime, &ns);
    
    pp_tables_verify();
    
    if (relse) {
        if (0 == (relse->flags & kTableAlwaysPass)) {
            pp_stats.pps_blocked_addrs -= relse->addrct;
            relse->addrct = 0; // make sure pp_table_free does not count it again.
        }
    } else
        (void)OSIncrementAtomic(&pp_tables_count); // brand new table
    
    if (0 == (ntblp->flags & kTableAlwaysPass))
        pp_stats.pps_blocked_addrs += ntblp->addrct;
    
    pg_table_id_t tableid;
    pg_table_id_copy(ntblp->tableid, tableid);
    
    pp_tables_unlock();
    
    if (relse) {
        PPASSERT(relse != pp_tables && !relse->prev && !relse->next, "corrupt list!");
        autherr = pp_table_free(relse, 0);
        PPASSERT(0 == autherr, "table_free err!");
    }
    
    pp_table_log(tableid, loadtype);
    
    return (0);
}

__private_extern__ int
pp_table_free(struct pp_iptable *tblp, int locked)
{
    if (!locked)
    	pp_tables_lock();
    
    int linked;
    if (tblp == pp_find_table_byid(tblp->tableid, 1)) {
        linked = 1;
        
        int autherr = pp_table_auth(tblp);
        if (autherr) {
            if (!locked)
                pp_tables_unlock();
            return (autherr);
        }
    } else
        linked = 0;
    
    if (tblp->next || tblp->prev) {
        // unlink from list
        if (tblp->prev)
            tblp->prev->next = tblp->next;
        else // we are the head table
            pp_tables = tblp->next;
        if (tblp->next)
            tblp->next->prev = tblp->prev;
        tblp->prev = tblp->next = NULL;
    } else if (tblp == pp_tables) {
        pp_tables = NULL;
    }
    
    pp_tables_verify();
    
    if (linked && 0 == (tblp->flags & kTableAlwaysPass))
        pp_stats.pps_blocked_addrs -= tblp->addrct;
    
    if (!locked)
        pp_tables_unlock();
    
    if (linked) {
    	pp_table_log(tblp->tableid, PP_LOG_TBL_ULD);
        (void)OSDecrementAtomic(&pp_tables_count);
    }
    
    if (0 == (tblp->flags & kTableIOKitAlloc))
        OSFree(tblp, tblp->size, pp_malloc_tag);
    else
        IOFreeAligned(tblp, tblp->size);
    
    return (0);
}

__private_extern__ int pp_tables_acquire_mod(pp_session_t sp)
{
    (void)OSIncrementAtomic((SInt32*)&pp_tablesloadwanted);
        
    register int attempts = 0;
    int err = 0;
    while (pp_tablesusect) { // wait for any filters to complete
        // Handle pathological deadlock case, where the filter is so
        // busy that tablesusect never stays at 0 long enough for us
        // to detect it.
        if (3 == attempts) {
            err = EBUSY;
            pp_stat_increment(table_loadfail);
            break;
        }
        
        pp_stat_increment(table_loadwait);
    #ifdef PPDGB
        if (sp) sp->lckth = NULL;
    #endif
        err = msleep(&pp_tablesusect, (sp ? sp->lck : NULL), PSOCK, __FUNCTION__, NULL);
    #ifdef PPDGB
        if (sp) sp->lckth = current_thread();
    #endif
        ++attempts;
        if (err)
            break;
    }
    return (err);
}

static pp_log_entry_t *logentriesbuf = NULL;
static thread_t logprocessth = NULL;

// ppfilter.c
__private_extern__ int pp_filter_register();
__private_extern__ int pp_filter_deregister();
// pplog.c
__private_extern__ pp_log_entry_t *pp_logentries;
__private_extern__ pp_log_entry_t *pp_logentries_free;

#ifdef PPDBG
#define PPASSERT_BUFSZ 2048
static char pp_assertbuf[PPASSERT_BUFSZ];
static lck_spin_t *pp_assertbuflck = NULL;
#endif

static u_int32_t logentriesbytes;
#define PP_LOGENTRIES_BYTES 20480
#define PP_LOGENTRIES_COUNT (bytes / sizeof(pp_log_entry_t))
kern_return_t PeerProtector_start (kmod_info_t * ki, void * d)
{
    errno_t err = KERN_MEMORY_FAILURE;
    
    pp_malloc_tag = OSMalloc_Tagalloc("PeerGuardian", OSMT_DEFAULT);
    if (!pp_malloc_tag)
        return (err);
    
    pp_spin_grp = lck_grp_alloc_init("PeerGuardian Spin", LCK_GRP_ATTR_NULL);
    if (!pp_spin_grp)
        goto init_free_malloc_tag;
    pp_mtx_grp = lck_grp_alloc_init("PeerGuardian Mutex", LCK_GRP_ATTR_NULL);
    if (!pp_mtx_grp)
        goto init_free_spin_grp;
    
    pp_tableslck = lck_mtx_alloc_init(pp_mtx_grp, LCK_ATTR_NULL);
    if (!pp_tableslck)
        goto init_free_mtx_grp;
    
    pp_sessionslck = lck_mtx_alloc_init(pp_mtx_grp, LCK_ATTR_NULL);
    if (!pp_sessionslck)
        goto init_free_tableslck;
    
    pp_loglck = lck_spin_alloc_init(pp_spin_grp, LCK_ATTR_NULL);
    if (!pp_loglck)
        goto init_free_sessionlck;
    
#ifdef PPDBG
    pp_assertbuflck = lck_spin_alloc_init(pp_spin_grp, LCK_ATTR_NULL);
    if (!pp_assertbuflck)
        goto init_free_loglck;
#endif
    
    u_int64_t memsize;
    size_t bytes = sizeof(memsize);
    if (0 == sysctlbyname("hw.memsize", &memsize, &bytes, NULL, 0)
        && memsize >= 0x40000000 /*1GB*/) {
        if ((bytes = PP_LOGENTRIES_BYTES * (memsize / 0x40000000)) > 0x20000 /*128K*/)
            bytes = 0x20000;
        else if (PP_LOGENTRIES_BYTES == bytes)
            bytes <<= 1;
    } else
        bytes = PP_LOGENTRIES_BYTES;
    
alloc_logentries:
    logentriesbuf = (__typeof__(pp_logentries))IOMallocAligned(bytes, PAGE_SIZE);
    if (!logentriesbuf) {
        if ((bytes >>= 1) > 0)
            goto alloc_logentries;
        else
            goto init_free_assertlck;
    }
    logentriesbytes = (typeof(logentriesbytes))bytes;
    printf("PeerGuardian log entries: %u (%u)\n", PP_LOGENTRIES_COUNT, bytes);
    
    int i;
    bzero(logentriesbuf, bytes);
    pp_logentries_free = logentriesbuf;
    for (i=0; i < PP_LOGENTRIES_COUNT-1; ++i) {
        pp_logentries_free[i].pplg_next = &pp_logentries_free[i+1];
        pp_logentries_free[i+1].pplg_prev = &pp_logentries_free[i];
    }
    pp_logentries_free[0].pplg_prev = NULL;
    pp_logentries_free[i].pplg_next = NULL;
    
    if (KERN_SUCCESS != semaphore_create (kernel_task, &pp_logsignal, SYNC_POLICY_FIFO, 0))
        goto init_free_logentries;
    
    if (pp_filter_register())
        goto init_free_logsignal;
    
    struct kern_ctl_reg reg = {
        PP_CTL_NAME, // ctl_name
        0, // ctl_id
        0, // ctl_unit
        0, // ctl_flags
        0, // ctl_sendsize
        0, // ctl_recvsize
        ctl_connect, // ctl_connect
        ctl_disconnect, // ctl_disconnect
        ctl_incoming_data, // ctl_send
        ctl_setopt, // ctl_setopt
        ctl_getopt,  // ctl_getopt
    };
    
    err = ctl_register(&reg, &pp_ctl_ref);
    if (!err) {
        mach_timespec_t now;
        clock_get_calendar_nanotime(&now.tv_sec, (u_int32_t*)&now.tv_nsec);
        pp_stats.pps_loadtime = ((typeof(pp_stats.pps_loadtime))now.tv_sec) * 1000000ULL;
        pp_stats.pps_loadtime += ((typeof(pp_stats.pps_loadtime))now.tv_nsec) / 1000ULL;
        if ((err = kernel_thread_start(pp_log_processth, NULL, &logprocessth))) {
            // not critical if this fails, just means log clients will sit idle
            printf("PeerGuardian: Failed to create log thread - error: %d.\n", err);
            err = 0;
            logprocessth = NULL;
        }
        return (KERN_SUCCESS);
    }

    pp_filter_deregister();
init_free_logsignal:
    semaphore_destroy(kernel_task, pp_logsignal);
init_free_logentries:
    IOFreeAligned(pp_logentries, logentriesbytes);
init_free_assertlck:
#ifdef PPDBG
    lck_spin_free(pp_assertbuflck, pp_spin_grp);
#endif
init_free_loglck:
    lck_spin_free(pp_loglck, pp_spin_grp);
init_free_sessionlck:
    lck_mtx_free(pp_sessionslck, pp_mtx_grp);
init_free_tableslck:
    lck_mtx_free(pp_tableslck, pp_mtx_grp);
init_free_mtx_grp:
    lck_grp_free(pp_mtx_grp);
init_free_spin_grp:
    lck_grp_free(pp_spin_grp);
init_free_malloc_tag:
    OSMalloc_Tagfree(pp_malloc_tag);

    return (KERN_FAILURE);
}

__private_extern__ int shutdown = 0;

kern_return_t PeerProtector_stop (kmod_info_t * ki, void * d)
{
    if (EBUSY == ctl_deregister(pp_ctl_ref))
        return (KERN_RESOURCE_SHORTAGE);
    
    pp_filter_deregister();
    
    // filters are gone, so there's no need to freeze the tables
    pp_tables_lock();
    while (pp_tables) {
        int PPDBG_ONLY err = pp_table_free(pp_tables, 1);
        PPASSERT(0 == err, "table_free err!");
    }
    pp_tables_unlock();
    
    (void)ExchangeAtomic(1, (unsigned int*)&shutdown);
    if (logprocessth) {
        pp_log_wakeup(); // wake the thread so it can exit
        // wait for the exit
        semaphore_wait(pp_logsignal);
    }
    semaphore_destroy(kernel_task, pp_logsignal);
    
    IOFreeAligned(logentriesbuf, logentriesbytes);
    
    lck_mtx_free(pp_sessionslck, pp_mtx_grp);
    lck_mtx_free(pp_tableslck, pp_mtx_grp);
    lck_spin_free(pp_loglck, pp_spin_grp);
#ifdef PPDBG
    lck_spin_free(pp_assertbuflck, pp_spin_grp);
#endif
    
    lck_grp_free(pp_mtx_grp);
    lck_grp_free(pp_spin_grp);
    
    OSMalloc_Tagfree(pp_malloc_tag);
    
    return (KERN_SUCCESS);
}

#ifdef PPDBG
__private_extern__
void pp_assert_failed(const char *condition, const char *msg, const char *fn, int line)
{
    lck_spin_lock(pp_assertbuflck);
    snprintf(pp_assertbuf, PPASSERT_BUFSZ-1, "Assert '%s' failed (%s:%u)! %s", condition, fn, line, msg);
    pp_assertbuf[PPASSERT_BUFSZ-1] = 0;
    PE_enter_debugger(pp_assertbuf);
    lck_spin_unlock(pp_assertbuflck);
}
#endif
