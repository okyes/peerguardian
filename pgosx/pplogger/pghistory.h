/*
    Copyright 2006 Brian Bergstrand

    This program is free software; you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program;
    if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef PG_HIST_H
#define PG_HIST_H

#include <sys/types.h>

#pragma pack(1)

typedef struct pg_hist_hdr {
    u_int32_t ph_byteorder;
    u_int32_t ph_magic;
    u_int32_t ph_version;
    unsigned char ph_reserved[52];
} pg_hist_hdr_t;
#define PG_HIST_VER 1
#define PG_HIST_MAGIC 0x49fab799

typedef struct pg_hist_ent {
    u_int64_t ph_timestamp; // UTC microseconds since epoch
    u_int16_t ph_emagic;
    u_int16_t ph_flags;
    // u_int32_t ph_uid; // v2
    u_int16_t ph_port; //local port
    u_int16_t ph_rport; // remote port
} pg_hist_ent_t;
#define PG_HIST_ENT_MAGIC 0xbeef

// HEF == HIST ENT FLAG
#define PG_HEF_BLK  0x00000001 // If not set, allowed
#define PG_HEF_TCP  0x00000002
#define PG_HEF_UDP  0x00000004
#define PG_HEF_ICMP 0x00000008
// If none of the above are set, then the transmission type is RAW
#define PG_HEF_IPv6 0x00000010 // If not set, IPv4
#define PG_HEF_RCV  0x00000020 // If not set, sending to remote

typedef struct pg_hist_file {
    pg_hist_hdr_t ph_hdr;
    pg_hist_ent_t ph_entries[0];
} pg_hist_file_t;

#pragma pack(0)

// Write: Open/Create the file and seek to EOF
// Read: Open file, validate and seek to beginning of entries (past the header)
// Returns open fd or -1 on error
int pg_hist_open(const char *path, int rdonly, u_int64_t max);
// Given a timestamp (UTC microseconds since epoch), returns the offset in the fd to the first inclusive entry.
// The internal fd offset is not changed.
int pg_hist_find_entry(int fd, u_int64_t timestamp, u_int64_t *offp);
// Returns 0 on no error, -1 for an unknown input file type, or > 0 if an error occurred
int pg_hist_byteswap(const char *in, const char *out);
// int pg_hist_upgrade(const char *in, const char *out);

#define PG_HIST_BLK_SIZE ((vm_page_size / sizeof(pg_hist_ent_t)) * sizeof(pg_hist_ent_t))
#define PG_HIST_FILE "~/Library/Caches/xxx.qNation.pghistory"
#define PG_HIST_FILE_DID_UPDATE "xxx.qNation.pg.hist.update"

#endif // PG_HIST_H
