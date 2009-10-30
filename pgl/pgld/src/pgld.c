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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <syslog.h>
#include <inttypes.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <linux/netfilter_ipv4.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <poll.h>

#ifdef HAVE_DBUS
#include <dlfcn.h>
#include "dbus.h"
#endif

#include "blocklist.h"
#include "parser.h"
#include "pgld.h"

// typedef enum {
//     CMD_NONE,
//     CMD_DUMPSTATS,
//     CMD_RELOAD,
//     CMD_QUIT,
// } command_t;

static blocklist_t blocklist;

static int opt_daemon = 1, daemonized = 0;
int opt_verbose = 0;
static int queue_num = 0;
static int use_syslog = 0;
static uint32_t accept_mark = 0, reject_mark = 0;
static const char *pidfile_name = "/var/run/pgld.pid";
static FILE *logfile;
static char *logfile_name=NULL;

static const char *current_charset = 0;

static int blockfile_count = 0;
static const char **blocklist_filenames = 0;
static const char **blocklist_charsets = 0;

#ifdef HAVE_DBUS
static int use_dbus = 0;
#endif

// static volatile command_t command = CMD_NONE;
// static time_t curtime = 0;
static FILE* pidfile = NULL;

struct nfq_handle *nfqueue_h = 0;
struct nfq_q_handle *nfqueue_qh = 0;

void
do_log(int priority, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

    if (use_syslog) {
    vsyslog(LOG_MAKEPRI(LOG_DAEMON, priority), format, ap);
    }

    if (logfile) {
        char timestr[17];
        time_t tv;
        struct tm * timeinfo;
        time( &tv );
        timeinfo = localtime ( &tv );
        strftime(timestr, 17, "%b %e %X", timeinfo);
        timestr[16] = '\0';
        fprintf(logfile,"%s ",timestr);
        vfprintf(logfile, format, ap);
        fprintf(logfile, "\n");
        fflush(logfile);
    }
// #ifdef HAVE_DBUS
//     if (use_dbus) {
//         pgl_dbus_send(format, ap);
//     }
// #endif

    va_end(ap);
}


/*#ifdef HAVE_DBUS
static void *dbus_lh = NULL;

// static pgl_dbus_init_t pgl_dbus_init = NULL;
// static pgl_dbus_send_blocked_t pgl_dbus_send_blocked = NULL;

#define do_dlsym(symbol)                                                \
    do {                                                                \
        symbol = dlsym(dbus_lh, # symbol);                              \
        err = dlerror();                                                \
        if (err) {                                                      \
            do_log(LOG_ERR, "Cannot get symbol %s: %s", # symbol, err); \
            goto out_err;                                               \
        }                                                               \
    } while (0)

static int
open_dbus()
{
    char *err;

    dbus_lh = dlopen(PLUGINDIR "/dbus.so", RTLD_NOW);
    if (!dbus_lh) {
        do_log(LOG_ERR, "dlopen() failed: %s", dlerror());
        return -1;
    }
    dlerror(); // clear the error flag

    do_dlsym(pgl_dbus_init);
    do_dlsym(pgl_dbus_send);

    return 0;

out_err:
    dlclose(dbus_lh);
    dbus_lh = 0;
    return -1;
}

static int
close_dbus()
{
    int ret = 0;

    if (dbus_lh) {
        ret = dlclose(dbus_lh);
        dbus_lh = 0;
    }

    return ret;
}

#endif*/


static int
load_all_lists()
{
    int i, ret = 0;

    blocklist_clear(&blocklist, 0);
    for (i = 0; i < blockfile_count; i++) {
        if (load_list(&blocklist, blocklist_filenames[i], blocklist_charsets[i])) {
            do_log(LOG_ERR, "Error loading %s", blocklist_filenames[i]);
            ret = -1;
        }
    }
    blocklist_sort(&blocklist);
    blocklist_trim(&blocklist);
    return ret;
}

