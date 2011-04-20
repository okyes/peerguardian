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

#include "ppk.h"
#include "pplog.h"
#include "pp_session.h"

__private_extern__ lck_spin_t     *pp_loglck = NULL;
__private_extern__ pp_log_entry_t *pp_logentries_free = NULL; // head of free list
__private_extern__ pp_log_entry_t *pp_logentries = NULL; // head of used list
static int32_t         pp_logentriesused = 0;
__private_extern__ int32_t         pp_logclients = 0;
__private_extern__ int32_t         pp_logallowed = 0;

__private_extern__ semaphore_t     pp_logsignal = NULL;

static pp_log_entry_t *entries_tail = NULL; // tail of used list

#ifdef PPDBG
static // requires lock to be held
void pp_logs_verify()
{
    pp_log_entry_t* cur;
    
    cur = pp_logentries_free;
    PPASSERT(cur ? !cur->pplg_prev : 1, "corrupt list!");
    while (cur) {
        PPASSERT(cur->pplg_next != cur && cur->pplg_prev != cur, "corrupt list!");
        if (cur->pplg_next)
            PPASSERT(cur->pplg_next->pplg_prev == cur, "corrupt forward link!");
        if (cur->pplg_prev)
            PPASSERT(cur->pplg_prev->pplg_next == cur, "corrupt back link!");
        PPASSERT(0 == cur->pplg_flags, "corrupted list!");
        cur = cur->pplg_next;
        PPASSERT(cur != pp_logentries_free, "corrupt list!");
    }
    
    if (entries_tail) {
        PPASSERT(!entries_tail->pplg_next, "corrupt tail!");
        if (entries_tail != pp_logentries)
            PPASSERT(entries_tail->pplg_prev, "corrupt tail!");
        else
            PPASSERT(1 == pp_logentriesused, "corrupt list!");
    }
    cur = pp_logentries;
    PPASSERT(cur ? !cur->pplg_prev : 1, "corrupt list!");
    while (cur) {
        PPASSERT(cur->pplg_next != cur && cur->pplg_prev != cur, "corrupt list!");
        if (cur->pplg_next)
            PPASSERT(cur->pplg_next->pplg_prev == cur, "corrupt forward link!");
        if (cur->pplg_prev)
            PPASSERT(cur->pplg_prev->pplg_next == cur, "corrupt back link!");
        PPASSERT((cur->pplg_flags & PP_LOG_INUSE), "corrupted list!");
        PPASSERT(cur->pplg_prev != entries_tail, "corrupt list!");
        cur = cur->pplg_next;
        PPASSERT(cur != pp_logentries, "corrupt list!");
    }
}
#else
#define pp_logs_verify()
#endif // PPDBG

__private_extern__
pp_log_entry_t* pp_log_get_entry(int lockheld)
{
    pp_log_entry_t *ep = NULL;
    
    if (!lockheld)
        pp_logentries_lock();
    
    // remove the free list head
    if (pp_logentries_free) {
        ep = pp_logentries_free;
        if ((pp_logentries_free = ep->pplg_next))
            pp_logentries_free->pplg_prev = NULL;
        PPASSERT(ep ? !ep->pplg_prev : 1, "corrupt list!");
    }
    
    // insert on the used list tail as the used list
    // grows from oldest to newest
    if (ep) {
        PPASSERT(0 == ep->pplg_flags, "corrupted list!");
        ep->pplg_next = NULL;
        if (entries_tail) {
            PPASSERT(!entries_tail->pplg_next, "corrupted list!");
            entries_tail->pplg_next = ep;
            ep->pplg_prev = entries_tail;
        } else {
            PPASSERT(!pp_logentries && !ep->pplg_prev, "corrupted list!");
            pp_logentries = ep;
        }
        entries_tail = ep;
        PPASSERT(!entries_tail->pplg_next, "corrupt list!");
        pp_logentriesused++;
        
        ep->pplg_flags = PP_LOG_INUSE;
    } else {
        if (pp_logentries) {
            // recycle the oldest entry
            PPASSERT(pp_logentries && pp_logentries->pplg_next && !pp_logentries->pplg_prev
                && entries_tail && entries_tail != pp_logentries, "corrupted list!");
            ep = pp_logentries;
            pp_logentries = ep->pplg_next;
            PPASSERT(pp_logentries && pp_logentries->pplg_prev == ep, "corrupt list");
            pp_logentries->pplg_prev = NULL;
            
            PPASSERT(!entries_tail->pplg_next, "corrupted list!");
            entries_tail->pplg_next = ep;
            ep->pplg_next = NULL;
            ep->pplg_prev = entries_tail;
            entries_tail = ep;
            
            ep->pplg_flags = PP_LOG_INUSE;
        }
        
        pp_stat_increment(droppedlogs);
    }
    
    pp_logs_verify();
    if (!lockheld)
        pp_logentries_unlock();
    
    return (ep);
}

