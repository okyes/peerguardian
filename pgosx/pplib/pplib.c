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
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#import <sys/sysctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>

#ifdef PPDBG
#define dbgprint printf
#else
#define dbgprint(f, ...)
#endif

#ifdef __APPLE__
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#include <CoreFoundation/CoreFoundation.h>
#endif // __APPLE__

#include "pplib.h"

#ifdef __APPLE__

static u_int32_t processLogEntries(const pp_log_handle_t, pp_log_entry_t *);

static int ppi_open_session(int *sock)
{
    int err = EINVAL;
    int so = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (so >= 0) {
        struct sockaddr_ctl addr;
        bzero(&addr, sizeof(addr));
        addr.sc_len = sizeof(addr);
        addr.sc_family = AF_SYSTEM;
        addr.ss_sysaddr = AF_SYS_CONTROL;
        
        struct ctl_info info;
        bzero(&info, sizeof(info));
        strncpy(info.ctl_name, PP_CTL_NAME, sizeof(info.ctl_name));
        if (!(err = ioctl(so, CTLIOCGINFO, &info))) {
            addr.sc_id = info.ctl_id;
            addr.sc_unit = 0;
            if (!(err = connect(so, (struct sockaddr *)&addr, sizeof(addr)))) {
                *sock = so;
                return (0);
            } else { // connect
                perror("Failed to connect to kern ctl");
                err = errno;
            }
        } else {
            err = errno;
            perror("Could not get ID for kernel control");
        }
    } else { // socket()
        err = errno;
    }
    
    if (so > -1)
        close(so);
    
    return (err);
}

struct pp_session_ {
    int sock;
    u_int32_t seq;
};

int pp_close_session(pp_session_t sp)
{
    close(sp->sock);
    free(sp);
    return (0);
}

int pp_open_session(pp_session_t *sp)
{
    int err;
    if ((*sp = malloc(sizeof(**sp))))
        bzero(*sp, sizeof(**sp));
    else
        return (ENOMEM);
    if (0 == (err = ppi_open_session(&(*sp)->sock)))
        return (0);
    
    free(*sp);
    *sp = NULL;
    return (err);
}


int pp_load_table(const pp_msg_iprange_t *addrs, int32_t rangect, u_int32_t addrct,
    const pg_table_id_t tableid, int loadflags)
{
    int so;
    int i, err;
    
    if (0 != (loadflags & ~PP_LOAD_VALID_FLGS) || 0 != (loadflags & PP_LOAD_END))
        return (EINVAL);
    
    if ((err = ppi_open_session(&so)))
        return (err);
    
    pp_msg_t msgp;
    u_int32_t size = sizeof(*msgp) + sizeof(pp_msg_load_t);
    int32_t count = (PP_MSG_RECOMMENDED_MAX - size) / sizeof(pp_msg_iprange_t);
    if (count > rangect)
        count = rangect;
    size += count * sizeof(pp_msg_iprange_t);
    
    msgp = malloc(size);
    if (!msgp) {
        err = ENOMEM;
        goto pp_load_exit;
    }
    bzero(msgp, size);
    msgp->ppm_magic = PP_MSG_MAGIC;
    msgp->ppm_version = PP_MSG_VERSION;
    msgp->ppm_type = PP_MSG_TYPE_LOAD;
    
    pp_msg_load_t *mldp = (pp_msg_load_t*)msgp->ppm_data;
    mldp->ppld_version = PP_LOAD_VERSION;
    mldp->ppld_flags = PP_LOAD_BEGIN | loadflags;
    pg_table_id_copy(tableid, mldp->ppld_tableid);
    // we have to send a begin msg with the total count first
    msgp->ppm_size = sizeof(*msgp) + sizeof(*mldp);
    mldp->ppld_count = rangect;
    mldp->ppld_addrct = addrct;
    if (-1 == send(so, msgp, msgp->ppm_size, 0)) {
        err = errno;
        goto pp_load_exit;
    }
    msgp->ppm_sequence++;
    
    msgp->ppm_size = size;
    mldp->ppld_flags = 0;
    i = 0;
    while (rangect > 0) {
        mldp->ppld_count = count;
        bcopy(&addrs[i], mldp->ppld_rngs, sizeof(addrs[0]) * count);
        if (-1 == send(so, msgp, msgp->ppm_size, 0)) {
            err = errno;
            perror("Failed to send load");
            break;
        }
        msgp->ppm_sequence++;
        
        rangect -= count;
        i += count;
        if (rangect && rangect < count) {
            count = rangect;
            msgp->ppm_size = sizeof(*msgp) + sizeof(*mldp) + (count * sizeof(pp_msg_iprange_t));
        }
    }
    
    if (!err) {
        msgp->ppm_size = sizeof(*msgp) + sizeof(*mldp);
        mldp->ppld_count = 0;
        mldp->ppld_flags = PP_LOAD_END;
        if (loadflags & PP_LOAD_PORT_RULE)
            mldp->ppld_flags |= PP_LOAD_PORT_RULE;
        if (-1 == send(so, msgp, msgp->ppm_size, 0)) {
            err = errno;
            perror("Failed to send load end");
        }
        msgp->ppm_sequence++;
    }
    
pp_load_exit:
    free(msgp);
    close(so);
    return (err);
}

