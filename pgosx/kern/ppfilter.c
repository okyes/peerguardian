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
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <libkern/OSMalloc.h>
#include <libkern/OSAtomic.h>
#include <kern/locks.h>
#include <sys/kpi_socketfilter.h>
#include <netinet/in.h>

// XXX not in the kernel headers for some reason
#ifndef INADDR_NONE
#define	INADDR_NONE		0xffffffff		/* -1 return */
#endif

#include "ppmsg.h"
#include "pp_session.h"
#include "pplog.h"
#include "ppk.h"

static int udpFiltDone = 0, tcpFiltDone = 0, udp6FiltDone = 0, tcp6FiltDone = 0,
icmpFiltDone = 0, icmp6FiltDone = 0, rawFiltDone = 0;

__private_extern__ u_int32_t sLastConnect = 0;

#define PP_FILTER_UDP_HANDLE (PP_MSG_MAGIC - (AF_INET + IPPROTO_UDP))
#define PP_FILTER_TCP_HANDLE (PP_MSG_MAGIC - (AF_INET + IPPROTO_TCP))
#define PP_FILTER_ICMP_HANDLE (PP_MSG_MAGIC - (AF_INET + IPPROTO_ICMP))
#define PP_FILTER_UDP6_HANDLE (PP_MSG_MAGIC - (AF_INET6 + IPPROTO_UDP))
#define PP_FILTER_TCP6_HANDLE (PP_MSG_MAGIC - (AF_INET6 + IPPROTO_TCP))
#define PP_FILTER_ICMP6_HANDLE (PP_MSG_MAGIC - (AF_INET6 + IPPROTO_ICMPV6))
#define PP_FILTER_RAW_HANDLE (PP_MSG_MAGIC - (AF_INET + IPPROTO_RAW))

#define PP_IN6_V4_MAPPED_ADDR(a) (*(const in_addr_t *)(const void *)(&(a)->s6_addr[12]))

// Special flags used for logging
#define PP_FLAG_DYN_ENTRY -1
#define PP_FLAG_FLT_STALL -2
#define PP_FLAG_FLT_LOOP  -3
#define PP_FLAG_FLT_UNKW  -4
#define PP_FLAG_FLT_V6ALW -5

// dynamic entries -- used for PASV FTP support right now
// host order
typedef struct pp_dyn_entry {
    u_int32_t ts;
    in_addr_t addr;
    in_port_t port;
    short     block;
} *pp_dyn_entry_t;

typedef struct pp_filter_cookie {
    pid_t pid;
    int32_t action;
} *pp_filter_cookie_t;
#define COOKIE_NO_ACTION -1

#define PP_DYN_ENTRIES_COUNT 16
static struct pp_dyn_entry pp_dyn_entries[PP_DYN_ENTRIES_COUNT] = {{0,},};
static SInt32 pp_dyn_entry_used = 0;

static lck_spin_t *pp_dynlck;
#define pp_dynentries_lock() lck_spin_lock(pp_dynlck)
#define pp_dynentries_unlock() lck_spin_unlock(pp_dynlck)

static pp_dyn_entry_t
pp_dyn_entry_get(void)
{
    int i;
    pp_dyn_entry_t e = NULL;
    u_int32_t now, now_nsec;
    clock_get_calendar_nanotime(&now, &now_nsec);
    
    for(i=0; i < PP_DYN_ENTRIES_COUNT; ++i) {
        // expire old entries
        if ((pp_dyn_entries[i].ts + 5) > now)
            pp_dyn_entries[i].addr = INADDR_NONE;
        
        if (INADDR_NONE == pp_dyn_entries[i].addr) {
            e = &pp_dyn_entries[i];
            e->addr = 0;
            e->ts = now;
            OSIncrementAtomic(&pp_dyn_entry_used);
            break;
        }
    }
    return (e);
}

static pp_dyn_entry_t
pp_dyn_entry_find(in_addr_t ip4, in_port_t port)
{
    int i;
    pp_dyn_entry_t e = NULL;
    for(i=0; i < PP_DYN_ENTRIES_COUNT; ++i) {
        if (ip4 == pp_dyn_entries[i].addr && port == pp_dyn_entries[i].port) {
            e = &pp_dyn_entries[i];
            break;
        }
    }
    return (e);
}

#define pp_dyn_entry_free(ep) do { \
    (ep)->addr = INADDR_NONE; \
    (ep)->ts = 0; \
    OSDecrementAtomic(&pp_dyn_entry_used); \
} while(0)

