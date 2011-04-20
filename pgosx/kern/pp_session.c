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
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <libkern/OSMalloc.h>
#include <libkern/OSAtomic.h>
#include <kern/locks.h>
#include <kern/task.h> // kernel_task
#include <mach/semaphore.h>
#include <mach/task.h> // semaphore_create/destroy

#include "ppk.h"
#include "ppmsg.h"
#include "pp_session.h"

__private_extern__ pp_session_t pp_sessions = NULL;
__private_extern__ lck_mtx_t   *pp_sessionslck = NULL;

__private_extern__ int32_t pp_logallowed; // pplog.c
__private_extern__ int32_t pp_logclients;

#ifdef PPDBG
static // requires lock to be held
void pp_sessions_verify()
{
    pp_session_t cur;
    
    cur = pp_sessions;
    PPASSERT(cur ? !cur->prev : 1, "corrupt list!");
    while (cur) {
        PPASSERT(cur->next != cur && cur->prev != cur, "corrupt list!");
        if (cur->next)
            PPASSERT(cur->next->prev == cur, "corrupt forward link!");
        if (cur->prev)
            PPASSERT(cur->prev->next == cur, "corrupt back link!");
        cur = cur->next;
        PPASSERT(cur != pp_sessions, "corrupt list!");
    }
}
#else
#define pp_sessions_verify()
#endif // PPDBG

__private_extern__ errno_t
pp_session_alloc(pp_session_t *spp, u_int32_t kctlunit)
{
    pp_session_t sp = (pp_session_t)OSMalloc(sizeof(*sp), pp_malloc_tag);
    if (!sp)
        return (ENOMEM);
    
    bzero(sp, sizeof(*sp));
    sp->size = sizeof(*sp);
    sp->kctlunit = kctlunit;
    proc_t p = proc_self();
    if (p) {
        sp->owner = (proc_ucred(p))->cr_uid;
        proc_rele(p);
    } else {
        sp->owner = (typeof(sp->owner))-1;
    }
    
    if (!(sp->lck = lck_mtx_alloc_init(pp_mtx_grp, LCK_ATTR_NULL))) {
        OSFree(sp, sizeof(*sp), pp_malloc_tag);
        return (ENOMEM);
    }
    
    // add it to the list
    pp_sessions_lock();
    
    if (pp_sessions) {
        PPASSERT(!pp_sessions->prev, "corrupt list");
        sp->next = pp_sessions;
        pp_sessions->prev = sp;
    }
    pp_sessions = sp;
    
    pp_sessions_verify();
    pp_sessions_unlock();
    
    pp_stat_increment(userclients);
    *spp = sp;
    return (0);
}

static inline void
pp_session_free_timer(pp_session_t sp)
{
    thread_call_free (sp->timer);
    pp_session_relse(sp); // for allocation increment
    sp->timer = NULL;
}

__private_extern__ void
pp_session_free(pp_session_t sp)
{
    (void)ExchangeAtomic((unsigned int)-1, (unsigned int*)&sp->kctlunit);
    
    // Remove it from the list
    pp_sessions_lock();
    
    if (sp->prev)
        sp->prev->next = sp->next;
    else // we are the head session
        pp_sessions = sp->next;
    if (sp->next)
        sp->next->prev = sp->prev;
    
    pp_sessions_verify();
    pp_sessions_unlock();
    
    pp_session_lock(sp);
    
    if (sp->flags & PP_SESSION_LOG)
        (void)OSDecrementAtomic((SInt32*)&pp_logclients);
    
    if (sp->flags & PP_SESSION_LOG_ALLOWED)
        (void)OSDecrementAtomic((SInt32*)&pp_logallowed);
    
    if (sp->timer) {
        thread_call_cancel(sp->timer);
        pp_session_free_timer(sp);
    }
    
    if (sp->stat_timer) {
        thread_call_cancel(sp->stat_timer);
        thread_call_free (sp->stat_timer);
        pp_session_relse(sp); // for allocation increment
        sp->stat_timer = NULL;
    }
    
    struct pp_iptable *tp = sp->table;
    sp->table = NULL;
    sp->rangect = 0;
    pp_session_unlock(sp);
    
    pp_stat_decrement(userclients);
    
    if (tp) {
        PPASSERT(!tp->prev && !tp->next, "corrupt list!");
        int PPDBG_ONLY err = pp_table_free(tp, 0);
        PPASSERT(0 == err, "table_free err!");
    }
    
    struct timespec ts = {0, 1000000}; // 1ms
    while(sp->refct > 0)
        msleep(NULL, NULL, PSOCK, __FUNCTION__, &ts);
    
    lck_mtx_free(sp->lck, pp_mtx_grp);
    
    OSFree(sp, sp->size, pp_malloc_tag);
}