struct pp_shared_names {
    u_int32_t magic;
    pg_table_id_t tblid;
    char tblname[32];
    int32_t nameCount;
    int32_t nameOffsets[0];
};
// should be enough for the largest of tables, if not, then we're screwed
#define PP_NAMES_SHM_SIZE (8 * 1024 * 1024)
#define PP_NAMES_MAGIC 0x7bd66bc1 
#define PP_NAMES_BAD_MAGIC 0xa775699f

// This is not defined in Tiger headers, but the limit is 31 chars + null
// XXX - Seems to be a bug in current Tiger kernels (up to 10.4.5) where the NULL
// byte is counted against the 31 char length, so the actual allowed length is 30.
#ifndef SEM_NAME_LEN
#define SEM_NAME_LEN 31
#endif

static __inline__ void
pp_make_shmname(const pg_table_id_t tableid, char *buf, size_t len)
{
    (void)pg_table_id_string(tableid, buf, len, 0);
    // XXX - We lose 1 digit of resolution in the UUID here because names are limted to 31 chars
    // XXX - POSIX specifies that only names beginning with '/' will reference the same object.
    // Without the '/', the behavior is implementation specific. In the case of OS X,
    // there is no difference, but this may not be true on other platforms.
    buf[SEM_NAME_LEN-1] = 0;
}

int pp_load_names(const pg_table_id_t tableid, const char *tblname, u_int8_t* const *names)
{
    char shmname[SEM_NAME_LEN*2];
    pp_make_shmname(tableid, shmname, sizeof(shmname)-1);
    
    int nameBytes = 0, nameCount;
    for (nameCount = 0; names[nameCount]; ++nameCount)  {
        nameBytes += strlen((char*)names[nameCount]) + 1 /* NULL */;
    }
    nameCount++;
    if (nameBytes + (sizeof(int32_t) * nameCount) + sizeof(struct pp_shared_names) > PP_NAMES_SHM_SIZE)
        return (EFBIG);
    
    int flags = O_CREAT|O_EXCL, sem_created = 0, shm_created = 0, err = 0, locked = 0;
    mode_t mask, perms;
    // we give access to all, since the kernel shares all tables with all users
    perms = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
load_names_open_sem:
    mask = umask(0);
    sem_t *lck = sem_open(shmname, flags, perms, SEM_VALUE_MAX);
    (void)umask(mask);
    
    if ((sem_t*)SEM_FAILED == lck) {
        if (EEXIST == errno && (flags & O_EXCL)) {
            flags &= ~O_EXCL;
            goto load_names_open_sem;
        }
        return (errno);
    }
    
    if (-1 == sem_wait(lck)) {
        err = errno;
        goto load_names_err;
    }
    locked = 1;
    
    if (flags & O_EXCL)
        sem_created = 1;
    
    flags = O_RDWR|O_CREAT|O_EXCL;
    int fd;
load_names_open_shm:
    mask = umask(0);
    fd = shm_open(shmname, flags, perms);
    (void)umask(mask);
    if (-1 == fd) {
        if (EEXIST == errno && (flags & O_EXCL)) {
            flags &= ~O_EXCL;
            goto load_names_open_shm;
        }
        
        err = errno;
        goto load_names_err;
    }
    
    if (flags & O_EXCL) {
        shm_created = 1;
        // for whatever reason, this is necessary, to let mmap work
        ftruncate(fd, PP_NAMES_SHM_SIZE);
    }
    
    struct stat sb;
    if (-1 == fstat(fd, &sb)) {
        err = errno;
        goto load_names_err;
    }
    
    struct pp_shared_names *map;
    map = (typeof(map))mmap(NULL, sb.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == map) {
        err = errno;
        goto load_names_err;
    }
    
    map->magic = PP_NAMES_MAGIC;
    
    strncpy(map->tblname, tblname, sizeof(map->tblname)-1);
    map->tblname[sizeof(map->tblname)-1] = 0;
    pg_table_id_copy(tableid, map->tblid);
    
    map->nameOffsets[0] = (int32_t)(((u_int8_t*)map->nameOffsets + ((nameCount+1) * sizeof(int32_t)))
        - (u_int8_t*)map->nameOffsets);
    int32_t i;
    for (i=0; names[i]; ++i) {
        strcpy(((char*)map->nameOffsets) + map->nameOffsets[i], (char*)names[i]);
        
        // setup the offset for the next name
        map->nameOffsets[i+1] = map->nameOffsets[i] + (strlen((char*)names[i]) + 1);
    }
    map->nameOffsets[i] = -1;
    
    map->nameCount = nameCount;
    
    (void)munmap(map, PP_NAMES_SHM_SIZE);

load_names_err:
    if (-1 != fd) {
        if (shm_created && err)
            shm_unlink(shmname);
        close(fd);
    }
    
    if (sem_created && err)
        sem_unlink(shmname);
        
    if (locked)
        sem_post(lck);
    
    sem_close(lck);
    
    return (err);
}