static int
nfqueue_cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
           struct nfq_data *nfa, void *data)
{
    int id = 0, status = 0;
    struct nfqnl_msg_packet_hdr *ph;
    block_entry_t *src, *dst;
    //uint32_t ip_src, ip_dst;
    struct iphdr *ip;
    struct udphdr *udp;
    struct tcphdr *tcp;
    char *payload, proto[5], ip_src[23], ip_dst[23];
#ifndef LOWMEM
    block_sub_entry_t *sranges[MAX_RANGES + 1], *dranges[MAX_RANGES + 1];
#else
    /* dummy variables */
    static void *sranges = 0, *dranges = 0;
#endif

    ph = nfq_get_msg_packet_hdr(nfa);
    if (ph) {
        id = ntohl(ph->packet_id);
        nfq_get_payload(nfa, &payload);
        ip = (struct iphdr*) payload;
        switch (ph->hook) {
        case NF_IP_LOCAL_IN:
            src = blocklist_find(&blocklist, ntohl(ip->saddr), sranges, MAX_RANGES);
            if (src) {
                status = nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
                src->hits++;
                    GETIPINFO
#ifndef LOWMEM
                    do_log(LOG_NOTICE, " IN: %-22s %-22s %-4s || %s",ip_src,ip_dst,proto,sranges[0]->name);
#else
                    do_log(LOG_NOTICE, " IN: %-22s %-22s %-4s",ip_src,ip_dst,proto);
#endif
            } else if (unlikely(accept_mark)) {
                // we set the user-defined accept_mark and set NF_REPEAT verdict
                // it's up to other iptables rules to decide what to do with this marked packet
                status = nfq_set_verdict_mark(qh, id, NF_REPEAT, accept_mark, 0, NULL);
            } else {
                // no accept_mark, just NF_ACCEPT the packet
                status = nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
            }
            break;
        case NF_IP_LOCAL_OUT:

            dst = blocklist_find(&blocklist, ntohl(ip->daddr), dranges, MAX_RANGES);
            if (dst) {
                if (likely(reject_mark)) {
                    // we set the user-defined reject_mark and set NF_REPEAT verdict
                    // it's up to other iptables rules to decide what to do with this marked packet
                    status = nfq_set_verdict_mark(qh, id, NF_REPEAT, reject_mark, 0, NULL);
                } else {
                    status = nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
                }
                dst->hits++;
                        GETIPINFO
#ifndef LOWMEM
                        do_log(LOG_NOTICE, "OUT: %-22s %-22s %-4s || %s",ip_src,ip_dst,proto,dranges[0]->name);
#else
                        do_log(LOG_NOTICE, "OUT: %-22s %-22s %-4su", ip_src,ip_dst,proto);
#endif
            } else if (unlikely(accept_mark)) {
                // we set the user-defined accept_mark and set NF_REPEAT verdict
                // it's up to other iptables rules to decide what to do with this marked packet
                status = nfq_set_verdict_mark(qh, id, NF_REPEAT, accept_mark, 0, NULL);
            } else {
                // no accept_mark, just NF_ACCEPT the packet
                status = nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
            }
            break;
        case NF_IP_FORWARD:
            src = blocklist_find(&blocklist, ntohl(ip->saddr), sranges, MAX_RANGES);
            dst = blocklist_find(&blocklist, ntohl(ip->daddr), dranges, MAX_RANGES);
            if (dst || src) {
                if (likely(reject_mark)) {
                    // we set the user-defined reject_mark and set NF_REPEAT verdict
                    // it's up to other iptables rules to decide what to do with this marked packet
                    status = nfq_set_verdict_mark(qh, id, NF_REPEAT, reject_mark, 0, NULL);
                } else {
                    status = nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
                }
                if (src) {
                    src->hits++;
                }
                if (dst) {
                    dst->hits++;
                }
                        GETIPINFO
#ifndef LOWMEM
                        do_log(LOG_NOTICE, "FWD: %-22s %-22s %-4s || %s",ip_src,ip_dst,proto,src ? sranges[0]->name : dranges[0]->name);
#else
                        do_log(LOG_NOTICE, "FWD: %-22s %-22s %-4s",ip_src,ip_dst,proto);
#endif
            } else if ( unlikely(accept_mark) ) {
                // we set the user-defined accept_mark and set NF_REPEAT verdict
                // it's up to other iptables rules to decide what to do with this marked packet
                status = nfq_set_verdict_mark(qh, id, NF_REPEAT, accept_mark, 0, NULL);
            } else {
                // no accept_mark, just NF_ACCEPT the packet
                status = nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
            }
            break;
        default:
            do_log(LOG_NOTICE, "Not NF_LOCAL_IN/OUT/FORWARD packet!");
            break;
        }
    } else {
        do_log(LOG_ERR, "NFQUEUE: can't get msg packet header.");
        return 1;               // from nfqueue source: 0 = ok, >0 = soft error, <0 hard error
    }
    return 0;
}

