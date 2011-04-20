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
#ifndef PP_MSG_H
#define PP_MSG_H

/*!
@header p2b library support
List structures used by the p2b library
@copyright Brian Bergstrand.
*/

#include <sys/types.h>

#pragma pack(1)

/*!
@struct pg_range4
@abstract Defines an IPv4 aaddress range.
@discussion All addresses are in host order.
@field p2_ipstart The starting address.
@field p2_ipend The ending Address.
@field p2_name_idx The index into the name table as returned by p2b_parse.
*/
typedef struct pg_range4 {
    u_int32_t p2_ipstart;
    u_int32_t p2_ipend;
    int32_t p2_name_idx;
} pg_range4_t, pp_msg_iprange_t;
// Backwards compat
#define pp_msg_iprange pg_range4

#ifdef notyet

#if BYTE_ORDER == BIG_ENDIAN
#define PG_INADDR6_QHIGH 0
#define PG_INADDR6_QLOW 1
#elif BYTE_ORDER == LITTLE_ENDIAN
#define PG_INADDR6_QHIGH 1
#define PG_INADDR6_QLOW 0
#endif
typedef struct pg_inaddr6 {
    union {
        unsigned char pg6_bytes[16];
        unsigned long long pg6_quads[2];
    };
} pg_inaddr6_t;

typedef struct pg_range6 {
    pg_inaddr6_t pgr_start;
    pg_inaddr6_t pgr_end;
    int32_t pg_nameidx;
} pg_range6_t;

typedef union pg_range {
    pg_range6_t pgr_6;
    pg_range4_t pgr_4;
} pg_range_t;

#endif // notyet

#ifdef __APPLE__

#define PP_CTL_NAME "xxx.qnation.PeerGuardian"

// Always big-endian
typedef unsigned char pg_table_id_t[16];

typedef struct pp_msg {
    u_int32_t ppm_magic;
    u_int16_t ppm_version;
    u_int16_t ppm_size;
    u_int32_t ppm_sequence;
    u_int32_t ppm_type;
    union {
        u_int32_t ppm_flags;
        int32_t   ppm_count; // TYPE_LOG or TYPE_TBL_INFO when sent from kernel
        #define ppm_logcount ppm_count
    };
    u_int8_t  ppm_data[0];
} *pp_msg_t;

#define pg_msg_cast(t, m) ((t)(m)->ppm_data)

#define PP_MSG_RECOMMENDED_MAX 2048 // CTL_SENDSIZE

#define PP_MSG_MAGIC 0xa024c05d
#define PP_MSG_VERSION 3

#define PP_MSG_TYPE_LOAD 1
#define PP_MSG_TYPE_LOG  2
#define PP_MSG_TYPE_REM  3
#define PP_MSG_TYPE_STAT 4
#define PP_MSG_TYPE_TBL_INFO 5

#ifdef PG_MAKE_TABLE_ID
#define PG_DEFINE_TABLE_ID(name, b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15) \
__private_extern__ const pg_table_id_t name = \
{b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15}
#else
#define PG_DEFINE_TABLE_ID(name, b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15) \
__private_extern__ const pg_table_id_t name;
#endif

static __inline__ __attribute__((always_inline)) int 
pg_table_id_cmp(const pg_table_id_t tid1, const pg_table_id_t tid2)
{
    return (bcmp(tid1, tid2, sizeof(pg_table_id_t)));
}

static __inline__ __attribute__((always_inline)) void
pg_table_id_copy(const pg_table_id_t src, pg_table_id_t dst)
{
    bcopy(src, dst, sizeof(pg_table_id_t));
}

// Give as id to REM to unload all tables.
PG_DEFINE_TABLE_ID(PG_TABLE_ID_ALL,
0x73, 0x21, 0x69, 0x39, 0xcc, 0xe6, 0x41, 0x61, 0xb1, 0xf1, 0xf4, 0x5c, 0x17, 0x96, 0xd9, 0x9d);
#define PP_TABLE_ID_ALL PG_TABLE_ID_ALL