struct pp_name_map_ {
    off_t mapsize;
    u_int32_t magic;
    sem_t *lck;
    int fd;
    struct pp_shared_names *map;
    pg_table_id_t tableid;
};
#define PP_NAME_MAP_MAGIC 0x93ecf9f3

int pp_map_names(const pg_table_id_t tableid, pp_name_map_t *handle)
{
    pp_name_map_t h = (pp_name_map_t)malloc(sizeof(*h));
    if (!h)
        return (ENOMEM);
    h->lck = (typeof(h->lck))SEM_FAILED;
    h->fd = -1;
    h->map = NULL;
    
    int err = 0;
    char shmname[SEM_NAME_LEN*2]; 
    pp_make_shmname(tableid, shmname, sizeof(shmname)-1);
    
    h->lck = sem_open(shmname, 0);
    if ((typeof(h->lck))SEM_FAILED == h->lck) {
        err = errno;
        //perror("sem_open");
        goto map_names_exit;
    }
    
    if (-1 ==(h->fd = shm_open(shmname, O_RDONLY))) {
        err = errno;
        //perror("shm_open");
        goto map_names_exit;
    }
    
    struct stat sb;
    if (-1 == fstat(h->fd, &sb)) {
        err = errno;
        //perror("fstat");
        goto map_names_exit;
    }
    
    h->map = (typeof(h->map))mmap(NULL, (size_t)sb.st_size, PROT_READ, MAP_SHARED, h->fd, 0);
    if (MAP_FAILED == h->map) {
        h->map = NULL;
        err = errno;
        //perror("mmap");
        goto map_names_exit;
    }
    h->mapsize = sb.st_size;
    
    if (PP_NAMES_MAGIC != h->map->magic) {
        //fprintf(stderr, "magic err\n");
        err = EINVAL;
    }
    
map_names_exit:
    if (!err) {
        h->magic = PP_NAME_MAP_MAGIC;
        pg_table_id_copy(tableid, h->tableid);
        *handle = h;
    } else {
        if (h->map)
            (void)munmap(h->map, PP_NAMES_SHM_SIZE);
        if (-1 != h->fd)
            (void)close(h->fd);
        if ((typeof(h->lck))SEM_FAILED != h->lck)
            (void)sem_close(h->lck);
        free(h);
    }
    
    return (err);
}

PP_EXTERN
int pp_map_name(const pp_name_map_t h, char **name)
{
    if (PP_NAME_MAP_MAGIC != h->magic)
        return (EINVAL);
        
    if (-1 == sem_wait(h->lck)) {
        //perror("sem_wait");
        return (errno);
    }
    
    char *nameCopy = (char*)malloc(strlen(h->map->tblname)+1);
    int err = 0;
    if (nameCopy) {
        strcpy(nameCopy, h->map->tblname);
        *name = nameCopy;
    } else
        err = ENOMEM;
    
    sem_post(h->lck);
    
    return (err);
}

int pp_resolve_name(const pp_name_map_t h, int32_t index, char **name)
{
    if (PP_NAME_MAP_MAGIC != h->magic)
        return (EINVAL);
        
    if (-1 == sem_wait(h->lck)) {
        //perror("sem_wait");
        return (errno);
    }
    
    int err = 0;
    if (index < h->map->nameCount) {
        char *mappedName = (char*)((char*)h->map->nameOffsets + h->map->nameOffsets[index]);
        char *nameCopy = (char*)malloc(strlen(mappedName)+1);
        if (nameCopy) {
            strcpy(nameCopy, mappedName);
            *name = nameCopy;
        } else
            err = ENOMEM;
    } else
        err = ERANGE;
      
#if 0
    if (err)
        fprintf(stderr, "resolve err: %d\n", err);
#endif

    sem_post(h->lck);
    
    return (err);
}

int pp_unmap_names(pp_name_map_t h)
{
    if (PP_NAME_MAP_MAGIC != h->magic)
        return (EINVAL);
    
    (void)munmap(h->map, h->mapsize);
    (void)close(h->fd);
    (void)sem_close(h->lck);
    free(h);
    return (0);
}

int pp_unload_names(const pg_table_id_t tableid)
{
    char shmname[SEM_NAME_LEN*2]; 
    pp_make_shmname(tableid, shmname, sizeof(shmname)-1);
    
    int errshm = 0, errsem = 0;
    if (-1 == shm_unlink(shmname))
        errshm = errno;
        
    if (-1 == sem_unlink(shmname))
        errsem = errno;
    
    return (errshm ? errshm : errsem);
}

int pp_unload_table(const pg_table_id_t tableid)
{
    int err, so;
    if ((err = ppi_open_session(&so)))
        return (err);
    
    pp_msg_t msg;
    unsigned char buf[sizeof(*msg) + sizeof(pg_msg_rem_t)];
    bzero(buf, sizeof(buf));
    msg = (typeof(msg))buf;
    msg->ppm_magic = PP_MSG_MAGIC;
    msg->ppm_version = PP_MSG_VERSION;
    msg->ppm_type = PP_MSG_TYPE_REM;
    pg_table_id_copy(tableid, pg_msg_cast(pg_msg_rem_t*, msg)->pgr_tableid);
    msg->ppm_size = sizeof(buf);
    if (-1 == send(so, msg, msg->ppm_size, 0)) {
        err = errno;
    }
    
    close(so);
    return (err);
}