static void
pp_session_load_expired (thread_call_param_t session, __unused thread_call_param_t unused)
{
    pp_session_t sp = (pp_session_t)session;
    
    pp_session_ref(sp);
    
    pp_session_lock(sp);
    
    struct pp_iptable *tp = sp->table;
    sp->table = NULL;
    sp->rangect = 0;
    sp->flags |= PP_SESSION_LOAD_TMOUT;
    
    pp_session_unlock(sp);
    
    if (tp) {
        PPASSERT(!tp->prev && !tp->next, "corrupt list!");
        int PPDBG_ONLY err = pp_table_free(tp, 0);
        PPASSERT(0 == err, "table_free err!");
    }
    
    pp_session_relse(sp);
}

__private_extern__ struct pp_iptable *rootPortsBlockTable; // PeerProtector.c

__private_extern__ errno_t
pp_session_load(pp_session_t sp, pp_msg_t msgp)
{
    pp_msg_load_t *mlp = (pp_msg_load_t*)msgp->ppm_data;
    
    u_int32_t expectedSize = sizeof(*msgp) + sizeof(*mlp);
    if (msgp->ppm_size < expectedSize || 0 != (mlp->ppld_flags & ~PP_LOAD_VALID_FLGS)
        || PP_LOAD_VERSION != mlp->ppld_version)
        return (EINVAL);
    if (!(mlp->ppld_flags & PP_LOAD_BEGIN)) { 
        expectedSize += sizeof(mlp->ppld_rngs[0]) * mlp->ppld_count;
    }
    if (msgp->ppm_size != expectedSize)
        return (EINVAL);
    
    errno_t err = 0, merr = 0;
    if (mlp->ppld_flags & PP_LOAD_END) {
        pp_session_lock(sp);
        
        if (sp->timer) {
            thread_call_cancel(sp->timer);
            pp_session_free_timer(sp);
        }
        if (!sp->table) {
            sp->rangect = 0;
            err = EINVAL;
        }
        // table load is done

        // announce we want to load
        merr = pp_tables_acquire_mod(sp);
        
        // tables are now safe to modify
        
        // the table could have disappeared if we had to wait
        // for pp_tablesusect to hit 0
        struct pp_iptable *tp = sp->table;
        sp->table = NULL;
        typeof(sp->rangect) rangect = sp->rangect;
        sp->rangect = 0;
        
        pp_session_unlock(sp);
        
        if (!err && !merr) {
            if (tp) {                
                if (rangect == tp->rangect) {
                    tp->owner = sp->owner;
                    if (0 == (mlp->ppld_flags & PP_LOAD_PORT_RULE)) {
                        if (0 == (err = pp_table_insert(tp)))
                            tp = NULL;
                    } else {
                        pp_tables_lock();
                        
                        err = rootPortsBlockTable ? pp_table_auth(rootPortsBlockTable) : 0;
                        if (0 == err) {                        
                            struct pp_iptable *ptp = rootPortsBlockTable;
                            rootPortsBlockTable = tp;
                            rootPortsBlockTable->addrct = 0;
                            
                            u_int32_t ns;
                            clock_get_calendar_nanotime(&rootPortsBlockTable->loadtime, &ns);
                            pp_tables_unlock();
                            // make sure the old table is freed
                            tp = ptp;
                        }
                    }
                } else {
                    PPASSERT(!tp->prev && !tp->next, "corrupt list!");
                    err = EINVAL;
                }
            } else
                err = EINVAL;
        }
        
        pp_tables_relse_mod();
        
        if (tp) {
            int PPDBG_ONLY ferr = pp_table_free(tp, 0);
            PPASSERT(0 == ferr, "table_free err!");
        }
        
        if (merr)
            err = merr;
        return (err);
    } // (ppld_flags & PP_LOAD_END)
    
    pp_session_lock(sp);
    
    if (sp->timer)
        thread_call_cancel(sp->timer);
    
    if (mlp->ppld_flags & PP_LOAD_BEGIN) {    
        if (!sp->table && mlp->ppld_count && 0 != pg_table_id_cmp(PP_TABLE_ID_ALL, mlp->ppld_tableid)) {
            if (!(err = pp_table_alloc(&sp->table, mlp->ppld_count))) {
                pg_table_id_copy(mlp->ppld_tableid, sp->table->tableid);
                if (mlp->ppld_flags & PP_LOAD_BLOCK_STD)
                    sp->table->flags |= kTableBlockStdPorts;
                if (mlp->ppld_flags & PP_LOAD_ALWYS_PASS)
                    sp->table->flags |= kTableAlwaysPass;
                sp->table->addrct = mlp->ppld_addrct;
            }
        } else {
            err = EINVAL;
        }
    } else do {
        if (!sp->table || 0 != pg_table_id_cmp(sp->table->tableid, mlp->ppld_tableid)) {
            if (sp->flags & PP_SESSION_LOAD_TMOUT)
                err = ETIMEDOUT;
            else
            	err = EINVAL;
            break;
        }
        
        // load the ranges into the table
        if ((sp->rangect + mlp->ppld_count) <= sp->table->rangect &&
            (sp->rangect ? (sp->table->ipranges[sp->rangect-1].p2_ipstart < mlp->ppld_rngs[0].p2_ipstart) : 1) &&
            (sp->table->ipranges[sp->rangect].p2_ipstart == 0 && sp->table->ipranges[sp->rangect].p2_ipend == 0)
            ) {
            bcopy(mlp->ppld_rngs, &sp->table->ipranges[sp->rangect],
                sizeof(mlp->ppld_rngs[0]) * mlp->ppld_count);
            sp->rangect += mlp->ppld_count;
        } else {
            err = EINVAL;
        }
    } while(0); // PP_LOAD_BEGIN
    
    if (!err) {
        if (!sp->timer) {
            sp->timer = thread_call_allocate(pp_session_load_expired, (thread_call_param_t)sp);
            if (sp->timer)
                pp_session_ref(sp);
            else
                err = ENOMEM;
        }
    }
    
    sp->flags &= ~PP_SESSION_LOAD_TMOUT;
    if (sp->timer) {
        u_int64_t deadline;
        // 8 seconds for the client to send us another load
        clock_interval_to_deadline(8000000UL, NSEC_PER_USEC, &deadline);
        thread_call_enter_delayed(sp->timer, deadline);
    }
    
    pp_session_unlock(sp);
    return (err);
}