static
void ppfilter_unregister(sflt_handle h)
{
    // Signal exit
    int *event;
    switch (h) {
        case PP_FILTER_UDP_HANDLE: event = &udpFiltDone; break;
        case PP_FILTER_TCP_HANDLE: event = &tcpFiltDone; break;
        case PP_FILTER_ICMP_HANDLE: event = &icmpFiltDone; break;
        case PP_FILTER_UDP6_HANDLE: event = &udp6FiltDone; break;
        case PP_FILTER_TCP6_HANDLE: event = &tcp6FiltDone; break;
        case PP_FILTER_ICMP6_HANDLE: event = &icmp6FiltDone; break;
        case PP_FILTER_RAW_HANDLE: event = &rawFiltDone; break;
        default: return; break;
    }
    
    (void)OSCompareAndSwap(0, 1, (UInt32*)event);
    wakeup(event);
}

static
errno_t ppfilter_attach(void **cookie, socket_t so)
{
    pp_filter_cookie_t cp = OSMalloc(sizeof(*cp), pp_malloc_tag);
    if (cp) {
        cp->pid = proc_selfpid();
        cp->action = COOKIE_NO_ACTION;
    }
    *cookie = (void*)cp;
    return (0);
}

static
void ppfilter_detach(void *cookie, __unused socket_t so)
{
    if (cookie)
        OSFree(cookie, sizeof(struct pp_filter_cookie), pp_malloc_tag);
}

static
void ppfilter_log (int blocked, const struct sockaddr *from, const struct sockaddr *to,
    const struct sockaddr *remote, int protocol, const pp_msg_iprange_t *match,
    const pg_table_id_t tableid, const pp_filter_cookie_t cookie, const mach_timespec_t *now)
{
    pp_log_entry_t *lep;
    typeof(lep->pplg_timestamp) ts;
    ts = ((typeof(ts))now->tv_sec) * 1000000ULL;
    ts += ((typeof(ts))now->tv_nsec) / 1000ULL;
    
    pp_logentries_lock();
    
    if ((lep = pp_log_get_entry(1))) {
        lep->pplg_timestamp = ts;
        if (blocked)
            lep->pplg_flags |= PP_LOG_BLOCKED;
        else
            lep->pplg_flags |= PP_LOG_ALLOWED;
        if (match->p2_name_idx >= 0) {
            pg_table_id_copy(tableid, lep->pplg_tableid);
            lep->pplg_name_idx = match->p2_name_idx;
        } else {
            bzero(lep->pplg_tableid, sizeof(lep->pplg_tableid));
            
            switch(match->p2_name_idx) {
                case PP_FLAG_DYN_ENTRY:
                    lep->pplg_flags |= PP_LOG_DYN;
                break;
                case PP_FLAG_FLT_STALL:
                    lep->pplg_flags |= PP_LOG_FLTR_STALL;
                break;
                default:
                break;
            }
            
            lep->pplg_name_idx = 0;
        }
        
        if (remote == from)
            lep->pplg_flags |= PP_LOG_RMT_FRM;
        else
            lep->pplg_flags |= PP_LOG_RMT_TO;
        
        if (IPPROTO_TCP == protocol)
            lep->pplg_flags |= PP_LOG_TCP;
        else if (IPPROTO_UDP == protocol)
            lep->pplg_flags |= PP_LOG_UDP;
        else if (IPPROTO_ICMP == protocol || IPPROTO_ICMPV6 == protocol)
            lep->pplg_flags |= PP_LOG_ICMP;
        
        // log addrs are in network order
        const struct sockaddr_in6 *addr6;
        if (AF_INET == from->sa_family) {
            lep->pplg_fromaddr = ((const struct sockaddr_in*)from)->sin_addr.s_addr;
            lep->pplg_fromport = ((const struct sockaddr_in*)from)->sin_port;
        } else if (AF_INET6 == from->sa_family) {
            addr6 = (const struct sockaddr_in6*)from;
            lep->pplg_fromaddr = PP_IN6_V4_MAPPED_ADDR(&addr6->sin6_addr);
            lep->pplg_fromport = addr6->sin6_port;
            lep->pplg_flags |= PP_LOG_IP6;
        }
        if (AF_INET == to->sa_family) {
            lep->pplg_toaddr = ((const struct sockaddr_in*)to)->sin_addr.s_addr;
            lep->pplg_toport = ((const struct sockaddr_in*)to)->sin_port;
        } else if (AF_INET6 == to->sa_family) {
            addr6 = (const struct sockaddr_in6*)to;
            lep->pplg_toaddr = PP_IN6_V4_MAPPED_ADDR(&addr6->sin6_addr);
            lep->pplg_toport = addr6->sin6_port;
            lep->pplg_flags |= PP_LOG_IP6;
        }
        
        if (cookie)
            lep->pplg_pid = cookie->pid;
        else
            lep->pplg_pid = -1;
    } else {
        #ifdef DOLOGWAKE
        ts = 0ULL;
        #endif
        pp_stat_increment(droppedlogs);
    }
    
    pp_logentries_unlock();
    
// Don't signal here, so as to avoid a possible thread preempt, or other stall.
// The new entry will be picked up by the log thread when its timeout expires.
#if DOLOGWAKE
    if (ts)
        pp_log_wakeup();
#endif
}