struct pp_log_handle_ {
    int magic, flags, sock;
    pp_log_entry_cb logcb, tblcb;
    pthread_t recvth, processth;
    pthread_mutex_t lck;
    pthread_cond_t  cond;
    u_int32_t sequence;
    pp_log_entry_t *logq_head, *logq_tail;
    u_int32_t entriesQueued, entriesProcessed, entriesReceived;
};
#define PP_LOG_MAGIC 0xdaadd00f

static void* processlogq(void* arg);
static void* recvlogmsg(void *arg);

int pp_log_listen(int logallowed, pp_log_entry_cb logEvent, pp_log_entry_cb tableEvent, pp_log_handle_t *handle)
{
    int err = 0;
    
    pp_log_handle_t h = (pp_log_handle_t)malloc(sizeof(*h));
    if (!h)
        return (ENOMEM);
    bzero(h, sizeof(*h));
    h->magic = PP_LOG_MAGIC;
    
    if ((err = ppi_open_session(&h->sock))) {
        free(h);
        return (err);
    }
    
    struct pp_msg msg;
    bzero(&msg, sizeof(msg));
    msg.ppm_magic = PP_MSG_MAGIC;
    msg.ppm_version = PP_MSG_VERSION;
    msg.ppm_type = PP_MSG_TYPE_LOG;
    if (logallowed)
        msg.ppm_flags = h->flags = PP_LOG_ALLOWED;
    msg.ppm_size = sizeof(msg);
    if (-1 == send(h->sock, &msg, msg.ppm_size, 0)) {
        free(h);
        return (errno);
    }
    h->sequence++;
    
    h->logcb = logEvent;
    h->tblcb = tableEvent;
    if (!(err = pthread_mutex_init(&h->lck, NULL))) {
        if (!(err = pthread_cond_init(&h->cond, NULL))) {
            if (!(err = pthread_create(&h->processth, NULL, recvlogmsg, h))) {
                err = pthread_create(&h->recvth, NULL, processlogq, h);
            }
        }
    }
    
    if (!err) {
        if (handle) {
            *handle = h;
        } else {
            pthread_t th = h->recvth;
            pthread_join(th, NULL);
            pp_log_stop(h); // when the thread exits, free the handle
        }
    } else
        pp_log_stop(h);
    
    return (err);
}

int pp_log_stop(pp_log_handle_t h)
{
    if (PP_LOG_MAGIC != h->magic)
        return (EINVAL);
    
    pthread_mutex_lock(&h->lck);
    int so = h->sock;
    pthread_t th = h->recvth;
    h->sock = -1;
    h->magic = PP_BAD_ADDRESS;
    pthread_mutex_unlock(&h->lck);
    
    if (so > -1)
    	close(so);
    
    // the recv thread will exit when the socket closes
    static const struct timespec ts = {0, 100000000};
    while (h->recvth)
        (void)nanosleep(&ts, NULL);
    // Don't know why, but pthread_join hangs here when the thread has already exited.
    pthread_detach(th);

    // we need to signal the processing thread to have it exit
    pthread_mutex_lock(&h->lck);
    th = h->processth;
    pthread_mutex_unlock(&h->lck);
    while (h->processth) {
        pthread_cond_signal(&h->cond);
        (void)nanosleep(&ts, NULL);
    }
    pthread_detach(th);
    
    // Process any outstanding msgs
    if (h->logq_head)
        (void)processLogEntries(h, h->logq_head);
    
    pthread_cond_destroy(&h->cond);
    pthread_mutex_destroy(&h->lck);
    
    free(h);
    
    return (0);
}

int pp_statistics_s(pp_session_t s, pp_msg_stats_t *stp)
{
    int err;
    int sclose = s ? 0 : 1;
    if (!s && (err = pp_open_session(&s)))
        return (err);
    else
        err = 0;
    
    pp_msg_t msgp;
    unsigned char buf[sizeof(*msgp)+sizeof(*stp)];
    msgp = (pp_msg_t)buf;
    
    bzero(msgp, sizeof(buf));
    msgp->ppm_magic = PP_MSG_MAGIC;
    msgp->ppm_version = PP_MSG_VERSION;
    msgp->ppm_type = PP_MSG_TYPE_STAT;
    msgp->ppm_size = sizeof(*msgp); // we don't need the extra stats size for the request
    msgp->ppm_sequence = s->seq;
    if (-1 != send(s->sock, msgp, msgp->ppm_size, 0)) {
        s->seq++;
        if (-1 != recv(s->sock, msgp, sizeof(buf), 0)) {
            s->seq++;
            pp_msg_stats_t *sp = (pp_msg_stats_t*)msgp->ppm_data;
            if (PP_MSG_TYPE_STAT == msgp->ppm_type && PP_STATS_VERSION == sp->pps_version) {
                *stp = *sp;
            } else {
                err = ENOTSUP;
            }
        } else
            err = errno;
    } else
        err = errno;
    
    if (sclose)
        pp_close_session(s);
    return (err);
}

