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
#include <Carbon/Carbon.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "pplib.h"

static int logfd = -1;

#define pp_log_entry_cmp(e1, e2) ( \
((e1)->pplg_flags == (e2)->pplg_flags) && \
(0 == pg_table_id_cmp((e1)->pplg_tableid, (e2)->pplg_tableid)) && \
((e1)->pplg_name_idx == (e2)->pplg_name_idx) && \
((e1)->pplg_pid == (e2)->pplg_pid) && \
((e1)->pplg_fromaddr == (e2)->pplg_fromaddr) && \
((e1)->pplg_toaddr == (e2)->pplg_toaddr) && \
((e1)->pplg_fromport == (e2)->pplg_fromport) && \
((e1)->pplg_toport == (e2)->pplg_toport) \
)

#define pp_log_entry_cmp_dbg(e1, e2) ( \
((e1)->pplg_fromaddr == (e2)->pplg_fromaddr) && \
((e1)->pplg_toaddr == (e2)->pplg_toaddr) && \
((e1)->pplg_fromport == (e2)->pplg_fromport) && \
((e1)->pplg_toport == (e2)->pplg_toport) \
)

static inline
unsigned int ExchangeAtomic(register unsigned int newVal, unsigned int volatile *addr)
{
    register unsigned int val;
    do {
        val = *addr;
    } while (val != newVal && FALSE == CompareAndSwap(val, newVal, (UInt32*)addr));
    return (val);
}

#define PP_REPEAT_INTERVAL 5

static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", 0};
static const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", 0};
void printlogentry(const pp_log_entry_t *ep, const char *entryName, const char *tableName)
{
    static pp_log_entry_t lastEvent = {0};
    static unsigned int lastRepeatTime __attribute__ ((aligned (4))) = 0xffffffff - PP_REPEAT_INTERVAL;
    static unsigned int eventRepeatCount __attribute__ ((aligned (4))) = 0;
    #if 0
    static pthread_mutex_t eventRepeatLock = PTHREAD_MUTEX_INITIALIZER;
    #define erlock() pthread_mutex_lock(&eventRepeatLock)
    #define erulock() pthread_mutex_unlock(&eventRepeatLock)
    #endif
    
    time_t secs = (time_t)(ep->pplg_timestamp / 1000000ULL);
    unsigned int lastVal = lastRepeatTime + PP_REPEAT_INTERVAL;
    if (lastVal > secs && pp_log_entry_cmp(&lastEvent, ep)) {
        (void)ExchangeAtomic((unsigned int)secs, &lastRepeatTime);
        (void)IncrementAtomic((SInt32*)&eventRepeatCount);
        return;
    } else {
        // XXX - This should be done under a lock,
        // but we know that pplib will only call us from a single thread
        if (pp_log_entry_cmp_dbg(ep, &lastEvent))
            eventRepeatCount = 0;
        
        bcopy(ep, &lastEvent, sizeof(*ep));
        (void)ExchangeAtomic(0xffffffff - PP_REPEAT_INTERVAL, &lastRepeatTime);
    }
    
    int msecs = ((int)(ep->pplg_timestamp % 1000000ULL)) / 1000;
    
    char *type;
    if (ep->pplg_flags & PP_LOG_ALLOWED)
        type = "Allw";
    else if (ep->pplg_flags & PP_LOG_BLOCKED)
        type = "Blck";
    else
        type = "????";
        
    char *protocol;
    if (ep->pplg_flags & PP_LOG_TCP)
        protocol = (ep->pplg_flags & PP_LOG_IP6) ? "TCP6" : "TCP ";
    else if (ep->pplg_flags & PP_LOG_UDP)
        protocol = (ep->pplg_flags & PP_LOG_IP6) ? "UDP6" : "UDP ";
    else if (ep->pplg_flags & PP_LOG_ICMP)
        protocol = (ep->pplg_flags & PP_LOG_IP6) ? "ICMP6" : "ICMP";
    else
        protocol = "????";
    
    struct in_addr to, from;
    to.s_addr = ep->pplg_toaddr; // log addrs are in network order
    from.s_addr = ep->pplg_fromaddr;
    char fromstr[64], tostr[64];
    if (from.s_addr)
        (void)pp_inet_ntoa(from, fromstr, sizeof(fromstr)-1);
    else
        strcpy(fromstr, "local");
    if (to.s_addr)
        (void)pp_inet_ntoa(to, tostr, sizeof(tostr)-1);
    else
        strcpy(tostr, "local");
    fromstr[sizeof(fromstr)-1] = tostr[sizeof(tostr)-1] = 0;
    
    struct tm tm;
    (void)localtime_r(&secs, &tm);
    char timestr[128];
    snprintf(timestr, sizeof(timestr)-1, "%s %s %2d %02d:%02d:%02d.%-3d %d", days[tm.tm_wday],
        months[tm.tm_mon], tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
        msecs, tm.tm_year + 1900);
    timestr[sizeof(timestr)-1] = 0;
    
    char nameStr[512];
    if (ep->pplg_flags & PP_LOG_FLTR_STALL) {
        strcpy(nameStr, "PP Filter Stall");
    } else if (PP_TABLE_ID_ALL == ep->pplg_tableid) {
        // Could have been an error getting the tableName, but we'll just assume this
        // was not in any table.
        nameStr[0] = 0;
    } else {
        if (!tableName && !entryName) {
            char namebuf[64];
            snprintf(nameStr, sizeof(nameStr)-1, "(%s:%d)",
                pg_table_id_string(ep->pplg_tableid, namebuf, sizeof(namebuf), 1),
                ep->pplg_name_idx);
        } else {
            // entryName may be NULL, but tableName will be valid
            if (entryName)
                snprintf(nameStr, sizeof(nameStr)-1, "(%s:%s)", entryName, tableName);
            else
                snprintf(nameStr, sizeof(nameStr)-1, "(%u:%s)", ep->pplg_name_idx, tableName);
        }
        nameStr[sizeof(nameStr)-1] = 0;
    }
    
    char buf[1536];
    snprintf(buf, sizeof(buf)-1, "%s -%s- %s:%d -> %s:%d %s %s\n", timestr, type, fromstr,
        ep->pplg_fromport, tostr, ep->pplg_toport, protocol, nameStr);
    buf[sizeof(buf)-1] = 0;
    if (-1 != logfd)
        write(logfd, buf, strlen(buf));
    else
        printf(buf);
}