static // requires table protection
const pp_msg_iprange_t* inrange4(const pp_msg_iprange_t *rp, int32_t ct, in_addr_t ip4)
{
    int32_t h=ct, l=-1, idx;
    
    while (h-l > 1) {
        idx = ((unsigned)(h+l)) >> 1;
        if (rp[idx].p2_ipstart > ip4)
            h = idx;
        else
            l = idx;
    }
    
    if (l > -1 && ip4 >= rp[l].p2_ipstart && ip4 <= rp[l].p2_ipend)
        return (&rp[l]);
    
    return (NULL);
}

__private_extern__ struct pp_iptable *pp_tables; //PeerProtector.c
__private_extern__ struct pp_iptable *rootPortsBlockTable;

static
int match_address(const struct sockaddr *addr, const struct sockaddr *local, int proto,
    pp_msg_iprange_t *match, pg_table_id_t tableid)
{
    in_addr_t ip4;
    in_port_t port, localPort;
    
    match->p2_ipstart = match->p2_ipend = INADDR_NONE;
    // our ip ranges are in host order
    if (AF_INET == addr->sa_family) {
        ip4 = ntohl(((const struct sockaddr_in*)addr)->sin_addr.s_addr);
        port = ntohs(((const struct sockaddr_in*)addr)->sin_port);
    } else if (AF_INET6 == addr->sa_family) {
        const struct sockaddr_in6* addr6 = (const struct sockaddr_in6*)addr;
        if (IN6_IS_ADDR_LOOPBACK(&addr6->sin6_addr) || !IN6_IS_ADDR_V4MAPPED(&addr6->sin6_addr)) {
            match->p2_name_idx = PP_FLAG_FLT_V6ALW;
            return (0); // tables do not contain native ip6 addreses
        }
        
        ip4 = ntohl(PP_IN6_V4_MAPPED_ADDR(&addr6->sin6_addr));
        port = ntohs(addr6->sin6_port);
    } else {
        match->p2_name_idx = PP_FLAG_FLT_UNKW;
        return (0);
    }
    
    if (ntohl(INADDR_LOOPBACK) == ip4) {
        match->p2_name_idx = PP_FLAG_FLT_LOOP;
        return (0); // allow anything on loopback
    }
    
    if (AF_INET == local->sa_family)
        localPort = ntohs(((const struct sockaddr_in*)local)->sin_port);
    else if (AF_INET6 == local->sa_family)
        localPort = ntohs(((const struct sockaddr_in6*)local)->sin6_port);
    else
        localPort = 0;
    
    int hit = 0;
    // check dynamic entries first
    if (pp_dyn_entry_used) {
        pp_dynentries_lock();
        pp_dyn_entry_t e = pp_dyn_entry_find(ip4, port);
        if (e) {
            hit = e->block;
            pp_dyn_entry_free(e);
        }
        pp_dynentries_unlock();
        if (e) {
            match->p2_ipstart = match->p2_ipend = ip4;
            match->p2_name_idx = PP_FLAG_DYN_ENTRY;
            return (hit);
        }
    }
    
    pp_usetables();
    
    if (pp_tablesloadwanted) {
        pp_relsetables();
        pp_stat_increment(filter_ignored);
        match->p2_name_idx = PP_FLAG_FLT_STALL;
        return (1); // block
    }
    
    // We can now safely access the tables read-only w/o the lock
    int stdport = (80 == port /* || 8080 == port */ || 443 == port /*FTP*/ || 20 == port || 21 == port
        /*|| IPPROTO_ICMP == proto || IPPROTO_ICMPV6 == proto*/);
    const pp_msg_iprange_t *found = NULL;
    const struct pp_iptable *tbl;
    
    // check for attempts from a privileged port
    if (stdport && localPort && (tbl = rootPortsBlockTable)) {
        if ((found = inrange4((const pp_msg_iprange_t*)tbl->ipranges, tbl->rangect, (in_addr_t)localPort)))
            stdport = 0; // check the lists for the address
    }
    tbl = pp_tables;
    match->p2_name_idx = 0;
    for (; tbl; tbl = tbl->next) {
        // alwayspass tables always have to be checked
        if (0 == (tbl->flags & kTableAlwaysPass) && 0 == (tbl->flags & kTableBlockStdPorts) && stdport)
            continue;
        
        if ((found = inrange4((const pp_msg_iprange_t*)tbl->ipranges, tbl->rangect, ip4))) {
            *match = *found;
            pg_table_id_copy(tbl->tableid, tableid);
            if (0 == (tbl->flags & kTableAlwaysPass))
                hit++;
            else
                hit = 0;
            break;
        }
    }
    
    pp_relsetables();
    
    if (!found)
        bzero(tableid, sizeof(pg_table_id_t));
    
    return (hit);
}