static void
pp_session_send_msg_cb(thread_call_param_t session, thread_call_param_t msg)
{
    pp_session_t sp = (pp_session_t)session;
    pp_msg_t msgp = (pp_msg_t)msg;
    PPASSERT(msgp, "invalid msg!");
    
    pp_session_lock(sp);
    if ((typeof(sp->kctlunit))-1 != sp->kctlunit) {
        msgp->ppm_sequence = sp->sequence;
        int err = ctl_enqueuedata(pp_ctl_ref, sp->kctlunit, msgp, msgp->ppm_size, 0);
        if (!err)
            sp->sequence++;
    }
    sp->flags &= ~PP_SESSION_SND_PENDING;
    pp_session_unlock(sp);
    
    OSFree(msgp, msgp->ppm_size, pp_malloc_tag);
}

static errno_t
pp_session_send_msg(pp_session_t sp, pp_msg_t msgp)
{
    int err = 0;
    pp_session_lock(sp);
    
    if (!sp->stat_timer) {
send_msg_alloc_timer:
        sp->stat_timer = thread_call_allocate(pp_session_send_msg_cb, (thread_call_param_t)sp);
        if (sp->stat_timer)
            pp_session_ref(sp);
    } else {
        while (0 != (sp->flags & PP_SESSION_SND_PENDING)) {
            struct timespec ts = {0, 10000000};
            if ((err = msleep(NULL, sp->lck, PSOCK, __FUNCTION__, &ts)))
                goto send_msg_exit_with_lock;
        }
        thread_call_cancel(sp->stat_timer);
        thread_call_free (sp->stat_timer);
        pp_session_relse(sp); // for allocation increment
        sp->stat_timer = NULL;
        goto send_msg_alloc_timer;
    }
    
    if (sp->stat_timer) {
        sp->flags |= PP_SESSION_SND_PENDING;
        u_int64_t deadline;
        clock_interval_to_deadline(1UL, NSEC_PER_USEC, &deadline);
        thread_call_enter1_delayed(sp->stat_timer, msgp, deadline);
    } else {
        err = ENOMEM;
    }
    
send_msg_exit_with_lock:
    pp_session_unlock(sp);
    return (err);
}