// lock must be held
static
void free_entry(pp_log_entry_t *ep)
{
    ep->pplg_next = ep->pplg_prev = NULL;
    ep->pplg_flags = 0;
    
    // Add back to free list
    if (pp_logentries_free) {
        ep->pplg_next = pp_logentries_free;
        pp_logentries_free->pplg_prev = ep;
    }
    pp_logentries_free = ep;
}

#ifdef notyet
__private_extern__
void pp_log_relse_entry(pp_log_entry_t *ep, int lockheld)
{
    if (!lockheld)
    	pp_logentries_lock();
    
    PPASSERT(ep->pplg_flags & PP_LOG_INUSE, "log entry not on used list!");
    
    // Remove from used list
    if (entries_tail == ep) {
        PPASSERT(ep->pplg_prev && !ep->pplg_next, "corrupt list!");
        entries_tail = ep->pplg_prev;
    }
    if (ep->pplg_prev)
        ep->pplg_prev->pplg_next = ep->pplg_next;
    else { // we are the head entry
        PPASSERT(pp_logentries == ep && !ep->pplg_prev, "corrupt list head");
        pp_logentries = ep->pplg_next;
    }
    if (ep->pplg_next)
        ep->pplg_next->pplg_prev = ep->pplg_prev;
    pp_logentriesused--;
    
    free_entry(ep); // Add back to free list
    
    if (!lockheld)
    	pp_logentries_unlock();
}
#endif

__private_extern__ int shutdown; // PeerProtector.c