#ifdef PPDBG
static u_int64_t felpsd_high = 0ULL, felpsd_low = 0xffffffffffffffffULL;
static u_int64_t filtTotalTime = 0ULL;
static u_int32_t filtct = 0UL;
// (filtTotalTime - felpsd_high - felpsd_low) / (filtct - 2) gives the avg per connection
#define filt_start() \
mach_timespec_t begin; \
clock_get_calendar_nanotime(&begin.tv_sec, (u_int32_t*)&begin.tv_nsec); \
(void)OSIncrementAtomic((SInt32*)&filtct);

#define filt_end() do { \
    mach_timespec_t end; \
    clock_get_calendar_nanotime(&end.tv_sec, (u_int32_t*)&end.tv_nsec); \
    end.tv_sec -= begin.tv_sec; \
    end.tv_nsec -= begin.tv_nsec; \
    /* huge problem if secs is not zero, but to be complete... */ \
    u_int64_t elapsed = ((u_int64_t)end.tv_sec) * 1000000000; \
    elapsed += (u_int64_t)end.tv_nsec; \
    pp_dynentries_lock(); \
    filtTotalTime += elapsed; \
    if (elapsed > felpsd_high) \
        felpsd_high = elapsed; \
    else if (elapsed < felpsd_low) \
        felpsd_low = elapsed; \
    pp_dynentries_unlock(); \
} while(0)
#else
#define filt_start()
#define filt_end()
#endif

static
errno_t ppfilter_connect (pp_filter_cookie_t cookie, socket_t so, const struct sockaddr *remote, int connectin)
{
    pp_msg_iprange_t match;
    pg_table_id_t tableid;
    
    filt_start();
    
    struct sockaddr_in6 local;
    if (0 != sock_getsockname(so, (struct sockaddr*)&local, sizeof(local)))
        bzero(&local, sizeof(local));
    
    int protocol = 0;
    (void)sock_gettype(so, NULL, NULL, &protocol);
    int block = match_address(remote, (const struct sockaddr*)&local, protocol, &match, tableid);
    
    mach_timespec_t now;
    clock_get_calendar_nanotime(&now.tv_sec, (u_int32_t*)&now.tv_nsec);
    u_int32_t lastConnect = sLastConnect;
    if (now.tv_sec >= (lastConnect+1)) {
        (void)ExchangeAtomic(now.tv_sec, &sLastConnect);
        (void)ExchangeAtomic(0, &pp_stats.pps_conns_per_sec);
        (void)ExchangeAtomic(0, &pp_stats.pps_blcks_per_sec);
    }
    
    // For now we always log even if a log client is not connected, that way
    // we keep as many records as possible so when a log client connects
    // it can save them.
#ifdef notyet
    if (pp_logclients && (block || pp_logallowed)) {
#endif
        const struct sockaddr *to, *from;
        if (!connectin) {
            to = remote;
            from = (const struct sockaddr*)&local;
        } else {
            to = (const struct sockaddr*)&local;
            from = remote;
        }
        
        ppfilter_log(block, from, to, remote, protocol, &match, tableid, cookie, &now);
#ifdef notyet
    }
#endif
    
    if (0 == block) {
        connectin ? pp_stat_increment(in_allowed) : pp_stat_increment(out_allowed);
    } else {
        connectin ? pp_stat_increment(in_blocked) : pp_stat_increment(out_blocked);
        block = ECONNREFUSED;
        pp_stat_increment(blcks_per_sec);
    }
    pp_stat_increment(conns_per_sec);
    
    filt_end();
    
    return (block);
}

