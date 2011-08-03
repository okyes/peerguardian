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

#include "pgld.h"

static int opt_merge = 0;
static uint16_t queue_num = 0;
static int use_syslog = 0;
static uint32_t accept_mark = 0, reject_mark = 0;
static char *pidfile_name = NULL;
static FILE *logfile;
static char *logfile_name=NULL;

static const char *current_charset = 0;

static int blockfile_count = 0;
static const char **blocklist_filenames = 0;
static const char **blocklist_charsets = 0;
static FILE* pidfile = NULL;
static char timestr[17];

// Default is no dbus, enable it with "-d"
#ifdef HAVE_DBUS
static int use_dbus = 0;
//Declarations of dbus functions so that they can be dynamically loaded
static int (*pgl_dbus_init)(void) = NULL;
static void (*pgl_dbus_send)(const char *, va_list) = NULL;
#endif

struct nfq_handle *nfqueue_h = 0;
struct nfq_q_handle *nfqueue_qh = 0;


void do_log(int priority, const char *format, ...)
{
    if (use_syslog) {
        va_list ap;
        va_start(ap, format);
        vsyslog(LOG_MAKEPRI(LOG_DAEMON, priority), format, ap);
        va_end(ap);
    }

    if (logfile) {
        va_list ap;
        va_start(ap, format);
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
        va_end(ap);
    }
#ifdef HAVE_DBUS
    if (use_dbus) {
       va_list ap;
       va_start(ap, format);
       pgl_dbus_send(format, ap);
       va_end(ap);
   }
#endif

    if (opt_merge) {
        va_list ap;
        va_start(ap, format);
        vfprintf(stderr, format, ap);
        fprintf(stderr,"\n");
        va_end(ap);
    }
}

void int2ip (uint32_t ipint, char *ipstr) {
    ipint=htonl(ipint);
    inet_ntop(AF_INET, &ipint, ipstr, INET_ADDRSTRLEN);
}

#ifdef HAVE_DBUS
static void *dbus_lh = NULL;