static int
nfqueue_bind()
{
    nfqueue_h = nfq_open();
    if (!nfqueue_h) {
        do_log(LOG_ERR, "Error during nfq_open(): %s", strerror(errno));
        return -1;
    }

    if (nfq_unbind_pf(nfqueue_h, AF_INET) < 0) {
        do_log(LOG_ERR, "Error during nfq_unbind_pf(): %s", strerror(errno));
        nfq_close(nfqueue_h);
        return -1;
    }

    if (nfq_bind_pf(nfqueue_h, AF_INET) < 0) {
        do_log(LOG_ERR, "Error during nfq_bind_pf(): %s", strerror(errno));
        nfq_close(nfqueue_h);
        return -1;
    }

    do_log(LOG_INFO, "NFQUEUE: binding to queue %d", queue_num);
    nfqueue_qh = nfq_create_queue(nfqueue_h, queue_num, &nfqueue_cb, NULL);
    if (!nfqueue_qh) {
        do_log(LOG_ERR, "Error during nfq_create_queue(): %s", strerror(errno));
        nfq_close(nfqueue_h);
        return -1;
    }

    if (nfq_set_mode(nfqueue_qh, NFQNL_COPY_PACKET, PAYLOADSIZE) < 0) {
        do_log(LOG_ERR, "Can't set packet_copy mode: %s", strerror(errno));
        nfq_destroy_queue(nfqueue_qh);
        nfq_close(nfqueue_h);
        return -1;
    }
    return 0;
}

static void
nfqueue_unbind()
{
    if (!nfqueue_h)
        return;

    do_log(LOG_INFO, "NFQUEUE: unbinding from queue 0");
    nfq_destroy_queue(nfqueue_qh);
    if (nfq_unbind_pf(nfqueue_h, AF_INET) < 0) {
        do_log(LOG_ERR, "Error during nfq_unbind_pf(): %s", strerror(errno));
    }
    nfq_close(nfqueue_h);
}

static int
nfqueue_loop ()
{
    struct nfnl_handle *nh;
    int fd, rv;
    char buf[RECVBUFFSIZE];
    struct pollfd fds[1];

    if (nfqueue_bind() < 0)
        return -1;

    nh = nfq_nfnlh(nfqueue_h);
    fd = nfnl_fd(nh);

    for (;;) {
        fds[0].fd = fd;
        fds[0].events = POLLIN;
        fds[0].revents = 0;
        rv = poll(fds, 1, 5000);

//         curtime = time(NULL);

        if (rv < 0) {
            if (errno == EINTR)
                continue;
            do_log(LOG_ERR, "Error waiting for socket: %s", strerror(errno));
            goto out;
        }
        if (rv > 0) {
            rv = recv(fd, buf, sizeof(buf), 0);
            if (rv < 0) {
                if (errno == EINTR)
                    continue;
                do_log(LOG_ERR, "Error reading from socket: %s", strerror(errno));
                goto out;
            }
            if (rv >= 0)
                nfq_handle_packet(nfqueue_h, buf, rv);
        }

    }
out:
    nfqueue_unbind();
    return 0;
}