int pp_avgconnstats(const pp_msg_stats_t *sp, u_int32_t *connps, u_int32_t *blockps)
{
    if (PP_STATS_VERSION == sp->pps_version) {
        struct timeval now;
        gettimeofday(&now, NULL);
        
        u_int64_t totalConn = (u_int64_t)sp->pps_in_blocked + (u_int64_t)sp->pps_in_allowed +
            (u_int64_t)sp->pps_out_blocked + (u_int64_t)sp->pps_out_allowed;
        u_int64_t totalBlocks = (u_int64_t)sp->pps_in_blocked + (u_int64_t)sp->pps_out_blocked;
        
        time_t elapsed = now.tv_sec - (time_t)(sp->pps_loadtime / 1000000UL);
        
        *connps = (typeof(*connps))(totalConn / elapsed);
        *blockps = (typeof(*blockps))(totalBlocks / elapsed);
        
        return (0);
    }
    
    return (EINVAL);
}

int pp_tableinfo(pp_msg_table_info_t *tip, int32_t *count)
{
    int err, so;
    if ((err = ppi_open_session(&so)))
        return (err);
    
    pp_msg_t msgp = malloc(PP_MSG_RECOMMENDED_MAX);
    if (!msgp)
        return (ENOMEM);
    
    int val = sizeof(*msgp);
    (void)setsockopt(so, SOL_SOCKET, SO_RCVLOWAT, &val, sizeof(val));
    
    bzero(msgp, sizeof(*msgp));
    memset(msgp->ppm_data, -1, PP_MSG_RECOMMENDED_MAX - sizeof(msgp));
    msgp->ppm_magic = PP_MSG_MAGIC;
    msgp->ppm_version = PP_MSG_VERSION;
    msgp->ppm_type = PP_MSG_TYPE_TBL_INFO;
    msgp->ppm_size = sizeof(*msgp); // we don't need the extra size for the request
    if (-1 != send(so, msgp, msgp->ppm_size, 0)) {
        if (-1 != recv(so, msgp, PP_MSG_RECOMMENDED_MAX, 0)) {
            if (PP_MSG_TYPE_TBL_INFO == msgp->ppm_type) {
                if (msgp->ppm_count < *count)
                    *count = msgp->ppm_count;
                bcopy(msgp->ppm_data, tip, (*count * sizeof(*tip)));
            } else {
                err = ENOTSUP;
            }
        } else
            err = errno;
    } else
        err = errno;
    
    free(msgp);
    
    close(so);
    return (err);
}

#if 0
int pp_procargs(int32_t pid, void *pbuf, size_t *bufSize)
{
    int err, so;
    if ((err = ppi_open_session(&so)))
        return (err);
    
    pp_msg_t msgp;
    pp_procargs_t *argsp;
    unsigned char buf[sizeof(*msgp)+sizeof(*argsp)];
    msgp = (pp_msg_t)buf;
    argsp = (pp_procargs_t*)msgp->ppm_data;
    
    bzero(msgp, sizeof(buf));
    msgp->ppm_magic = PP_MSG_MAGIC;
    msgp->ppm_version = PP_MSG_VERSION;
    msgp->ppm_type = PP_MSG_TYPE_PROC;
    msgp->ppm_size = sizeof(*msgp)+sizeof(*argsp);
    
    argsp->ppp_buf = CAST_USER_ADDR_T(pbuf);
    argsp->ppp_bufsize = CAST_USER_ADDR_T(bufSize);
    argsp->ppp_pid = pid;
    if (-1 == send(so, msgp, msgp->ppm_size, 0))
        err = errno;
    
    close(so);
    return (err);
}
#endif

// private

static int
log_compare(const void *r1, const void *r2)
{
    if (((const pp_log_entry_t*)r1)->pplg_timestamp == ((const pp_log_entry_t*)r2)->pplg_timestamp)
        return (0);
    else if (((const pp_log_entry_t*)r1)->pplg_timestamp < ((const pp_log_entry_t*)r2)->pplg_timestamp)
        return (-1);
    else
        return (1);
}

