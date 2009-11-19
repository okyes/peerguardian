/*
   Netfilter blocking daemon

   (c) 2008 Jindrich Makovicka (makovick@gmail.com)

   Portions (c) 2004 Morpheus (ebutera@users.berlios.de)

   This file is part of pgl.

   pgl is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   pgl is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Emacs; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef PGLD_H
#define PGLD_H

#include <stdlib.h>
#include <inttypes.h>

#define RECVBUFFSIZE 1500
#define PAYLOADSIZE 24
#define ICMP    1
#define TCP     6
#define UDP     17
#define MAX_RANGES 16

#define NIPQUAD(addr) \
((unsigned char *)&addr)[0], \
((unsigned char *)&addr)[1], \
((unsigned char *)&addr)[2], \
((unsigned char *)&addr)[3]

#define NIPQUADREV(addr) \
((unsigned char *)&addr)[3], \
((unsigned char *)&addr)[2], \
((unsigned char *)&addr)[1], \
((unsigned char *)&addr)[0]

#define SRC_ADDR(payload) (*(in_addr_t *)((payload)+12))
#define DST_ADDR(payload) (*(in_addr_t *)((payload)+16))

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define CHECK_OOM(ptr)                                                  \
    do {                                                                \
        if (!ptr) {                                                     \
            do_log(LOG_CRIT, "Out of memory in %s (%s:%d)",             \
                   __func__, __FILE__, __LINE__);                       \
            exit (-1);                                                  \
        }                                                               \
    } while(0);                                                         \

#endif

#define GETIPINFO   if (ip->protocol == TCP) {\
    strcpy(proto, "TCP");\
    tcp     = (struct tcphdr*) (payload + (4 * ip->ihl));\
    sprintf(ip_src, "%u.%u.%u.%u:%u",NIPQUAD(ip->saddr),ntohs(tcp->source));\
    sprintf(ip_dst, "%u.%u.%u.%u:%u",NIPQUAD(ip->daddr),ntohs(tcp->dest));\
} else if (ip->protocol == UDP) {\
    strcpy(proto, "UDP");\
    udp     = (struct udphdr*) (payload + (4 * ip->ihl));\
    sprintf(ip_src, "%u.%u.%u.%u:%u",NIPQUAD(ip->saddr),ntohs(udp->source));\
    sprintf(ip_dst, "%u.%u.%u.%u:%u",NIPQUAD(ip->daddr),ntohs(udp->dest));\
} else if (ip->protocol == ICMP) {\
    strcpy(proto, "ICMP");\
    sprintf(ip_src, "%u.%u.%u.%u",NIPQUAD(ip->saddr));\
    sprintf(ip_dst, "%u.%u.%u.%u",NIPQUAD(ip->daddr));\
} else {\
    sprintf(proto, "%d", ip->protocol);\
    sprintf(ip_src, "%u.%u.%u.%u",NIPQUAD(ip->saddr));\
    sprintf(ip_dst, "%u.%u.%u.%u",NIPQUAD(ip->daddr));\
}

void do_log(int priority, const char *format, ...);
int opt_verbose;
