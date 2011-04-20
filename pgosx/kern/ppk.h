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
#include <sys/kern_control.h>

#include "ppmsg.h"

__private_extern__ lck_grp_t *pp_spin_grp;
__private_extern__ lck_grp_t *pp_mtx_grp;

__private_extern__ kern_ctl_ref pp_ctl_ref;
__private_extern__ OSMallocTag pp_malloc_tag;

__private_extern__ pp_msg_stats_t pp_stats;
#define pp_stat_increment(st) (void)OSIncrementAtomic((SInt32*)&pp_stats.pps_ ## st)
#define pp_stat_decrement(st) (void)OSDecrementAtomic((SInt32*)&pp_stats.pps_ ## st)

typedef enum {
    kTableBlockStdPorts = 0x00000001,
    kTableAlwaysPass    = 0x00000002,
    kTableIOKitAlloc    = 0x00000004
} pp_iptable_flags_t;

struct pp_iptable {
    struct pp_iptable *next;
    struct pp_iptable *prev;
    u_int32_t size;
    int32_t   rangect;
    u_int32_t addrct;
    u_int32_t loadtime;
    u_int32_t flags;
    uid_t owner;
    pg_table_id_t tableid;
    pp_msg_iprange_t ipranges[0];
};

__private_extern__ lck_mtx_t *pp_tableslck;
#define pp_tables_lock() lck_mtx_lock(pp_tableslck)
#define pp_tables_unlock() lck_mtx_unlock(pp_tableslck)

__private_extern__ int32_t pp_tablesusect;
__private_extern__ int32_t pp_tablesloadwanted;
#define pp_usetables() (void)OSIncrementAtomic((SInt32*)&pp_tablesusect)
#define pp_relsetables() do { \
    (void)OSDecrementAtomic((SInt32*)&pp_tablesusect); \
    if (pp_tablesloadwanted > 0) \
        wakeup(&pp_tablesusect); \
} while(0)

// if sp is not null, it is assumed locked and the lock will be dropped if a wait is necessary
struct pp_session; // forward
__private_extern__ int pp_tables_acquire_mod(struct pp_session* sp);
#define pp_tables_relse_mod() (void)OSDecrementAtomic((SInt32*)&pp_tablesloadwanted);

__private_extern__ int
pp_table_alloc(struct pp_iptable **, int32_t rangect);
__private_extern__
struct pp_iptable* pp_find_table_byid(const pg_table_id_t, int locked);
__private_extern__ int
pp_table_insert(struct pp_iptable *);
__private_extern__ int
pp_table_free(struct pp_iptable*, int locked);
__private_extern__ int
pp_table_auth(struct pp_iptable *tp);

static inline
unsigned int ExchangeAtomic(register unsigned int newVal, unsigned int volatile *addr)
{
    register unsigned int val;
    do {
        val = *addr;
    } while (val != newVal && FALSE == OSCompareAndSwap(val, newVal, (UInt32*)addr));
    return (val);
}

#ifdef PPDBG
__private_extern__
void pp_assert_failed(const char*, const char*, const char*, int);
#define PPASSERT(condition,msg) do { \
    if (0 == (condition)) { \
        pp_assert_failed(#condition, msg, __FUNCTION__, __LINE__); \
    } \
} while(0)

#define PPDBG_ONLY
#else
#define PPASSERT(condition,msg) {}
#define PPDBG_ONLY __unused
#endif