static
errno_t ppfilter_connect_in (void *cookie, socket_t so, const struct sockaddr *from)
{
    errno_t block = ppfilter_connect((pp_filter_cookie_t)cookie, so, from, 1);
    // action is used by the raw data filters
    if (cookie) {
        if (!block)
            (void)OSCompareAndSwap(COOKIE_NO_ACTION, 0, (UInt32*)&((pp_filter_cookie_t)cookie)->action);
        else
            (void)OSCompareAndSwap(COOKIE_NO_ACTION, EHOSTUNREACH, (UInt32*)&((pp_filter_cookie_t)cookie)->action);
    }
    return (block);
}

static
errno_t ppfilter_connect_out (void *cookie, socket_t so, const struct sockaddr *to)
{
    errno_t block = ppfilter_connect((pp_filter_cookie_t)cookie, so, to, 0);
    // action is used by the raw data filters
    if (cookie) {
        if (!block)
            (void)OSCompareAndSwap(COOKIE_NO_ACTION, 0, (UInt32*)&((pp_filter_cookie_t)cookie)->action);
        else
            (void)OSCompareAndSwap(COOKIE_NO_ACTION, EHOSTUNREACH, (UInt32*)&((pp_filter_cookie_t)cookie)->action);
    }
    return (block);
}

#include "ppdatafilter.c"

// data in is currently used for PASV FTP support
static
errno_t	ppfilter_data_in (__unused void *cookie, socket_t so, const struct sockaddr *from,
    mbuf_t *data, __unused mbuf_t *control, __unused sflt_data_flag_t flags)
{
    in_addr_t ip4;
    in_port_t port;
    
    if (!from) {
        struct sockaddr_in6 local;
        if (0 != sock_getpeername(so, (struct sockaddr*)&local, sizeof(local)))
            bzero(&local, sizeof(local));
        from = (const struct sockaddr*)&local;
    }
    
    if (AF_INET == from->sa_family) {
        port = ntohs(((const struct sockaddr_in*)from)->sin_port);
    } else if (AF_INET6 == from->sa_family) {
        const struct sockaddr_in6* addr6 = (const struct sockaddr_in6*)from;
        if (IN6_IS_ADDR_LOOPBACK(&addr6->sin6_addr) || !IN6_IS_ADDR_V4MAPPED(&addr6->sin6_addr))
            return (0); // tables do not contain native ip6 addreses
        port = ntohs(addr6->sin6_port);
    } else
        return (0);
    
    // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
    // Short-circuit optimization for ftp filter -- if any other filters are ever added,
    // this will have to be removed.
    if (21 != port)
        return (0);
    // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
    
    mbuf_t mdata = *data;
    while (mdata && MBUF_TYPE_DATA != mbuf_type(mdata)) {
        mdata = mbuf_next(mdata);
    }
    if (!mdata)
        return (0);
        
    char *pkt = mbuf_data(mdata);
    if (!pkt)
        return (0);
    size_t len = mbuf_len(mdata);
    
    ip4 = INADDR_NONE;
    int block, i;
    pp_data_filter_t filt = pp_data_filters[0];
    for (i = 1; filt; ++i) {
        block = filt(pkt, len, &ip4, &port);
        if (INADDR_NONE != ip4) {
            // add to dynamic list
            pp_dynentries_lock();
            pp_dyn_entry_t e = pp_dyn_entry_get();
            if (e) {
                e->addr = ip4;
                e->port = port;
                e->block = block;
            }
            pp_dynentries_unlock();
            break;
        }
        filt = pp_data_filters[i];
    }
    
    return (0);
}

