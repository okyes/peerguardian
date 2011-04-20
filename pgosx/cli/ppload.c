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
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>

#include "p2b.h"
#include "pplib.h"

enum {
    op_load,
    op_list,
    op_unload,
    op_match,
}; 

int main(int argc, char* argv[])
{
    int err, opt;
    int loadflags = 0, op = op_load;
    u_int32_t tableid = PP_TABLE_ID_ALL;
    char *path = NULL;
    in_addr_t ipmatch = 0;
    
    while (-1 != (opt = getopt(argc, argv, "abi:lm:u"))) {
        switch (opt) {
            case 'a':
                loadflags |= PP_LOAD_ALWYS_PASS;
            break;
            
            case 'b':
                loadflags |= PP_LOAD_BLOCK_STD;
            break;

            case 'i':
                tableid = strtoul(optarg, NULL, 0);
                if (!tableid && errno) {
                    perror("invalid table id");
                    exit(errno);
                }
            break;

            case 'l':
                op = op_list;
            break;
            
            case 'm':
                op = op_match;
                path = optarg;
                ipmatch = ntohl(inet_addr(path));
                if (INADDR_NONE == ipmatch)
                    exit(EINVAL);
            break;
            
            case 'u':
                op = op_unload;
            break;

            case '?':
                exit(EINVAL);
            break;
        }
    }
    if (optind < argc)
        path = argv[optind];
    else if (op_unload != op) {
        errno = EINVAL;
        perror("table file not specified");
        exit(errno);
    }
    if (PP_TABLE_ID_ALL == tableid && op_load == op) {
        errno = EINVAL;
        perror("table id not specified");
        exit(errno);
    }
    
    if (op_unload == op) {
        // remove shared names
        return (pp_unload_table(PP_TABLE_ID_ALL));
    }
    
    pp_msg_iprange_t *addrs;
    u_int8_t **names;
    int32_t rangecount;
    u_int32_t addrcount;
    if (!(err = p2b_parse(path, &addrs, &rangecount, &addrcount, &names))) {
        if (op_match == op) {
            const pp_msg_iprange_t *range = pp_match_addr(addrs, rangecount, ipmatch);
            if (range)
                printf("matched: %s (%d)\n", (char*)names[range->p2_name_idx], range->p2_name_idx);
            else
                err = ENOENT;
            return(err);
        }
        
        if (op_list == op)
            pp_print_table(addrs, rangecount, names);
        printf("%d total entries\n", rangecount);
        
        if (PP_TABLE_ID_ALL == tableid)
            return(0);
        
        char *tblname = strrchr(path, '/');
        if (tblname)
            tblname++;
        else
            tblname = path;
        
        char tname[32];
        strncpy(tname, tblname, sizeof(tname)-1);
        tname[sizeof(tname)-1] = 0;
        tblname = tname;
        (void)strsep(&tblname, ".");
        tblname = tname;
        
        // load the names into shared mem
        if ((err = pp_load_names(tableid, tblname, names))) {
            fprintf(stderr, "failed to load names for table %s,%u: %d\n", tblname, tableid, err);
            err = 0; // not fatal
        }
        
        // load them into the kernel
        err = pp_load_table(addrs, rangecount, addrcount, tableid, loadflags);
        
        free(addrs);
        free(names);
    } else { /// p2b_parse
        fprintf(stderr, "table parse failed: %d\n", err);
    }
    
    return (err);
}