static void* recvlogmsg(void *arg)
{
    fd_set rfds, tfds;
    int nread, err;
    pp_log_handle_t h = (pp_log_handle_t)arg;
    int so = h->sock;
    
    FD_ZERO(&rfds);
    FD_SET(so, &rfds);
    
    do {
        tfds = rfds;
        dbgprint("waiting for msg\n");
        err = select(so+1, &tfds, (fd_set *)0, 
            (fd_set *)0, (struct timeval *)0);
        if (err < 1) {
            if (-1 == err && EBADF == errno) {
                // the socket has disappeared, try to reconnect ?
                perror("socket died");
                break;
            } else {
                continue;
            }
        }
        
        if(!FD_ISSET(so, &tfds))
            continue;
        
        // Get the size of data on the socket
        err = ioctl(so, FIONREAD, &nread);
        dbgprint("sock wake: %d\n", nread);
        if (-1 == err || !nread || nread < sizeof(struct pp_msg)) {
            // If there is no data, but select woke us up, then the client
            // must have terminated the connection on their end
            continue;
        }
        
        unsigned char *readbuf = malloc(nread);
        if (!readbuf) {
            errno = ENOMEM;
            perror("memory alloc failed");
            continue;
        }
        
        if (-1 == recv(so, readbuf, nread, 0)) {
            free(readbuf);
            perror("recv");
            continue;
        }
        
        pp_msg_t msgp;
        int32_t msgsize = 0, signal = 0;
        do {
            msgp = (pp_msg_t)(readbuf + msgsize);
            
            if (PP_MSG_MAGIC != msgp->ppm_magic || PP_MSG_VERSION != msgp->ppm_version ||
                PP_MSG_TYPE_LOG != msgp->ppm_type) {
                dbgprint("bad msg: m=%x, v=%d, t=%u\n", msgp->ppm_magic,
                    msgp->ppm_version, msgp->ppm_type);
                break;
            }
            msgsize = msgp->ppm_size;
            nread -= msgsize;
            
            h->sequence++;
            
            __typeof__(msgp->ppm_size) entrysize = msgp->ppm_size - sizeof(*msgp);
            int32_t entryct = entrysize / sizeof(pp_log_entry_t);
            
        #if 0
            if (msgp->ppm_logcount != entryct)
        #endif
            if (!entryct) {
                dbgprint("no entries for sequence %u (%d)\n", msgp->ppm_sequence, entrysize);
                continue;
            }
            dbgprint("%d entries in msg\n", entryct);
            
            pp_log_entry_t *ep = (pp_log_entry_t*)msgp->ppm_data;
            pp_log_entry_t *nep, *ehead = NULL, *etail = NULL;
            
            // sort the log array from oldest ts to newest
            qsort(ep, entryct, sizeof(*ep), log_compare);
            
            err = 0;
            int i;
            // process in reverse as we want the list
            // ordered from oldest to newest
            for (i=(entryct-1); i >= 0; --i) {
                if ((ep[i].pplg_flags & PP_LOG_ALLOWED) && !(h->flags & PP_LOG_ALLOWED))
                    continue;
                
                nep = malloc(sizeof(*nep));
                if (!nep) {
                    errno = ENOMEM;
                    perror("memory alloc failed");
                    break;
                }
                bcopy(&ep[i], nep, sizeof(*ep));
                nep->pplg_next = nep->pplg_prev = NULL;
                
                // insert onto our private list
                if (ehead) {
                    nep->pplg_next = ehead;
                } else {
                    etail = nep;
                }
                ehead = nep;
            }
            
            if (!ehead) {
                dbgprint("no list entries!\n");
                continue; // no entries created
            }
            dbgprint("recvlogmsg: added %d of %d entries\n", entryct - (i+1), entryct);
            
            pthread_mutex_lock(&h->lck);
        
            // insert our private list onto the q            
            if (h->logq_head)
                h->logq_tail->pplg_next = ehead;
            else
                h->logq_head = ehead;
             h->logq_tail = etail;
            
            h->entriesReceived += entryct;
            h->entriesQueued += entryct - (i+1);
                
            pthread_mutex_unlock(&h->lck);
            
            signal = 1;
        } while(nread > 0);
        
        if (signal)
            (void)pthread_cond_signal(&h->cond);
        
        free(readbuf);
        msgp = NULL;
    } while(1);
    
    pthread_mutex_lock(&h->lck);
    h->recvth = NULL;
    pthread_mutex_unlock(&h->lck);
    
    return (NULL);
}

static CFStringRef MapCopyName(const void *value)
{
    return (CFStringCreateWithCString(kCFAllocatorDefault,
        ((pp_name_map_t)value)->map->tblname, kCFStringEncodingUTF8));
}

static Boolean MapEqual(const void *value1, const void *value2)
{
    return (0 == pg_table_id_cmp(((pp_name_map_t)value1)->map->tblid, ((pp_name_map_t)value2)->map->tblid));
}

static void MapUnmap(const void *key __unused, const void *value, void *context __unused)
{
    (void)pp_unmap_names((pp_name_map_t)value);
}

static CFMutableDictionaryRef maps = NULL;
// It's possbile for one app to have multple processlogq threads (one for each log handle).
static pthread_mutex_t mapsLock = PTHREAD_MUTEX_INITIALIZER;
#define ml() pthread_mutex_lock(&mapsLock)
#define mul() pthread_mutex_unlock(&mapsLock)