static void
sighandler(int sig, siginfo_t *info, void *context)
{
    switch (sig) {
    case SIGUSR1:
        // dump and reset stats
        blocklist_stats(&blocklist, 1);
        break;
    case SIGUSR2:
        // just dump stats
        blocklist_stats(&blocklist, 0);
        break;
    case SIGHUP:
        blocklist_stats(&blocklist, 1);
        if (load_all_lists() < 0)
            do_log(LOG_ERR, "Cannot load the blocklist");
        do_log(LOG_INFO, "Blocklist reloaded");
        break;
    case SIGTERM:
    case SIGINT:
        nfqueue_unbind();
        blocklist_stats(&blocklist, 0);

// #ifdef HAVE_DBUS
//         if (use_dbus)
//             close_dbus();
// #endif
        blocklist_clear(&blocklist, 0);
        free(blocklist_filenames);
        free(blocklist_charsets);
        closelog();
        if (pidfile) {
            fclose(pidfile);
            unlink(pidfile_name);
        }
        exit(0);
        break;
    case SIGSEGV:
        nfqueue_unbind();
        abort();
        break;
    default:
        break;
    }
}

static int
install_sighandler()
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = sighandler;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("Error setting signal handler for SIGUSR1\n");
        return -1;
    }
    if (sigaction(SIGUSR2, &sa, NULL) < 0) {
        perror("Error setting signal handler for SIGUSR2\n");
        return -1;
    }
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        perror("Error setting signal handler for SIGHUP\n");
        return -1;
    }
    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        perror("Error setting signal handler for SIGTERM\n");
        return -1;
    }
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("Error setting signal handler for SIGINT\n");
        return -1;
    }
    if (sigaction(SIGSEGV, &sa, NULL) < 0) {
        perror("Error setting signal handler for SIGABRT\n");
        return -1;
    }
    return 0;
}

static FILE *
create_pidfile(const char *name)
{
    FILE *f;

    f = fopen(name, "w");
    if (f == NULL){
        fprintf(stderr, "Unable to create PID file %s: %s\n", name, strerror(errno));
        return NULL;
    }

    /* this works even if pidfile is stale after daemon is sigkilled */
    if (lockf(fileno(f), F_TLOCK, 0) == -1){
        fprintf(stderr, "Unable to set exclusive lock for pidfile %s: %s\n", name, strerror(errno));
        return NULL;
    }

    fprintf(f, "%d\n", getpid());
    fflush(f);

    /* leave fd open as long as daemon is running */
    /* this is useful for example so that inotify can catch a file
     * closed event even if daemon is killed */
    return f;
}


static void
daemonize() {
    /* Fork off and have parent exit. */
    switch (fork()) {
    case -1:
        perror("fork");
        exit(1);

    case 0:
        break;

    default:
        exit(0);
    }

    /* detach from the controlling terminal */

    setsid();

    close(fileno(stdin));
    close(fileno(stdout));
    close(fileno(stderr));
    daemonized = 1;
}

static void
print_usage()
{
    fprintf(stderr, PKGNAME " " VERSION " (c) 2008 Jindrich Makovicka\n");
    fprintf(stderr, "Syntax: pgld [-d] [-s] [-v] [-a MARK] [-r MARK] [-q 0-65535] BLOCKLIST...\n\n");
//     fprintf(stderr, "        -d            Run in foreground in debug mode\n");
#ifndef LOWMEM
    fprintf(stderr, "        -c            Blocklist file charset (for all following filenames)\n");
#endif
    fprintf(stderr, "        -f            Blocklist file name\n");
    fprintf(stderr, "        -p NAME       Use a pidfile named NAME\n");
    fprintf(stderr, "        -v            Verbose output\n");
//     fprintf(stderr, "        -b            Benchmark IP matches per second\n");
    fprintf(stderr, "        -q 0-65535    NFQUEUE number, as specified in --queue-num with iptables\n");
    fprintf(stderr, "        -a MARK       32-bit mark to place on ACCEPTED packets\n");
    fprintf(stderr, "        -r MARK       32-bit mark to place on REJECTED packets\n");
    fprintf(stderr, "        -l LOGFILE    Log to LOGFILE (Default: " LOGDIR "/pgld.log)\n");
    fprintf(stderr, "        -s            Enable logging to the system log\n");
#ifdef HAVE_DBUS
    fprintf(stderr, "        -d            Enable D-Bus support\n");
#endif
    fprintf(stderr, "\n");
}

