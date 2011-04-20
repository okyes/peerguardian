/*
    Copyright 2005 Brian Bergstrand

    This program is free software; you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program;
    if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include <kern/thread_call.h>

typedef struct pp_session {
    struct pp_session *next;
    struct pp_session *prev;
    lck_mtx_t *lck;
#ifdef PPDGB
    thread_t *lckth;
#endif
    thread_call_t timer; // load timer
    thread_call_t stat_timer;
    u_int32_t kctlunit;
    u_int32_t sequence;
    u_int32_t size;
    u_int32_t flags;
    uid_t owner;
    int32_t refct;
    
    struct pp_iptable *table; // table that is being loaded
    int32_t rangect; // current # of ranges loaded
} *pp_session_t;

#define pp_session_ref(s) (void)OSIncrementAtomic((SInt32*)&(s)->refct)
#define pp_session_relse(s) (void)OSDecrementAtomic((SInt32*)&(s)->refct)

#ifndef PPDGB
#define pp_session_lock(s) lck_mtx_lock((s)->lck)
#define pp_session_unlock(s) lck_mtx_unlock((s)->lck)
#else
#define pp_session_lock(s) do { \
    lck_mtx_lock((s)->lck); \
    (s)->lckth = current_thread(); \
} while (0)
#define pp_session_unlock(s) do { \
    (s)->lckth = NULL; \
    lck_mtx_unlock((s)->lck); \
} while(0)
#endif

#define PP_SESSION_LOG         0x00000001
#define PP_SESSION_LOG_ALLOWED 0x00000002
#define PP_SESSION_LOAD_TMOUT  0x00000004
#define PP_SESSION_SND_PENDING 0x00000008

#define PP_SESSION_EOF 0xEFFFFFFF

__private_extern__ pp_session_t   pp_sessions;
__private_extern__ lck_mtx_t     *pp_sessionslck;
#define pp_sessions_lock() lck_mtx_lock(pp_sessionslck)
#define pp_sessions_unlock() lck_mtx_unlock(pp_sessionslck)

__private_extern__ errno_t
pp_session_alloc(pp_session_t *, u_int32_t kctlunit);

__private_extern__ void
pp_session_free(pp_session_t);

// session ref must be taken and lock not held
__private_extern__ errno_t
pp_session_load(pp_session_t sp, pp_msg_t msgp);

// session ref must be taken and lock not held
__private_extern__ errno_t
pp_session_send_stats(pp_session_t sp);

// session ref must be taken and lock not held
__private_extern__ errno_t
pp_session_send_table_info(pp_session_t sp);