__private_extern__ u_int32_t sLastConnect; // ppfilter.c

__private_extern__ errno_t
pp_session_send_stats(pp_session_t sp)
{
    errno_t err = 0;
    
    pp_msg_t msgp;
    size_t size = sizeof(*msgp) + sizeof(pp_msg_stats_t);
    msgp  = OSMalloc(size, pp_malloc_tag);
    if (!msgp)
        return (ENOMEM);
    
    bzero(msgp, size);
    msgp->ppm_magic = PP_MSG_MAGIC;
    msgp->ppm_version = PP_MSG_VERSION;
    msgp->ppm_type = PP_MSG_TYPE_STAT;
    msgp->ppm_size = size;
    
    // Make sure the conn stats are up-to-date
    mach_timespec_t now;
    clock_get_calendar_nanotime(&now.tv_sec, (u_int32_t*)&now.tv_nsec);
    u_int32_t lastConnect = sLastConnect;
    if (now.tv_sec >= (lastConnect+1)) {
        (void)ExchangeAtomic(now.tv_sec, &sLastConnect);
        (void)ExchangeAtomic(0, &pp_stats.pps_conns_per_sec);
        (void)ExchangeAtomic(0, &pp_stats.pps_blcks_per_sec);
    }
    
    pp_msg_stats_t *stp = (typeof(stp))msgp->ppm_data;
    *stp = pp_stats;
    stp->pps_version = PP_STATS_VERSION;
    
    if ((err = pp_session_send_msg(sp, msgp)))
        OSFree(msgp, msgp->ppm_size, pp_malloc_tag);

    return (err);
}

// PeerProtector.c
__private_extern__ struct pp_iptable *pp_tables;
__private_extern__ SInt32 pp_tables_count;

__private_extern__ errno_t
pp_session_send_table_info(pp_session_t sp)
{
    int max, count = pp_tables_count;
    if (count <= 0)
        return (ENOENT);
    
    pp_msg_t msgp;
    pp_msg_table_info_t *tip;
    size_t size = sizeof(*msgp) + ((count+1 /*port table*/) * sizeof(*tip));
    if (size > PP_MSG_RECOMMENDED_MAX) {
        // XXX Only support 1 msg right now -- this maxes out at about 100 lists
        size = PP_MSG_RECOMMENDED_MAX;
        max = count = (size - sizeof(*msgp)) / sizeof(*tip);
    } else
        max = (size - sizeof(*msgp)) / sizeof(*tip);
    
    if (!(msgp = (typeof(msgp))OSMalloc(size, pp_malloc_tag)))
        return (ENOMEM);
    
    bzero(msgp, size);
    msgp->ppm_magic = PP_MSG_MAGIC;
    msgp->ppm_version = PP_MSG_VERSION;
    msgp->ppm_type = PP_MSG_TYPE_TBL_INFO;
    msgp->ppm_size = size;
    
    tip = (typeof(tip))msgp->ppm_data;
    
    // XXX -- We do not have table mod access, do not modify
    pp_tables_lock();
    
    // Count could have changed if we had to wait on the lock
    if (pp_tables_count < count)
        count = pp_tables_count;
    
    // Safe to access tables read-only now
    int i;
    const struct pp_iptable *tp = pp_tables;
    for (i=0; i < count && tp; ++i) {
        tip[i].size = tp->size;
        pg_table_id_copy(tp->tableid, tip[i].tableid);
        tip[i].rangect = tp->rangect;
        tip[i].addrct = tp->addrct;
        tip[i].loadtime = (typeof(tip[0].loadtime))tp->loadtime;
        if (0 != (tp->flags & kTableBlockStdPorts))
            tip[i].flags |= PP_LOAD_BLOCK_STD;
        if (0 != (tp->flags & kTableAlwaysPass))
            tip[i].flags |= PP_LOAD_ALWYS_PASS;
        
        tp = tp->next;
    }
    
    if (count != i)
        count = i;
    
    if (count < max && (tp = rootPortsBlockTable)) {
        tip[count].size = tp->size;
        pg_table_id_copy(tp->tableid, tip[count].tableid);
        tip[count].rangect = tp->rangect;
        tip[count].addrct = tp->addrct;
        tip[count].loadtime = (typeof(tip[0].loadtime))tp->loadtime;
        tip[count].flags = 0;
        count++;
    }
    
    pp_tables_unlock();
    
    int err;
    if (count > 0) {
        msgp->ppm_count = count;
        err = pp_session_send_msg(sp, msgp);
    } else
        err = ENOENT;
    if (err)
        OSFree(msgp, msgp->ppm_size, pp_malloc_tag);
    
    return (err);
}