#define do_dlsym(symbol)                                                       \
    do {                                                                       \
        symbol = dlsym(dbus_lh, # symbol);                                     \
        err = dlerror();                                                       \
        if (err) {                                                             \
            do_log(LOG_ERR, "ERROR: Cannot get symbol %s: %s", # symbol, err); \
            goto out_err;                                                      \
        }                                                                      \
    } while (0)

static int open_dbus() {

    char *err;

    dbus_lh = dlopen(PLUGINDIR "/dbus.so", RTLD_NOW);
    if (!dbus_lh) {
        do_log(LOG_ERR, "ERROR: dlopen() failed: %s", dlerror());
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

static int close_dbus() {
    int ret = 0;

    if (dbus_lh) {
        ret = dlclose(dbus_lh);
        dbus_lh = 0;
    }

    return ret;
}

#endif

static FILE *create_pidfile(const char *name) {
    FILE *f;

    f = fopen(name, "w");
    if (f == NULL) {
        fprintf(stderr, "Unable to create PID file %s: %s\n", name, strerror(errno));
        return NULL;
    }

    /* this works even if pidfile is stale after daemon is sigkilled */
    if (lockf(fileno(f), F_TLOCK, 0) == -1) {
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

static void daemonize() {
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
    do_log(LOG_INFO, "INFO: Started");
}

static int load_all_lists() {
    int i, ret = 0;

    blocklist_clear(0);
    if (blockfile_count) {
        for (i = 0; i < blockfile_count; i++) {
            if (load_list(blocklist_filenames[i], blocklist_charsets[i])) {
                do_log(LOG_ERR, "ERROR: Error loading %s", blocklist_filenames[i]);
                ret = -1;
            }
        }
    } else {
        //assume stdin for list
        load_list(NULL,NULL);
    }
    blocklist_sort();
    blocklist_merge();
    do_log(LOG_INFO, "INFO: Blocklist has %u IP ranges (%u IPs)", blocklist.count, blocklist.numips);
    return ret;
}

static void nfqueue_unbind() {
    if (!nfqueue_h)
        return;

    do_log(LOG_INFO, "INFO: NFQUEUE: unbinding from queue 0");
    nfq_destroy_queue(nfqueue_qh);
    if (nfq_unbind_pf(nfqueue_h, AF_INET) < 0) {
        do_log(LOG_ERR, "ERROR: during nfq_unbind_pf(): %s", strerror(errno));
    }
    nfq_close(nfqueue_h);
}

static void sighandler(int sig, siginfo_t *info, void *context) {
    switch (sig) {
    case SIGUSR1:
        // dump and reset stats
        blocklist_stats(1);
        break;
    case SIGUSR2:
        // just dump stats
        blocklist_stats(0);
        break;
    case SIGHUP:
        if (logfile_name != NULL) {
            do_log(LOG_INFO, "INFO: Closing logfile: %s", logfile_name);
            fclose(logfile);
            logfile=NULL;
            if ((logfile=fopen(logfile_name,"a")) == NULL) {
                do_log(LOG_ERR, "ERROR: Unable to open logfile: %s", logfile_name);
                perror(" ");
                exit(-1);
            } else {
                do_log(LOG_INFO, "INFO: Reopened logfile: %s", logfile_name);
            }
        }
        if (load_all_lists() < 0) {
            do_log(LOG_ERR, "ERROR: Cannot re-load the blocklist(s)");
        }
        do_log(LOG_INFO, "INFO: Blocklist reloaded");
        break;
    case SIGTERM:
    case SIGINT:
        nfqueue_unbind();
        blocklist_stats(0);

// #ifdef HAVE_DBUS
//         if (use_dbus)
//             close_dbus();
// #endif
        blocklist_clear(0);
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

static int install_sighandler() {
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

static void setipinfo (char *src, char *dst, char *proto, struct iphdr *ip, char *payload){
    struct udphdr *udp;
    struct tcphdr *tcp;
    switch (ip->protocol) {
    case TCP:
        strcpy(proto, "TCP");
        tcp     = (struct tcphdr*) (payload + (4 * ip->ihl));
        sprintf(src, "%u.%u.%u.%u:%u",NIPQUAD(ip->saddr),ntohs(tcp->source));
        sprintf(dst, "%u.%u.%u.%u:%u",NIPQUAD(ip->daddr),ntohs(tcp->dest));
        break;
    case UDP:
        strcpy(proto, "UDP");
        udp     = (struct udphdr*) (payload + (4 * ip->ihl));
        sprintf(src, "%u.%u.%u.%u:%u",NIPQUAD(ip->saddr),ntohs(udp->source));
        sprintf(dst, "%u.%u.%u.%u:%u",NIPQUAD(ip->daddr),ntohs(udp->dest));
        break;
    case ICMP:
        strcpy(proto, "ICMP");\
        sprintf(src, "%u.%u.%u.%u",NIPQUAD(ip->saddr));
        sprintf(dst, "%u.%u.%u.%u",NIPQUAD(ip->daddr));
        break;
    default:
        sprintf(proto, "%d", ip->protocol);
        sprintf(src, "%u.%u.%u.%u",NIPQUAD(ip->saddr));
        sprintf(dst, "%u.%u.%u.%u",NIPQUAD(ip->daddr));
        break;
    }
}

static int nfqueue_cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data) {
    int id = 0, status = 0;
    struct nfqnl_msg_packet_hdr *ph;
    block_entry_t *found_range;
    struct iphdr *ip;
    char *payload, proto[5], src[23], dst[23];  //src and dst are 23 for IP(16)+port(5) + : + NULL

    ph = nfq_get_msg_packet_hdr(nfa);
    if (ph) {
        id = ntohl(ph->packet_id);
        nfq_get_payload(nfa, &payload);
        ip = (struct iphdr*) payload;
        switch (ph->hook) {
        case NF_IP_LOCAL_IN:
            found_range = blocklist_find(ntohl(ip->saddr));
            if (found_range) {
                status = nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
                found_range->hits++;
                setipinfo(src, dst, proto, ip, payload);
#ifndef LOWMEM
                do_log(LOG_NOTICE, " IN: %-22s %-22s %-4s || %s",src,dst,proto,found_range->name);
#else
                do_log(LOG_NOTICE, " IN: %-22s %-22s %-4s",src,dst,proto);
#endif
            } else if (accept_mark) {
                // we set the user-defined accept_mark and set NF_REPEAT verdict
                // it's up to other iptables rules to decide what to do with this marked packet
                status = nfq_set_verdict_mark(qh, id, NF_REPEAT, accept_mark, 0, NULL);
            } else {
                // no accept_mark, just NF_ACCEPT the packet
                status = nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
            }
            break;
        case NF_IP_LOCAL_OUT:

            found_range = blocklist_find(ntohl(ip->daddr));
            if (found_range) {
                if (reject_mark) {
                    // we set the user-defined reject_mark and set NF_REPEAT verdict
                    // it's up to other iptables rules to decide what to do with this marked packet
                    status = nfq_set_verdict_mark(qh, id, NF_REPEAT, reject_mark, 0, NULL);
                } else {
                    status = nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
                }
                found_range->hits++;
                setipinfo(src, dst, proto, ip, payload);
#ifndef LOWMEM
                do_log(LOG_NOTICE, "OUT: %-22s %-22s %-4s || %s",src,dst,proto,found_range->name);
#else
                do_log(LOG_NOTICE, "OUT: %-22s %-22s %-4su", src,dst,proto);
#endif
            } else if (accept_mark) {
                // we set the user-defined accept_mark and set NF_REPEAT verdict
                // it's up to other iptables rules to decide what to do with this marked packet
                status = nfq_set_verdict_mark(qh, id, NF_REPEAT, accept_mark, 0, NULL);
            } else {
                // no accept_mark, just NF_ACCEPT the packet
                status = nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
            }
            break;
        case NF_IP_FORWARD:
            found_range = blocklist_find(ntohl(ip->saddr));
            if (!found_range) {
                found_range = blocklist_find(ntohl(ip->daddr));
            }
            if (found_range) {
                if (reject_mark) {
                    // we set the user-defined reject_mark and set NF_REPEAT verdict
                    // it's up to other iptables rules to decide what to do with this marked packet
                    status = nfq_set_verdict_mark(qh, id, NF_REPEAT, reject_mark, 0, NULL);
                } else {
                    status = nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
                }

                found_range->hits++;
                setipinfo(src, dst, proto, ip, payload);
#ifndef LOWMEM
                do_log(LOG_NOTICE, "FWD: %-22s %-22s %-4s || %s",src,dst,proto,found_range->name);
#else
                do_log(LOG_NOTICE, "FWD: %-22s %-22s %-4s",src,dst,proto);
#endif
            } else if (accept_mark) {
                // we set the user-defined accept_mark and set NF_REPEAT verdict
                // it's up to other iptables rules to decide what to do with this marked packet
                status = nfq_set_verdict_mark(qh, id, NF_REPEAT, accept_mark, 0, NULL);
            } else {
                // no accept_mark, just NF_ACCEPT the packet
                status = nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
            }
            break;
        default:
            do_log(LOG_NOTICE, "WARN: Not NF_LOCAL_IN/OUT/FORWARD packet!");
            break;
        }
    } else {
        do_log(LOG_ERR, "ERROR: NFQUEUE: can't get msg packet header.");
        return 1;               // from nfqueue source: 0 = ok, >0 = soft error, <0 hard error
    }
    return 0;
}

static int nfqueue_bind() {
    nfqueue_h = nfq_open();
    if (!nfqueue_h) {
        do_log(LOG_ERR, "ERROR: Error during nfq_open(): %s", strerror(errno));
        return -1;
    }

    if (nfq_unbind_pf(nfqueue_h, AF_INET) < 0) {
        do_log(LOG_ERR, "ERROR: Error during nfq_unbind_pf(): %s", strerror(errno));
        nfq_close(nfqueue_h);
        return -1;
    }

    if (nfq_bind_pf(nfqueue_h, AF_INET) < 0) {
        do_log(LOG_ERR, "ERROR: Error during nfq_bind_pf(): %s", strerror(errno));
        nfq_close(nfqueue_h);
        return -1;
    }

    do_log(LOG_INFO, "INFO: NFQUEUE: binding to queue %d", ntohs(queue_num));
    if (accept_mark) {
        do_log(LOG_INFO, "INFO: ACCEPT mark: %u", ntohl(accept_mark));
    }
    if (reject_mark) {
        do_log(LOG_INFO, "INFO: REJECT mark: %u", ntohl(reject_mark));
    }
    nfqueue_qh = nfq_create_queue(nfqueue_h, ntohs(queue_num), &nfqueue_cb, NULL);
    if (!nfqueue_qh) {
        do_log(LOG_ERR, "ERROR: Error during nfq_create_queue(): %s", strerror(errno));
        nfq_close(nfqueue_h);
        return -1;
    }

    if (nfq_set_mode(nfqueue_qh, NFQNL_COPY_PACKET, PAYLOADSIZE) < 0) {
        do_log(LOG_ERR, "ERROR: Can't set packet_copy mode: %s", strerror(errno));
        nfq_destroy_queue(nfqueue_qh);
        nfq_close(nfqueue_h);
        return -1;
    }
    return 0;
}

static void nfqueue_loop () {
    struct nfnl_handle *nh;
    int fd, rv;
    char buf[RECVBUFFSIZE];
//  struct pollfd fds[1];

    if (nfqueue_bind() < 0) {
        do_log(LOG_ERR, "ERROR: ERROR binding to queue!");
        exit(1);
    }
    daemonize();

    if (install_sighandler() != 0) {
        do_log(LOG_ERR, "ERROR: ERROR installing signal handlers");
        exit(1);
    }

    pidfile = create_pidfile(pidfile_name);
    if (!pidfile) {
        do_log(LOG_ERR, "ERROR: ERROR creating pidfile %s", pidfile_name);
        exit(1);
    }

    nh = nfq_nfnlh(nfqueue_h);
    fd = nfnl_fd(nh);

    while ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) {
        nfq_handle_packet(nfqueue_h, buf, rv);
    }
    int err=errno;
    do_log(LOG_ERR, "ERROR: unbinding from queue '%hd', recv returned %s", queue_num, strerror(err));
    if ( err == ENOBUFS ) {
        /* close and return, nfq_destroy_queue() won't work as we've no buffers */
        nfq_close(nfqueue_h);
        exit(1);

    } else {
        nfqueue_unbind();
        exit(0);
    }
}

static void print_usage() {
    fprintf(stderr, PACKAGE_NAME " " VERSION "\n\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  pgld ");
#ifdef HAVE_DBUS
    fprintf(stderr, "[-d] ");
#endif
    fprintf(stderr, "[-s] [-l LOGFILE] ");
#ifndef LOWMEM
    fprintf(stderr, "[-c CHARSET] ");
#endif
    fprintf(stderr, "[-p PIDFILE] [-a MARK] [-r MARK] [-q 0-65535] BLOCKLIST(S)\n");
    fprintf(stderr, "  pgld -m [BLOCKLIST(S)]\n\n");
#ifdef HAVE_DBUS
    fprintf(stderr, "        -d            Enable D-Bus support.\n");
#endif
    fprintf(stderr, "        -s            Enable logging to syslog.\n");
    fprintf(stderr, "        -l LOGFILE    Enable logging to LOGFILE.\n");
#ifndef LOWMEM
    fprintf(stderr, "        -c CHARSET    Specify blocklist file CHARSET.\n");
#endif
    fprintf(stderr, "        -p PIDFILE    Use a PIDFILE.\n");
    fprintf(stderr, "        -a MARK       Place a 32-bit MARK on accepted packets.\n");
    fprintf(stderr, "        -r MARK       Place a 32-bit MARK on rejected packets.\n");
    fprintf(stderr, "        -q 0-65535    Specify a 16-bit NFQUEUE number.\n");
    fprintf(stderr, "                      Must match --queue-num used in iptables rules.\n");
    fprintf(stderr, "        -m [BLOCKLIST(S)] Load, sort, merge, and dump blocklist(s) specified or piped from stdin.\n");
}

void add_blocklist(const char *name, const char *charset) {
    blocklist_filenames = (const char**)realloc(blocklist_filenames, sizeof(const char*) * (blockfile_count + 1));
    CHECK_OOM(blocklist_filenames);
    blocklist_charsets = (const char**)realloc(blocklist_charsets, sizeof(const char*) * (blockfile_count + 1));
    CHECK_OOM(blocklist_charsets);
    blocklist_filenames[blockfile_count] = name;
    blocklist_charsets[blockfile_count] = charset;
    blockfile_count++;
}

int main(int argc, char *argv[]) {
    int opt, i;
    int try_dbus = 0;
    while ((opt = getopt(argc, argv, "q:a:r:dp:sl:mh"
#ifndef LOWMEM
        "c:"
#endif
        )) != -1) {
        switch (opt) {
        case 'q':
            queue_num = htons((uint16_t)atoi(optarg));
            break;
        case 'r':
            reject_mark = htonl((uint32_t)atoi(optarg));
            break;
        case 'a':
            accept_mark = htonl((uint32_t)atoi(optarg));
            break;
        case 'p':
            pidfile_name=malloc(strlen(optarg)+1);
            CHECK_OOM(pidfile_name);
            strcpy(pidfile_name,optarg);
            break;
#ifndef LOWMEM
        case 'c':
            current_charset = optarg;
            break;
#endif
        case 's':
            use_syslog = 1;
            break;
        case 'm':
            opt_merge = 1;
            break;
        case 'l':
            logfile_name=malloc(strlen(optarg)+1);
            CHECK_OOM(logfile_name);
            strcpy(logfile_name,optarg);
            break;
        case 'h':
            print_usage();
            exit(0);
#ifdef HAVE_DBUS
        case 'd':
            try_dbus = 1;
            break;
#endif
        }
    }

    for (i = 0; i < argc - optind; i++) {
        add_blocklist(argv[optind + i], current_charset);
    }

    if (opt_merge) {
        blocklist_init();
        load_all_lists();
        blocklist_dump();
        exit(0);
    }

    if (blockfile_count == 0) {
        fprintf(stderr, "\nERROR: No blocklist specified!\n\n");
        print_usage();
        exit(1);
    }

    if (!queue_num) {
        queue_num = htons(92);
    }

    if ((ntohs(queue_num) < 0 || ntohs(queue_num) > 65535) && !opt_merge) {
        fprintf(stderr, "\nERROR: Invalid queue number! Must be 0-65535\n\n");
        print_usage();
        exit(1);
    }

    if (logfile_name != NULL) {
        if ((logfile=fopen(logfile_name,"a")) == NULL) {
            fprintf(stderr, "Unable to open logfile: %s", logfile_name);
            perror(" ");
            exit(-1);
        }
    }

    if (pidfile_name == NULL) {
            pidfile_name=malloc(strlen(PIDFILE)+1);
            CHECK_OOM(pidfile_name);
            strcpy(pidfile_name,PIDFILE);
    }

    //open syslog
    if (use_syslog) {
        openlog("pgld", 0, LOG_DAEMON);
    }

    blocklist_init();
    if (load_all_lists() < 0) {
        do_log(LOG_ERR, "ERROR: Cannot load the blocklist(s)");
        return -1;
    }

#ifdef HAVE_DBUS
    if (try_dbus) {
        if (open_dbus() < 0) {
            do_log(LOG_ERR, "ERROR: Cannot load D-Bus plugin");
            try_dbus = 0;
        }
        if (pgl_dbus_init() < 0) {
            fprintf(stderr, "Cannot initialize D-Bus");
            try_dbus = 0;
            exit(1);
        }
    }
    use_dbus = try_dbus;
#endif

    nfqueue_loop();
    blocklist_stats(0);
#ifdef HAVE_DBUS
    if (use_dbus)
        close_dbus();
#endif
    blocklist_clear(0);
    free(blocklist_filenames);
    free(blocklist_charsets);
    // close syslog
    if (use_syslog) {
        closelog();
    }
    if (pidfile) {
        fclose(pidfile);
        unlink(pidfile_name);
    }
    return 0;
}