void
add_blocklist(const char *name, const char *charset)
{
    blocklist_filenames = (const char**)realloc(blocklist_filenames, sizeof(const char*) * (blockfile_count + 1));
    CHECK_OOM(blocklist_filenames);
    blocklist_charsets = (const char**)realloc(blocklist_charsets, sizeof(const char*) * (blockfile_count + 1));
    CHECK_OOM(blocklist_charsets);
    blocklist_filenames[blockfile_count] = name;
    blocklist_charsets[blockfile_count] = charset;
    blockfile_count++;
}

int
main(int argc, char *argv[])
{
    int opt, i;

    while ((opt = getopt(argc, argv, "q:a:r:dp:f:vsl:"
#ifndef LOWMEM
                              "c:"
#endif
                              )) != -1) {
        switch (opt) {
        case 'q':
            queue_num = atoi(optarg);
            break;
        case 'r':
            reject_mark = htonl((uint32_t)atoi(optarg));
            break;
        case 'a':
            accept_mark = htonl((uint32_t)atoi(optarg));
            break;
        case 'p':
            pidfile_name = optarg;
            break;
#ifndef LOWMEM
        case 'c':
            current_charset = optarg;
            break;
#endif
        case 'f':
            add_blocklist(optarg, current_charset);
            break;
        case 'v':
            opt_verbose++;
            break;
        case 's':
            use_syslog = 1;
            break;
        case 'l':
            logfile_name=malloc(strlen(optarg)+1);
            CHECK_OOM(logfile_name);
            strcpy(logfile_name,optarg);
            break;
#ifdef HAVE_DBUS
        case 'd':
            use_dbus = 1;
            break;
#endif
        }
    }

    if (queue_num < 0 || queue_num > 65535) {
        print_usage();
        exit(1);
    }

//     if (logfile_name == NULL) {
//         logfile_name=malloc(strlen(LOGDIR) + strlen(PKGNAME) + 6);
//         CHECK_OOM(logfile_name);
//         strcpy(logfile_name, LOGDIR);
//         strcat(logfile_name, "/");
//         strcat(logfile_name, PKGNAME);
//         strcat(logfile_name, ".log");
//     }
    if (logfile_name != NULL) {
        if ((logfile=fopen(logfile_name,"a")) == NULL) {
            fprintf(stderr, "Unable to open logfile: %s", logfile_name);
            perror(" ");
            exit(-1);
        }
    }

    for (i = 0; i < argc - optind; i++)
        add_blocklist(argv[optind + i], current_charset);

    if (blockfile_count == 0) {
        print_usage();
        exit(1);
    }

    blocklist_init(&blocklist);

    if (load_all_lists() < 0) {
        do_log(LOG_ERR, "Cannot load the blocklist");
        return -1;
    }

    if (opt_daemon) {
        daemonize();
        openlog("pgld", 0, LOG_DAEMON);
    }

// #ifdef HAVE_DBUS
//     if (use_dbus) {
//         if (open_dbus() < 0) {
//             do_log(LOG_ERR, "Cannot load D-Bus plugin");
//             use_dbus = 0;
//         }
//
//         if (pgl_dbus_init() < 0) {
//             fprintf(stderr, "Cannot initialize D-Bus");
//             exit(1);
//         }
//     }
// #endif

    if (install_sighandler() != 0)
        return -1;

    pidfile = create_pidfile(pidfile_name);
    if (!pidfile)
        return -1;

    do_log(LOG_INFO, "Started");
    do_log(LOG_INFO, "Blocklist has %d entries", blocklist.count);
    nfqueue_loop();
    blocklist_stats(&blocklist,0);
// #ifdef HAVE_DBUS
//     if (use_dbus)
//         close_dbus();
// #endif
    blocklist_clear(&blocklist, 0);
    free(blocklist_filenames);
    free(blocklist_charsets);
    closelog();
    if (pidfile) {
        fclose(pidfile);
        unlink(pidfile_name);
    }
    return 0;
}