typedef struct pp_msg_load {
    u_int32_t ppld_version;
    u_int32_t ppld_flags;
    int32_t   ppld_count; // range count
    u_int32_t ppld_addrct; // address count
    pg_table_id_t ppld_tableid;
    pp_msg_iprange_t ppld_rngs[0]; // must be sorted
} pp_msg_load_t;

#define PP_LOAD_VERSION 3
// Load begin is just the total count and the id only
// Ranges should be zero
#define PP_LOAD_BEGIN      0x00000001
// Count and ranges should be 0
#define PP_LOAD_END        0x00000002
// Blocks access from remote std.ports to specified local ports
// Must be given with PP_LOAD_END msg
#define PP_LOAD_PORT_RULE  0x00000004

// Block HTTP, FTP
#define PP_LOAD_BLOCK_STD  0x00000010
#define PP_LOAD_ALWYS_PASS 0x00000020

#define PP_LOAD_VALID_FLGS (PP_LOAD_BEGIN|PP_LOAD_END|PP_LOAD_PORT_RULE|PP_LOAD_BLOCK_STD|PP_LOAD_ALWYS_PASS)

typedef struct pg_msg_rem {
    pg_table_id_t pgr_tableid;
} pg_msg_rem_t;

typedef struct pp_log_entry {
    u_int64_t pplg_timestamp; // UTC microseconds since epoch
    // sorry, no 64bit list support for now
#ifdef __LP64__
    u_int64_t pplg_rsrvd;
#else
    struct pp_log_entry *pplg_next;
    struct pp_log_entry *pplg_prev;
#endif
    u_int32_t pplg_flags;
    int32_t pplg_name_idx; // idx into name table
    int32_t pplg_pid; // at the time of socket creation
    // network order
    u_int32_t pplg_fromaddr;
    u_int32_t pplg_toaddr;
    u_int16_t pplg_fromport;
    u_int16_t pplg_toport;
    pg_table_id_t pplg_tableid; // name table id
} pp_log_entry_t;

#define PP_LOG_ALLOWED 0x00000001
#define PP_LOG_BLOCKED 0x00000002
#define PP_LOG_UDP     0x00000004
#define PP_LOG_TCP     0x00000008
#define PP_LOG_IP6     0x00000010
#define PP_LOG_ICMP    0x00000020
// A table load was in progress while the filter was running
// and no tables were actually checked.
#define PP_LOG_FLTR_STALL 0x00000040
// This was a dynamic entry that the kext created on the fly
#define PP_LOG_DYN     0x00000080
#define PP_LOG_RMT_TO  0x00000100
#define PP_LOG_RMT_FRM 0x00000200

// Table events (load/unload/reload)
#define PP_LOG_TBL_LD  0x00100000
#define PP_LOG_TBL_ULD 0x00200000
#define PP_LOG_TBL_RLD 0x00400000

#define PP_LOG_INUSE   0x80000000 // no significance outside of kernel

#define PP_LOG_TBL_EVT (PP_LOG_TBL_LD|PP_LOG_TBL_ULD|PP_LOG_TBL_RLD)

typedef struct pp_msg_stats {
    u_int32_t pps_version;
    u_int32_t pps_in_blocked;
    u_int32_t pps_in_allowed;
    u_int32_t pps_out_blocked;
    u_int32_t pps_out_allowed;
    u_int32_t pps_droppedlogs;
    u_int32_t pps_filter_ignored;
    u_int32_t pps_table_loadwait;
    u_int32_t pps_table_loadfail;
    u_int32_t pps_userclients;
    u_int32_t pps_blocked_addrs; // IPv4 max ct is 2^32
    u_int32_t pps_conns_per_sec;
    u_int32_t pps_blcks_per_sec;
    u_int64_t pps_loadtime; // KEXT load time in UTC microseconds since epoch
} pp_msg_stats_t;
#define PP_STATS_VERSION 2

typedef struct pp_msg_table_info {
    u_int64_t loadtime; // UTC seconds since epoch
    u_int32_t size; // memory used
    int32_t   rangect;
    u_int32_t addrct;
    u_int32_t flags; // PP_LOAD flags
    pg_table_id_t tableid;
} pp_msg_table_info_t;

#endif // __APPLE__

#pragma pack(0)

#endif // PP_MSG_H
