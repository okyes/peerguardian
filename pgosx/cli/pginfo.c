/*
    Copyright 2006,2007 Brian Bergstrand

    This program is free software; you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program;
    if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/errno.h>

#define PG_MAKE_TABLE_ID
#include "pplib.h"

static const char whatid[] __attribute__ ((used)) =
"@(#)$LastChangedRevision: 589 $ Built: " __DATE__ ", " __TIME__;

static int print_stats()
{
    pp_msg_stats_t s;
    int err = pp_statistics(&s);
    if (0 == err) {
        printf("PeerGuardian Statistics\n"
            "\tBlocked Addrs: %u\n"
            "\tClients      : %u\n"
            "\tBlocked      : I:%5u, O:%5u (%6qu)\n"
            "\tAllowed      : I:%5u, O:%5u (%6qu)\n"
            "\tLog overflows: %u\n"
            "\tFilter miss  : %u\n"
            "\tLoad stalls  : %u\n"
            "\tLoad failures: %u\n"
            "\tConns / sec  : %u\n"
            "\tBlocks / sec : %u\n",
            s.pps_blocked_addrs, s.pps_userclients,
            s.pps_in_blocked, s.pps_out_blocked, (u_int64_t)((u_int64_t)s.pps_in_blocked + (u_int64_t)s.pps_out_blocked),
            s.pps_in_allowed, s.pps_out_allowed, (u_int64_t)((u_int64_t)s.pps_in_allowed + (u_int64_t)s.pps_out_allowed),
            s.pps_droppedlogs, s.pps_filter_ignored, s.pps_table_loadwait, s.pps_table_loadfail,
            s.pps_conns_per_sec, s.pps_blcks_per_sec);
    }
    return (err);
}

static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", 0};

static int print_tables()
{
    pp_msg_table_info_t tables[128];
    
    int count = 128;
    int err = pp_tableinfo(tables, &count);
    if (0 == err) {
        int i;
        pp_name_map_t mh;
        char buf[64];
        char *name;
        u_int32_t ranges = 0, mem = 0;
        u_int32_t addrs = 0ULL;
        char moniker;
        
        printf("ID%34c Addresses  Ranges  Allow Memory  Load Time%11c Name\n\n", ' ', ' ');
        for (i=0; i < count; ++i) {
            name = NULL;
            mh = NULL;
            if (0 == pg_table_id_cmp(PG_TABLE_ID_PORTS, tables[i].tableid)) {
                name = "Port Rules";
            } else if (0 == pp_map_names(tables[i].tableid, &mh)) {
                (void)pp_map_name(mh, &name);
            }
            if (!name) {
                name = "Unknown";
            }
            
            ranges += tables[i].rangect;
            addrs += (typeof(addrs))tables[i].addrct;
            mem += tables[i].size;
            
            if (tables[i].size >= 1024) {
                moniker = 'k';
                tables[i].size >>= 10;
            } else
                moniker = 'b';
            
            char flag;
            if (tables[i].flags & PP_LOAD_ALWYS_PASS)
                flag = 'A';
            else if (0 == (tables[i].flags & PP_LOAD_BLOCK_STD))
                flag = 'S';
            else
                flag = '-';
            
            struct tm tm;
            time_t secs = (time_t)tables[i].loadtime;
            (void)localtime_r(&secs, &tm);
            char timestr[36];
            snprintf(timestr, sizeof(timestr)-1, "%s %2d %d %02d:%02d:%02d",
                months[tm.tm_mon], tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
            timestr[sizeof(timestr)-1] = 0;
            
            printf("%s %10u %7u %5c %6u%c %s %s\n",
                pg_table_id_string(tables[i].tableid, buf, sizeof(buf), 1),
                tables[i].addrct, tables[i].rangect,
                flag, tables[i].size, moniker, timestr, name);
            
            if (mh) {
                (void)pp_unmap_names(mh);
                free(name);
            }
        }
        
        if (mem >= 1024) {
            moniker = 'k';
            mem >>= 10;
        } else
            moniker = 'b';
        
        printf("All%33c %10u %7u %5c %6u%c\n", ' ', addrs, ranges, ' ', mem, moniker);
    }
    return (err);
}

int main(int argc, char* argv[])
{
    int err, opt;
    char *outfile = NULL;
    int version = -1;
    
    char *progname = strrchr(argv[0], '/');
    if (progname)
        progname++;
    else
        progname = argv[0];
    
    if (argc == 1)
        goto print_help;
    
    while (-1 != (opt = getopt(argc, argv, "hst"))) {
        switch (opt) {
            case 's':
                if ((err = print_stats()))
                    fprintf(stderr, "statistics failed with: %d\n", err);
            break;

            case 't':
                if ((err = print_tables()))
                    fprintf(stderr, "tables failed with: %d\n", err);
            break;
            
            case 'h':
    print_help:
                printf("%s options\n"
                "\t-s Print kernel statistics.\n"
                "\t-t Print kernel table info.\n"
                "\t-h This help.\n"
                , progname);
                return (0);
            break;
            
            case '?':
                goto print_help;
            break;
        }
    }
    return (0);
}