int main(int argc, char* argv[])
{
    int err = 0, allowed = 0, opt;
    
    while (-1 != (opt = getopt(argc, argv, "af:s"))) {
        switch (opt) {
            case 'a':
                allowed = 1;
            break;

            case 'f': {
                char *path = optarg;
                logfd = open(path, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
                if (-1 != logfd) {
                    daemon(0, 0);
                } else {
                    perror("failed to open log file");
                    exit(errno);
                }
            } break;

            case 's': {
                pp_msg_stats_t s;
                err = pp_statistics(&s);
                if (!err) {
                    printf("PeerProtector Statistics\n"
                        "\tBlocked Addresses: %u\n"
                        "\tClients: %u\n"
                        "\tBlocked: I:%5u, O:%5u (%6qu)\n"
                        "\tAllowed: I:%5u, O:%5u (%6qu)\n"
                        "\tLog overflows: %u\n"
                        "\tFilter miss  : %u\n"
                        "\tLoad stalls  : %u\n"
                        "\tLoad failures  : %u\n",
                        s.pps_blocked_addrs, s.pps_userclients,
                        s.pps_in_blocked, s.pps_out_blocked, (u_int64_t)((u_int64_t)s.pps_in_blocked + (u_int64_t)s.pps_out_blocked),
                        s.pps_in_allowed, s.pps_out_allowed, (u_int64_t)((u_int64_t)s.pps_in_allowed + (u_int64_t)s.pps_out_allowed),
                        s.pps_droppedlogs, s.pps_filter_ignored, s.pps_table_loadwait, s.pps_table_loadfail);
                }
                return (err);
            } break;

            case '?':
                exit(EINVAL);
            break;
        }
    }
    
    err = pp_log_listen(allowed, printlogentry, NULL, NULL);
    
    return (err);
}
    