static u_int32_t processLogEntries(const pp_log_handle_t h, pp_log_entry_t *ep)
{
    pp_name_map_t mh;
    char tableNameBuf[64];
    char *entryName, *tableName;
    pg_table_id_t tableid;
    u_int32_t entryct = 0;
    pp_log_entry_t *dep;
    
    while(ep) {
        entryName = tableName = NULL;
        pg_table_id_copy(ep->pplg_tableid, tableid);
        mh = NULL;
        
        if (0 != pg_table_id_cmp(PG_TABLE_ID_ALL, tableid)) {
            (void)pg_table_id_string(tableid, tableNameBuf, sizeof(tableNameBuf), 1);
            CFStringRef tableidStr = CFStringCreateWithCString(kCFAllocatorDefault, tableNameBuf,
                kCFStringEncodingASCII);
            
            ml();
            if (!maps || !tableidStr
                || !CFDictionaryGetValueIfPresent(maps, tableidStr, (const void**)&mh)) {
                if (0 == pp_map_names(tableid, &mh) && maps && tableidStr)
                    CFDictionaryAddValue(maps, tableidStr, mh);
            }
            mul();
            
            if (tableidStr)
                CFRelease(tableidStr);
        }
        
        if (mh) {
            tableName = tableNameBuf;
            strcpy(tableName, mh->map->tblname);
            if (pp_resolve_name(mh, ep->pplg_name_idx, &entryName))
                entryName = NULL;
            ml();
            if (!maps || !CFDictionaryContainsValue(maps, mh))
                (void)pp_unmap_names(mh);
            mul();
        }
        
        if (0 == (ep->pplg_flags & PP_LOG_TBL_EVT))
            h->logcb(ep, entryName, tableName);
        else if (h->tblcb)
            h->tblcb(ep, NULL, tableName);
        
        if (entryName)
            free(entryName);
        
        dep = ep;
        ep = dep->pplg_next;
        free(dep);
        entryct++;
    }
    
    return (entryct);
}

static void* processlogq(void* arg)
{
    const pp_log_handle_t h = (const pp_log_handle_t)arg;
    
    ml();
    if (!maps) {
        CFDictionaryValueCallBacks vcb = {0};
        vcb.copyDescription = MapCopyName;
        vcb.equal = MapEqual;
        
        maps = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &vcb);
    } else
        CFRetain(maps);
    mul();
    
    do {
        pp_log_entry_t *ep;
        
        pthread_mutex_lock(&h->lck);
        
        while (!h->logq_head) {
        	pthread_cond_wait(&h->cond, &h->lck);
            if (h->sock < 0)
                break;
        }
        
        ep = h->logq_head;
        h->logq_head = h->logq_tail = NULL; // we now "own" the list
        h->entriesQueued = 0;
        
        pthread_mutex_unlock(&h->lck);
        
        u_int32_t entryct = processLogEntries(h, ep);
        
        pthread_mutex_lock(&h->lck);
        h->entriesProcessed += entryct;
        pthread_mutex_unlock(&h->lck);
        
        dbgprint("processlogq: processed %d entries\n", entryct);
    } while(h->sock >= 0);
    
    ml();
    if (maps) {
        int lastref = 0;
        if (1 == CFGetRetainCount(maps)) {
            CFDictionaryApplyFunction(maps, MapUnmap, NULL);
            lastref = 1;
        }
        CFRelease(maps);
        if (lastref)
            maps = NULL;
    }
    mul();
    
    pthread_mutex_lock(&h->lck);
    h->processth = NULL;
    pthread_mutex_unlock(&h->lck);
    
    return (NULL);
}

PP_EXTERN
int pp_log_stats(pp_log_handle_t handle, pp_log_stats_t stats)
{
    pthread_mutex_lock(&handle->lck);
    
    stats->pps_entriesQueued = handle->entriesQueued;
    stats->pps_entriesReceived = handle->entriesReceived;
    stats->pps_entriesProcessed = handle->entriesProcessed;
    
    pthread_mutex_unlock(&handle->lck);
    return (0);
}

// Defining fmt_upper ourself lets us get rid of the '-' chars.
static const char *fmt_upper = 
	"%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X";

#include "uuid.c"

char*
pg_table_id_string(const pg_table_id_t tid, char *buf, size_t len __unused, int separators)
{
    if (separators)
        uuid_unparse(tid, buf);
    else
        uuid_unparse_upper(tid, buf);
    return (buf);
}

int pg_table_id_fromstr(const char *str, pg_table_id_t tid)
{
    CFStringRef uustr = str ? CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, str,
        kCFStringEncodingASCII, kCFAllocatorNull) : NULL;
    int err = pg_table_id_fromcfstr(uustr, tid);
    if (uustr)
        CFRelease(uustr);
    return (err);
    
}

int pg_table_id_fromcfstr(CFStringRef str, pg_table_id_t tid)
{
    bzero(tid, sizeof(tid));
    
    if (str) {
        CFUUIDRef uu = CFUUIDCreateFromString(kCFAllocatorDefault, str);
        if (uu) {
            CFUUIDBytes *bytes = (CFUUIDBytes*)tid;
            *bytes = CFUUIDGetUUIDBytes(uu);
            CFRelease(uu);
            return (0);
        }
    }
    
    return (EINVAL);
}

void pg_table_id_create(pg_table_id_t tid, CFStringRef *str)
{
    bzero(tid, sizeof(tid));
    
    CFUUIDRef uu = CFUUIDCreate(kCFAllocatorDefault);
    if (uu) {
    	CFUUIDBytes *bytes = (CFUUIDBytes*)tid;
        *bytes = CFUUIDGetUUIDBytes(uu);
        if (str)
            *str = CFUUIDCreateString(kCFAllocatorDefault, uu);
        CFRelease(uu);
    }
}