// data raw is used for ICMP
static
errno_t	ppfilter_data_in_raw (void *cookie, socket_t so, const struct sockaddr *from,
    __unused mbuf_t *data, __unused mbuf_t *control, __unused sflt_data_flag_t flags)
{
    int32_t action = cookie ? ((struct pp_filter_cookie*)cookie)->action : COOKIE_NO_ACTION;
    if (action > COOKIE_NO_ACTION)
        return (action); // socket has already been filtered -- this will occur with UDP sockets that call connect
    
    if (!from) {
        struct sockaddr_in6 local;
        if (0 != sock_getpeername(so, (struct sockaddr*)&local, sizeof(local)))
            bzero(&local, sizeof(local));
        from = (const struct sockaddr*)&local;
    }
    
    return ( ppfilter_connect((pp_filter_cookie_t)cookie, so, from, 1) ? EHOSTUNREACH : 0 );
}

static
errno_t	ppfilter_data_out_raw (void *cookie, socket_t so, const struct sockaddr *to,
    __unused mbuf_t *data, __unused mbuf_t *control, __unused sflt_data_flag_t flags)
{
    int32_t action = cookie ? ((struct pp_filter_cookie*)cookie)->action : COOKIE_NO_ACTION;
    if (action > COOKIE_NO_ACTION)
        return (action); // socket has already been filtered -- this will occur with UDP sockets that call connect
    
    if (!to) {
        struct sockaddr_in6 local;
        if (0 != sock_getpeername(so, (struct sockaddr*)&local, sizeof(local)))
            bzero(&local, sizeof(local));
        to = (const struct sockaddr*)&local;
    }
    
    return ( ppfilter_connect((pp_filter_cookie_t)cookie, so, to, 0) ? EHOSTUNREACH : 0 );
}

static
void pp_filter_deregister_handle(sflt_handle h, int *event)
{
    struct timespec ts = {2, 0};
    
    if (0 == sflt_unregister(h)) {
        // Wait for unregister to complete
        while (0 == *event) {
            if (ETIMEDOUT != msleep(event, NULL, PWAIT, __FUNCTION__, &ts))
                break;
        }
    }
}