#if 0
// This is disabled becaue the kernel has a special verison of sysctlbyname that
// only exports a limited set of variables, and PROCARGS is not one of them. Stupid.

// The kernel prevents non-root processes from using {CTL_KERN, KERN_PROCARGS} to get
// info on other users processes, so we need to act as  proxy.
typedef struct pp_procargs {
    user_addr_t ppp_buf;
    user_addr_t ppp_bufsize;
    int32_t     ppp_pid;
} pp_procargs_t;

struct procargs {
    thread_call_t thread;
    semaphore_t sem;
    int err;
};

static void
pp_session_proc_args_cb(thread_call_param_t tmsg1, thread_call_param_t args2)
{
    struct procargs *tmsg = (struct procargs*)tmsg1;
    pp_procargs_t *args = (pp_procargs_t*)args2;
    
    size_t sz;
    if (0 == (tmsg->err = copyin(args->ppp_bufsize, &sz, sizeof(sz)))) {
        tmsg->err = sysctlbyname("kern.procargs2", CAST_DOWN(void*, args->ppp_buf),
            &sz, NULL, 0);
        if (0 == tmsg->err)
            (void)copyout(&sz, args->ppp_bufsize, sizeof(sz));
    }
    semaphore_signal(tmsg->sem);
}

__private_extern__ errno_t
pp_session_proc_args(pp_session_t sp, pp_msg_t msgp)
{
    pp_procargs_t *args = (pp_procargs_t*)msgp->ppm_data;
    
    // sysctlbyname does not support user_addr_t
    proc_t me = proc_self();
    if (!me)
        return (EINVAL);
    
    int is64 = proc_is64bit(me);
    proc_rele(me);
    if (msgp->ppm_size != (sizeof(*msgp) + sizeof(*args)) || is64)
        return (EINVAL);
    
    // We have to do the sysctl from another thread so current_proc() is the kernel,
    // otherwise we'll get denied just like in userland.
    struct procargs tmsg;
    if (KERN_SUCCESS != semaphore_create(kernel_task, &tmsg.sem, SYNC_POLICY_FIFO, 0))
        return (ENOMEM);
    
    if (!(tmsg.thread = thread_call_allocate(pp_session_proc_args_cb, (thread_call_param_t)&tmsg))) {
        semaphore_destroy(kernel_task, tmsg.sem);
        return (ENOMEM);
    }
    
    tmsg.err = 0;
    kern_return_t kret;
    thread_call_enter1(tmsg.thread, (thread_call_param_t)args);
    if (KERN_SUCCESS != (kret = semaphore_wait(tmsg.sem) && 0 == tmsg.err))
        tmsg.err = KERN_TERMINATED == kret ? EINTR : EINVAL;
    
    thread_call_cancel(tmsg.thread);
    thread_call_free(tmsg.thread);
    semaphore_destroy(kernel_task, tmsg.sem);
    return (tmsg.err);
}
#endif