__private_extern__
int FindPIDByPathOrName(const char *path, const char *name, uid_t user)
{
    int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    struct kinfo_proc *procs;
    struct extern_proc *proc;
    char *pargs, *cmdpath;
    size_t plen, argmax, arglen;
    int err, i, argct, retry = 0;
    pid_t found = (pid_t)0, me;

    procs = nil;
    do {
        // Get needed buffer size
        plen = 0;
        err = sysctl(mib, 3, nil, &plen, nil, 0);
        if (err) {
            printf("get process size failed with: %d\n", err == -1 ? errno : err);
            break;
        }

        procs = (struct kinfo_proc *)malloc(plen);
        if (!procs) {
            printf("Failed to allocate memory\n");
            break;
        }

        // Get the process list
        err = sysctl(mib, 3, procs, &plen, nil, 0);
        if (-1 == err || (0 != err && ENOMEM == err)) {
            printf("get process list failed with: %d\n", err == -1 ? errno : err);
            break;
        }
        if (ENOMEM == err) {
            // buffer too small, try again
            free(procs);
            procs = nil;
            err = 0;
            retry++;
            if (retry < 3)
            continue;
            printf("Failed to obtain process list after %d tries.\n", retry);
            break;
        }

        // Get the max arg size for a command
        mib[1] = KERN_ARGMAX;
        mib[2] = 0;
        arglen  = sizeof(argmax);
        err = sysctl(mib, 2, &argmax, &arglen, nil, 0);
        if (err) {
            printf("get process arg size failed with: %d\n", err == -1 ? errno : err);
            break;
        }

        // Allocate space for args
        pargs = malloc(argmax);
        if (!pargs) {
            printf("Failed to allocate memory\n");
            break;
        }

        // Walk the list and find the process
        me = getpid();
        for (i=0; i < plen / sizeof(struct kinfo_proc); ++i) {
            proc = &(procs+i)->kp_proc;
            if (proc->p_pid == me)
                continue;
            if (ANY_USER != user && (procs+i)->kp_eproc.e_pcred.p_ruid != user)
                continue;

            // Get the process arguments
            mib[1] = KERN_PROCARGS2;
            mib[2] = proc->p_pid;
            arglen = argmax;
            if (0 == sysctl(mib, 3, pargs, &arglen, nil, 0)) {
                // argc is first for KERN_PROCARGS2
                bcopy(pargs, &argct, sizeof(argct));
                // the command path is next
                cmdpath = (pargs+sizeof(argct));
                *(pargs+(arglen-1)) = '\0'; // Just in case
                if ((path && 0 == strcmp(cmdpath, path)) ||
                    (name && strstr(cmdpath, name))) {
                    found = proc->p_pid;
                    break;
                }
            }
        }
        free(pargs);
        break;

    } while(1);

    if (procs)
        free(procs);
    return (found);
}

#endif // __APPLE__

const pp_msg_iprange_t* pp_match_addr(const pp_msg_iprange_t *addrs, int32_t addrcount, in_addr_t ip4)
{
    int32_t h=addrcount, l=-1, idx;
    
    while (h-l > 1) {
        idx = ((unsigned)(h+l)) >> 1;
        if (addrs[idx].p2_ipstart > ip4)
            h = idx;
        else
            l = idx;
    }
    
    if (l > -1 && ip4 >= addrs[l].p2_ipstart && ip4 <= addrs[l].p2_ipend)
        return (&addrs[l]);
    
    return (NULL);
}

int pp_enum_table(const pp_msg_iprange_t *addrs, int32_t addrcount, u_int8_t* const *names,
    pp_enum_callback_t cb, void *enumContext)
{
    struct in_addr start, end;            
    
    int i;
    for (i=0; i < addrcount; ++i) {
        start.s_addr = htonl(addrs[i].p2_ipstart);
        end.s_addr = htonl(addrs[i].p2_ipend);
        
        char startstr[64], endstr[64];
        (void)pp_inet_ntoa(start, startstr, sizeof(startstr)-1);
        (void)pp_inet_ntoa(end, endstr, sizeof(endstr)-1);
        startstr[sizeof(startstr)-1] = endstr[sizeof(endstr)-1] = 0;
        cb((const char*)names[addrs[i].p2_name_idx], startstr, endstr, addrs[i].p2_name_idx, enumContext);
    }
    return (0);
}

static
void pp_print_cb(const char *name, const char *ipStart, const char *ipEnd, int32_t index,
    __unused void *enumContext)
{
    printf("%s:%s-%s\n", name, ipStart, ipEnd);
}

int pp_print_table(const pp_msg_iprange_t *addrs, int32_t addrcount, u_int8_t* const *names)
{
    pp_enum_table(addrs, addrcount, names, pp_print_cb, NULL);
    return (0);
}

char* pp_inet_ntoa(struct in_addr addr, char *b, size_t len)
{
   char *p;

   p = (char *)&addr;
#define UC(b)        (((int)b)&0xff)
   (void)snprintf(b, len,
      "%d.%d.%d.%d", UC(p[0]), UC(p[1]), UC(p[2]), UC(p[3]));
#undef UC
   return (b);
}

