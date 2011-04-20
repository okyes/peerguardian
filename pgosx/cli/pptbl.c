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

// Build: gcc -I pplib -I p2b pplib/pplib.c p2b/p2b.c pptbl.c -o pgtbl

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>

#define PG_MAKE_TABLE_ID
#include "p2b.h"
#include "pplib.h"

enum {
    op_bad = -1,
    op_list,
    op_add,
    op_rem,
    op_vrfy,
    op_portlist,
};

int main(int argc, char* argv[])
{
    int err, opt, op = op_bad, ipcount = 0;
    char *path, *name = NULL;
    pp_msg_iprange_t range = {0};
    
    char *progname = strrchr(argv[0], '/');
    if (progname)
        progname++;
    else
        progname = argv[0];
    
    while (-1 != (opt = getopt(argc, argv, "ahi:ln:p:rv"))) {
        switch (opt) {
            case 'a':
                op = op_add;
            break;
            
            case 'i':
                path = optarg;
                if (!ipcount)
                    range.p2_ipstart = ntohl(inet_addr(path));
                else if (1 == ipcount)
                    range.p2_ipend = ntohl(inet_addr(path));
                ipcount++;
            break;
            
            case 'l':
                op = op_list;
            break;
            
            case 'n':
                name = optarg;
            break;
            
            case 'p':
                op = op_portlist;
                path = optarg;
            break;

            case 'r':
                op = op_rem;
            break;
            
            case 'h':
                printf("%s OPTIONS path:\n"
                "\t-a: add an iprange -- requires -i and -n\n"
                "\t-i: an ip4 address in dotted quad form -- e.g 192.168.1.1\n"
                "\t-l: list table contents\n"
                "\t-h: this help\n"
                "\t-n: a range name/description -- required for range addition\n"
                "\t-r: remove an iprange requires -i\n"
                
                "\nNOTES:\n"
                "\t-i can be repeated twice (more than that are silently ignored).\n\tOrder does not matter. "
                "If only one address is specified, the range\n\tis just that one address.\n"
                "\n\tALWAYS WORK ON A COPY OF YOUR FILE!\n"
                , progname);
                return (0);
            break;
            
            case 'v':
                op = op_vrfy;
            break;
            
            case '?':
                exit(EINVAL);
            break;
        }
    }
    
    pp_msg_iprange_t *addrs;
    int32_t rangecount;
    u_int32_t addrcount;
    if (op == op_portlist) {
        if (!(err = p2b_parseports(path, strlen(path), &addrs, &rangecount)))
            free(addrs);
        return (err);
    }
    
    if (optind >= argc) {
        errno = EINVAL;
        perror("table file not specified");
        exit(errno);
    }
    if (op == op_bad) {
        errno = EINVAL;
        perror("operation not specified");
        exit(errno);
    }
    if (ipcount) {
        if (INADDR_NONE == range.p2_ipstart || INADDR_NONE == range.p2_ipend) {
            errno = EINVAL;
            perror("invalid address");
            exit(errno);
        }
        if (!range.p2_ipend)
            range.p2_ipend = range.p2_ipstart;
        if (range.p2_ipend < range.p2_ipstart) {
            u_int32_t tmp = range.p2_ipstart;
            range.p2_ipstart = range.p2_ipend;
            range.p2_ipend = tmp;
        }
    }
    if (op_list != op && op_vrfy != op && (!ipcount || (1 == ipcount && !range.p2_ipstart))) {
        errno = EINVAL;
        perror("address not specified, or it's invalid");
        exit(errno);
    }
    if (op_add == op && !name) {
        errno = EINVAL;
        perror("range addition requires a name");
        exit(errno);
    }
    path = argv[optind];
    
    p2b_handle_t h = NULL;
    u_int8_t* *names;
    int version = P2P_VERSION;
    switch (op) {
        case op_add:
            if (0 == (err = p2b_open(path, version, &h))) {
                err = p2b_add(h, name, &range);
                if (err)
                    fprintf(stderr, "addition failed: %d\n", err);
            }
        break;
        
        case op_list:
            if (!(err = p2b_parse(path, &addrs, &rangecount, &addrcount, &names))) {
                pp_print_table(addrs, rangecount, names);
                printf("%d total entries (%u)\n", rangecount, addrcount);
            } else
                fprintf(stderr, "table parse failed: %d\n", err);
        break;
        
        case op_rem:
            if (0 == (err = p2b_open(path, version, &h))) {
                err = p2b_remove(h, NULL, &range);
                if (err)
                    fprintf(stderr, "removal failed: %d\n", err);
            }
        break;
        
        case op_vrfy:
            if (!(err = p2b_parse(path, &addrs, &rangecount, NULL, &names))) {
                u_int32_t addr;
                int i;
                struct sockaddr_in sa;
            #ifndef linux
                sa.sin_len = sizeof(sa);
            #endif
                sa.sin_family = AF_INET;
                sa.sin_port = htons(2081);
                
                int so = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                
                for (i=0; i < rangecount; ++i) {
                    for (addr = addrs[i].p2_ipstart; addr <= addrs[i].p2_ipend; ++addr) {
                        sa.sin_addr.s_addr = htonl(addr);
                    #if 1
                        if (0 == connect(so, (struct sockaddr*)&sa, sizeof(sa)) || ECONNREFUSED != errno) {
                            shutdown(so, SHUT_RDWR);
                    #else
                        if (!pp_match_addr(addrs, rangecount, addr)) {
                    #endif
                            printf("failed to match %s:%x\n", names[addrs[i].p2_name_idx], addr);
                            exit(0);
                        }
                    }
                    fprintf(stdout, "%d,", (rangecount-i));
                    fflush(stdout);
                }
                close(so);
                printf("\n");
            }
        break;
        
        default:
            err = EINVAL;
        break;
    }
    
    if (h)
        p2b_close(h);
    
    return (err);
}