__private_extern__
int pp_filter_register()
{
    struct sflt_filter pp_filter = {
        0, // sf_handle
        SFLT_GLOBAL, // sf_flags
        0, // sf_name
        ppfilter_unregister, // sf_unregistered
        ppfilter_attach, // sf_attach
        ppfilter_detach, // sf_detach
        NULL, // sf_notify
        NULL, // sf_getpeername
        NULL, // sf_getsockname
        NULL, // sf_data_in
        NULL, // sf_data_out
        ppfilter_connect_in, // sf_connect_in
        ppfilter_connect_out, // sf_connect_out
        NULL, // sf_bind
        NULL, // sf_setoption
        NULL, // sf_getoption
        NULL, // sf_listen
        NULL, // sf_ioctl
    };
    
    int i;
    for(i=0; i < PP_DYN_ENTRIES_COUNT; ++i)
        pp_dyn_entries[i].addr = INADDR_NONE;
    
    pp_dynlck = lck_spin_alloc_init(pp_spin_grp, LCK_ATTR_NULL);
    if (!pp_dynlck)
        return (ENOMEM);
    
    errno_t err;
    
    pp_filter.sf_handle = PP_FILTER_TCP_HANDLE;
    // data filter is used for PASV FTP support
    pp_filter.sf_data_in = ppfilter_data_in;
    pp_filter.sf_name = "PeerGuardian TCP";
    if ((err = sflt_register(&pp_filter, AF_INET, SOCK_STREAM, IPPROTO_TCP))) {
        printf("PeerGuardian: Failed to register '%s' filter: %d.\n", pp_filter.sf_name, err);
        return (err);
    }
    
    pp_filter.sf_handle = PP_FILTER_TCP6_HANDLE;
    pp_filter.sf_name = "PeerGuardian TCP6";
    if ((err = sflt_register(&pp_filter, AF_INET6, SOCK_STREAM, IPPROTO_TCP))) {
        printf("PeerGuardian: Failed to register '%s' filter: %d.\n", pp_filter.sf_name, err);
        pp_filter_deregister_handle(PP_FILTER_TCP_HANDLE, &tcpFiltDone);
        goto filter_register_exit;
    }
    pp_filter.sf_data_in = (typeof(pp_filter.sf_data_in))NULL;
    
    // UDP can "connect", but it can also just send the data, so we need to monitor both
    pp_filter.sf_data_in = ppfilter_data_in_raw;
    pp_filter.sf_data_out = ppfilter_data_out_raw;
    
    pp_filter.sf_handle = PP_FILTER_UDP_HANDLE;
    pp_filter.sf_name = "PeerGuardian UDP";
    if ((err = sflt_register(&pp_filter, AF_INET, SOCK_DGRAM, IPPROTO_UDP))) {
        printf("PeerGuardian: Failed to register '%s' filter: %d.\n", pp_filter.sf_name, err);
        pp_filter_deregister_handle(PP_FILTER_TCP_HANDLE, &tcpFiltDone);
        pp_filter_deregister_handle(PP_FILTER_TCP6_HANDLE, &tcp6FiltDone);
        goto filter_register_exit;
    }
    
    pp_filter.sf_handle = PP_FILTER_UDP6_HANDLE;
    pp_filter.sf_name = "PeerGuardian UDP6";
    if ((err = sflt_register(&pp_filter, AF_INET6, SOCK_DGRAM, IPPROTO_UDP))) {
        printf("PeerGuardian: Failed to register '%s' filter: %d.\n", pp_filter.sf_name, err);
        pp_filter_deregister_handle(PP_FILTER_TCP_HANDLE, &tcpFiltDone);
        pp_filter_deregister_handle(PP_FILTER_TCP6_HANDLE, &tcp6FiltDone);
        pp_filter_deregister_handle(PP_FILTER_UDP_HANDLE, &udpFiltDone);
        goto filter_register_exit;
    }
    
    // RAW sockets don't "connect", they just send/recv data
    pp_filter.sf_connect_in = (typeof(pp_filter.sf_connect_in))NULL;
    pp_filter.sf_connect_out = (typeof(pp_filter.sf_connect_out))NULL;
    
    // Failures of the following are not fatal
    pp_filter.sf_handle = PP_FILTER_ICMP_HANDLE;
    pp_filter.sf_name = "PeerGuardian ICMP";
    if ((err = sflt_register(&pp_filter, AF_INET, SOCK_RAW, IPPROTO_ICMP))) {
        printf("PeerGuardian: Failed to register '%s' filter: %d.\n", pp_filter.sf_name, err);
        icmpFiltDone = -1;
    }
    
    pp_filter.sf_handle = PP_FILTER_ICMP6_HANDLE;
    pp_filter.sf_name = "PeerGuardian ICMP6";
    if ((err = sflt_register(&pp_filter, AF_INET6, SOCK_RAW, IPPROTO_ICMPV6))) {
        printf("PeerGuardian: Failed to register '%s' filter: %d.\n", pp_filter.sf_name, err);
        icmp6FiltDone = -1;
    }
        
    pp_filter.sf_handle = PP_FILTER_RAW_HANDLE;
    pp_filter.sf_name = "PeerGuardian RAW";
    if ((err = sflt_register(&pp_filter, AF_INET, SOCK_RAW, IPPROTO_RAW))) {
        printf("PeerGuardian: Failed to register '%s' filter: %d.\n", pp_filter.sf_name, err);
        rawFiltDone = -1;
    }
    
    err = 0;
    
filter_register_exit:
    if (err && pp_dynlck)
        lck_spin_free(pp_dynlck, pp_spin_grp);

    return (err);
}

__private_extern__
int pp_filter_deregister()
{
    // Bug in 10.4.2 - if we attached to an IP6 socket, the kernel will
    // panic when we try to deregister the filter. It appears the socket
    // stays on the filter list even when closed. Radar# 4234311.
    // This is fixed in 10.4.3.
    
    pp_filter_deregister_handle(PP_FILTER_UDP_HANDLE, &udpFiltDone);
    pp_filter_deregister_handle(PP_FILTER_TCP_HANDLE, &tcpFiltDone);
    pp_filter_deregister_handle(PP_FILTER_UDP6_HANDLE, &udp6FiltDone);
    pp_filter_deregister_handle(PP_FILTER_TCP6_HANDLE, &tcp6FiltDone);
    
    if (-1 != icmpFiltDone)
        pp_filter_deregister_handle(PP_FILTER_ICMP_HANDLE, &icmpFiltDone);
    if (-1 != icmp6FiltDone)
        pp_filter_deregister_handle(PP_FILTER_ICMP6_HANDLE, &icmp6FiltDone);
    if (-1 != rawFiltDone)
        pp_filter_deregister_handle(PP_FILTER_RAW_HANDLE, &rawFiltDone);
    
    lck_spin_free(pp_dynlck, pp_spin_grp);
    return (0);
}