static const mach_timespec_t pp_logwait = {0,500000}; // .5 seconds
__private_extern__
void pp_log_processth(void *arg, wait_result_t wait)
{
    int err;
    pp_log_entry_t *ep, *etailp;
    int32_t entriestaken;
        
    do {        
        if (!pp_logclients) {
            semaphore_wait(pp_logsignal);
            continue;
        }
        
        ep = NULL;
        
        pp_logentries_lock();
        
        if (pp_logentries) {
            ep = pp_logentries;
            etailp = entries_tail;
            pp_logentries = entries_tail = NULL; // we now "own" the entries
            entriestaken = pp_logentriesused;
            pp_logentriesused = 0;
        }
        
        pp_logentries_unlock();
        
        if (!ep) {
            #ifndef DOLOGWAKE
            semaphore_timedwait(pp_logsignal, pp_logwait);
            #else
            semaphore_wait(pp_logsignal);
            #endif
            continue;
        }
        PPASSERT(etailp, "corrupt list!");
        
        #define MAX_ENTRIES ((PP_MSG_RECOMMENDED_MAX - sizeof(struct pp_msg)) / sizeof(*ep))
        typeof(entriestaken) count = entriestaken <= MAX_ENTRIES ? entriestaken : MAX_ENTRIES;
        
        u_int32_t totalSize = sizeof(struct pp_msg) + (count * sizeof(*ep));
        pp_msg_t msgp = OSMalloc(totalSize, pp_malloc_tag);
        if (!msgp) {
            // put the entries back on the list in the hopes the shortage is transient
            pp_logentries_lock();
            
            if (!pp_logentries) {
                PPASSERT(!entries_tail, "corrupt list!");
                pp_logentries = ep;
                entries_tail = etailp;
                PPASSERT(!pp_logentries->pplg_prev && !entries_tail->pplg_next, "corrupt list");
            } else {
                // list is ordered from oldest to newest, therefore
                // our "old" entries have to be first 
                PPASSERT(!pp_logentries->pplg_prev && !entries_tail->pplg_next, "corrupt list!");
                pp_logentries->pplg_prev = etailp;
                etailp->pplg_next = pp_logentries;
                pp_logentries = ep;
            }
            pp_logentriesused += entriestaken;
            
            pp_logs_verify();
            pp_logentries_unlock();
            
            semaphore_timedwait(pp_logsignal, pp_logwait);
            continue;
        }
        bzero(msgp, totalSize);
        msgp->ppm_magic = PP_MSG_MAGIC;
        msgp->ppm_version = PP_MSG_VERSION;
        msgp->ppm_type = PP_MSG_TYPE_LOG;
        
        pp_log_entry_t *nep = ep, *oep;
        u_int32_t msgSize = totalSize;
        do {
            entriestaken -= count;
            
            int i;
            ep = nep;
            oep = (pp_log_entry_t*)msgp->ppm_data;
            for(i=0; i < count; ++i) {
                PPASSERT(nep, "corrupt list!");
                oep[i] = *nep;
                // 0 the copied kernel ptrs
                oep[i].pplg_next = oep[i].pplg_prev = NULL;
                nep = nep->pplg_next;
            }
            // nep is now the head of the next batch to process (or nil)
            PPASSERT(nep ? nep->pplg_prev->pplg_timestamp == oep[i-1].pplg_timestamp : 1, "corrupt batch!");
            
            // Now that the data is copied, make the entries availble for reuse.
            pp_logentries_lock();
            
            while (ep && ep != nep) {
                oep = ep;
                ep = oep->pplg_next;
                free_entry(oep);
            }
            
            pp_logs_verify();
            pp_logentries_unlock();
            
            ep = NULL;
            oep = NULL;
            
            // send the entries to any willing session
            #define MAX_SESSIONS 20
            pp_session_t cookies[MAX_SESSIONS+1] = {NULL};
            i = 0;
            pp_sessions_lock();
            pp_session_t cookie = pp_sessions;
            while(cookie && i < MAX_SESSIONS) {
                if (cookie->flags & PP_SESSION_LOG) {
                    pp_session_ref(cookie);
                    cookies[i] = cookie;
                    ++i;
                }
                cookie = cookie->next;
            }
            pp_sessions_unlock();
            
            msgp->ppm_size = msgSize;
            msgp->ppm_logcount = count;
            
            for (i = 0; (cookie = cookies[i]); ++i) {
                int retry = 2;
                pp_session_lock(cookie);
            cookie_send_msg:
                if (cookie->kctlunit != (typeof(cookie->kctlunit))-1) {
                    size_t recvsz = 0;
                    if (0 == ctl_getenqueuespace(pp_ctl_ref, cookie->kctlunit, &recvsz)
                        && recvsz < msgp->ppm_size && retry > 0) {
                        // msg q is full
                        // Sleep one quantum giving the client a chance to wake
                        struct timespec ts = {0, 10000000};
                        (void)msleep(NULL, cookie->lck, PSOCK, __FUNCTION__, &ts);
                        retry--;
                        goto cookie_send_msg;
                    }
                    msgp->ppm_sequence = cookie->sequence;
                    err = ctl_enqueuedata(pp_ctl_ref, cookie->kctlunit, msgp, msgp->ppm_size, 0);
                    if (!err)
                        cookie->sequence++;
                    else
                        printf("PeerGuardian: failed to send log msg to client: %u\n",
                            cookie->kctlunit);
                }
                pp_session_unlock(cookie);
                pp_session_relse(cookie);
            }
            
            count = entriestaken <= MAX_ENTRIES ? entriestaken : MAX_ENTRIES;
            msgSize = sizeof(struct pp_msg) + (count * sizeof(*ep));
        } while(nep && entriestaken > 0);
        
        OSFree(msgp, totalSize, pp_malloc_tag);
    } while(!shutdown);
    
    // signal the sem, so unload can oocur
    pp_log_wakeup();
    
    return;
}
