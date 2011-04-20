/*
    Copyright 2005-2007 Brian Bergstrand

    This program is free software; you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program;
    if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#ifndef PP_LIB_H
#define PP_LIB_H

/*!
@header p2b library support
Internal p2b support functions.
@copyright Brian Bergstrand.
*/

#include <sys/types.h>
#include <netinet/in.h>

#include "ppmsg.h"

#ifndef __unused
#define	__unused	__attribute__((__unused__))
#endif

#ifndef PP_EXTERN
#ifdef __APPLE__
#ifdef PP_SHARED
#define PP_EXTERN extern
#else
#define PP_EXTERN __private_extern__
#endif
#else
#define PP_EXTERN
#endif // __APPLE__
#endif

#ifdef __APPLE__

typedef struct pp_session_ *pp_session_t;

PP_EXTERN
int pp_open_session(pp_session_t *);

PP_EXTERN
int pp_close_session(pp_session_t);

// load table into kernel
PP_EXTERN
int pp_load_table(const pp_msg_iprange_t *addrs, int32_t rangecount, u_int32_t addrcount,
    const pg_table_id_t tableid, int loadflags);

// unload table from kernel
PP_EXTERN
int pp_unload_table(const pg_table_id_t tableid);

// load table names into shared memory for use in logging
PP_EXTERN
int pp_load_names(const pg_table_id_t tableid, const char *tblname,  u_int8_t* const *names);

// map shared mem names into current vm space
typedef struct pp_name_map_ *pp_name_map_t; 
PP_EXTERN
int pp_map_names(const pg_table_id_t tableid, pp_name_map_t *handle);

// caller must free 'name' upon successful return
PP_EXTERN
int pp_resolve_name(const pp_name_map_t handle, int32_t index, char **name);

// caller must free 'name' upon successful return
PP_EXTERN
int pp_map_name(const pp_name_map_t handle, char **name);

PP_EXTERN
int pp_unmap_names(pp_name_map_t handle);

PP_EXTERN
int pp_unload_names(const pg_table_id_t tableid);

typedef struct pp_log_handle_* pp_log_handle_t;
// this will NOT be called on the main thread -- entry and table names may be NULL
typedef void (*pp_log_entry_cb)(const pp_log_entry_t*, const char *entryName, const char *tableName);

// if handle is NULL, the call will never return, tableEvent may be NULL, logEvent may not
PP_EXTERN
int pp_log_listen(int logallowed, pp_log_entry_cb logEvent, pp_log_entry_cb tableEvent, pp_log_handle_t *handle);

PP_EXTERN
int pp_log_stop(pp_log_handle_t handle);

typedef struct pp_log_stats {
    u_int32_t pps_entriesQueued;
    u_int32_t pps_entriesReceived; // lifetime
    u_int32_t pps_entriesProcessed; // lifetime
    unsigned char pps_reserved[116];
} *pp_log_stats_t;

PP_EXTERN
int pp_log_stats(pp_log_handle_t handle, pp_log_stats_t stats);

PP_EXTERN
int pp_statistics_s(pp_session_t s, pp_msg_stats_t *);

static __inline__ int pp_statistics(pp_msg_stats_t *stp)
{
    return (pp_statistics_s(NULL, stp));
}

// Lifetime (since KEXT load) stats
PP_EXTERN
int pp_avgconnstats(const pp_msg_stats_t *, u_int32_t *connps, u_int32_t *blockps);

// on input, count should contain the number of elements in ti
// on successful return, count will contain the actual number of tables
PP_EXTERN
int pp_tableinfo(pp_msg_table_info_t *ti, int32_t *count);

PP_EXTERN char*
pg_table_id_string(const pg_table_id_t tid, char *buf, size_t len, int separators);

#ifdef __COREFOUNDATION__
PP_EXTERN int pg_table_id_fromstr(const char *str, pg_table_id_t tid);

PP_EXTERN int pg_table_id_fromcfstr(CFStringRef str, pg_table_id_t tid);

// string ref can be NULL - if not, must be released by caller
PP_EXTERN void pg_table_id_create(pg_table_id_t tid, CFStringRef *);
#endif

PG_DEFINE_TABLE_ID(PG_TABLE_ID_PORTS,
0xb5, 0x8f, 0x3b, 0xf4, 0xe0, 0x0f, 0x40, 0xa3, 0xbc, 0xa5, 0x1e, 0xbb, 0xcd, 0x4c, 0xd8, 0x53);
#define PP_TABLE_ID_PORTS PG_TABLE_ID_PORTS

#define ANY_USER 0xffffffffUL
__private_extern__ int FindPIDByPathOrName(const char *path, const char *name, uid_t user);

#endif // __APPLE__

// ip4 must be in host order
PP_EXTERN const pp_msg_iprange_t*
pp_match_addr(const pp_msg_iprange_t *addrs, int32_t addrcount, in_addr_t ip4);

typedef void (*pp_enum_callback_t)(const char *name, const char *ipStart, const char *ipEnd,
    int32_t index, void *enumContext);

PP_EXTERN
int pp_enum_table(const pp_msg_iprange_t *addrs, int32_t addrcount, u_int8_t* const *names,
    pp_enum_callback_t cb, void *enumContext);

PP_EXTERN
int pp_print_table(const pp_msg_iprange_t *addrs, int32_t addrcount, u_int8_t* const *names);

/*!
@function pp_inet_ntoa
@abstract Thread-safe address to string conversion.
@param addr Addresses in network byte order.
@param b Storage for string output.
@param len Storage length.
@result Pointer to given storage, or NULL if an error occurred.
*/
PP_EXTERN
char* pp_inet_ntoa(struct in_addr addr, char *b, size_t len);

#ifdef __ppc__
    #define trap() asm volatile("trap")
    #define PP_BAD_ADDRESS 0xdeadbeef
#elif defined(__i386__)
    #define trap() asm volatile("int $3")
    #define PP_BAD_ADDRESS 0xbaadf00d
#else
    #error unknown arch
#endif

#ifndef PPDBG
    #define dbgbrk()
    #define ppassert(c)
#else
    #define dbgbrk() trap()
    #define ppassert(c) do {if (0 == (c)) trap();} while(0)
#endif

#if defined(__APPLE__) && defined(__OBJC__)

// Address specification keys.
// NSString, required - dotted quad IP addr
#define PP_ADDR_SPEC_START @"IP4Start"
// NSString, optional - dotted quad IP addr
#define PP_ADDR_SPEC_END @"IP4End"
// NSString, optional - Range description
#define PP_ADDR_SPEC_NAME @"Description"
// NSNumber (BOOL), optional - If true, block address instead of allow (which is the default)
#define PP_ADDR_SPEC_BLOCK @"Block"
// NSNumber, required - Number of seconds until address expires. If UINT_MAX, perm-allow.
#define PP_ADDR_SPEC_EXPIRE @"Expire"

@interface NSObject (PPLibExtensions)

- (NSMutableArray*)addressSpecificationsFromFile:(NSString*)path;

// ranges = Array of address specifications (dictionary)
- (int)commitAddressSpecifications:(NSArray*)ranges toFile:(NSString*)path;
- (int)commitAddressSpecifications:(NSArray*)ranges toFile:(NSString*)path inBackground:(BOOL)background;
    
@end

#endif // __APPLE__ && __OBJC__

#endif // PP_LIB_H
