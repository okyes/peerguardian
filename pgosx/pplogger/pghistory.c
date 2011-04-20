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

#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <mach/mach_init.h>
#include <libkern/OSByteOrder.h>

#define Swap16 OSSwapInt16
#define Swap32 OSSwapInt32
#define Swap64 OSSwapInt64

#include "pghistory.h"
#include "pplib.h"

// This will reset the fd offset to 0
static int pg_hist_trunc(int fd, u_int64_t size, u_int64_t max)
{
    unsigned char *buf = malloc(vm_page_size);
    if (!buf)
        return (ENOMEM);
    
    (void)lseek(fd, 0LL, SEEK_SET);
    pg_hist_hdr_t *hdr = (pg_hist_hdr_t*)buf;
    int err = 0;
    ssize_t bytesRead = read(fd, hdr, sizeof(*hdr));
    if (sizeof(*hdr) != bytesRead) {
        err = errno;
        goto trunc_exit;
    }
    
    if (!(PG_HIST_MAGIC == hdr->ph_magic && PG_HIST_VER == hdr->ph_version)
        && !(PG_HIST_MAGIC == Swap32(hdr->ph_magic) && PG_HIST_VER == Swap32(hdr->ph_version))) {
        err = ENOTSUP;
        goto trunc_exit;
    }
    
    // compute total size of entries
    size = ((size - sizeof(*hdr)) / sizeof(pg_hist_ent_t)) * sizeof(pg_hist_ent_t);
    // we cut max in half to leave room for new entries
    max = (((max - sizeof(*hdr)) / sizeof(pg_hist_ent_t)) * sizeof(pg_hist_ent_t)) / 2;
    u_int64_t off = (size - max) + sizeof(*hdr);
    
    // overwrite old entries with the latest ones
    size += sizeof(*hdr);
    while ((bytesRead = pread(fd, buf, vm_page_size, off)) > 0 && off < size) {
        u_int32_t magic = ((pg_hist_ent_t*)buf)->ph_emagic;
        if (PG_HIST_ENT_MAGIC != magic
            && PG_HIST_ENT_MAGIC != (magic = Swap32(magic))) {
            dbgbrk();
            break;
        }
        off += bytesRead;
        if (bytesRead != write(fd, buf, bytesRead)) {
            err = errno;
            break;
        }
    }
    if (-1 != bytesRead) {
        ppassert(off == size);
        (void)ftruncate(fd, max + sizeof(*hdr));
    } else
        err = errno;
                            
trunc_exit:
    (void)lseek(fd, 0LL, SEEK_SET);
    free(buf);
    return (err);
}

int pg_hist_open(const char *path, int rdonly, u_int64_t max)
{
    int fd;
hist_reopen:
    fd = open(path, rdonly ? O_RDONLY : O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fd > -1) {
        if (!rdonly)
            (void)fcntl(fd, F_NOCACHE, 1);
        (void)fcntl(fd, F_RDAHEAD, rdonly ? 1 : 0);
        struct stat sb;
        if (-1 != fstat(fd, &sb)) {
            if (sb.st_size > max && !rdonly) {
                pg_hist_trunc(fd, sb.st_size, max);
            }
            
            pg_hist_hdr_t hdr;
            if (sb.st_size && sizeof(hdr) == read(fd, &hdr, sizeof(hdr))) {
                if (BYTE_ORDER != hdr.ph_byteorder && !rdonly) {
                    close(fd);
                    fd = -1;
                    int len = strlen(path);
                    char tmp[len+2];
                    strcpy(tmp, path);
                    tmp[len++] = '~';
                    tmp[len] = 0;
                    
                    if (0 == pg_hist_byteswap(path, tmp)) {
                        unlink(path);
                        link(tmp, path);
                        unlink(tmp);
                        goto hist_reopen;
                    }
                }
                
                // Validate header
                if (PG_HIST_MAGIC == hdr.ph_magic
                    // XXX need to support verson upgrade
                    && PG_HIST_VER == hdr.ph_version) {
                    if (!rdonly)
                        (void)lseek(fd, 0LL, SEEK_END);
                    return (fd);
                }
            }
            
            // file's no good
            if (!rdonly) {
                (void)ftruncate(fd, 0);
                hdr.ph_byteorder = BYTE_ORDER;
                hdr.ph_magic = PG_HIST_MAGIC;
                hdr.ph_version = PG_HIST_VER;
                if (sizeof(hdr) == write(fd, &hdr, sizeof(hdr)))
                    return (fd);
            }
        }
        close(fd);
    }
    return (-1);
}

int pg_hist_find_entry(int fd, u_int64_t timestamp, u_int64_t *offp)
{
    if (-1 == fd || !offp)
        return (EINVAL);
    
    struct stat sb;
    if (-1 == fstat(fd, &sb))
        return (errno);
    
    sb.st_size -= sizeof(pg_hist_hdr_t);
    
    if (0ULL == timestamp && sb.st_size > 0) {
        *offp = sizeof(pg_hist_hdr_t);
        return (0);
    }
    
    pg_hist_ent_t ent;
    int64_t h = sb.st_size / sizeof(ent);
    int64_t l = -1LL, idx;
    u_int64_t off;
    while (h-l > 1LL) {
        idx = ((u_int64_t)(h+l)) >> 1;
        off = sizeof(pg_hist_hdr_t) + (idx * sizeof(ent));
        if (sizeof(ent) != pread(fd, &ent, sizeof(ent), off)) {
            l = -1LL;
            goto entry_bad;
            break;
        }
        if (PG_HIST_ENT_MAGIC != ent.ph_emagic) {
            l = -1LL;
            dbgbrk();
            break;
        }
        if (ent.ph_timestamp > timestamp)
            h = idx;
        else
            l = idx;
    }
    
    if (l > -1) {
        if (ent.ph_timestamp < timestamp)
            off += sizeof(ent);
entry_found:
        *offp = off;
        return (0);
    } else {
        off = sizeof(pg_hist_hdr_t);
        if (sizeof(ent) == (l = pread(fd, &ent, sizeof(ent), off))) {
            if (ent.ph_timestamp >= timestamp)
                goto entry_found;
        }
    }
    
entry_bad:
    *offp = 0;
    return (ENOENT);
    
}

int pg_hist_byteswap(const char *in, const char *out)
{
    if (!in || !out || 0 == strcasecmp(in, out))
        return (EINVAL);
    
    int ofd = -1;
    const int infd = open(in, O_RDONLY|O_EXCL);
    if (-1 == infd)
        return (errno);
    
    int err = -1;
    pg_hist_ent_t *entries = NULL;
#ifdef notyet
    struct stat sb;
    if (-1 == fstat(infd, &sb)) {
        err = errno;
        goto swap_exit;
    }
#endif
    
    pg_hist_hdr_t hdr;
    if (sizeof(hdr) != read(infd, &hdr, sizeof(hdr))) {
        goto swap_exit;
    }
    
    if (BYTE_ORDER == hdr.ph_byteorder) {
        if (PG_HIST_MAGIC == hdr.ph_magic) {
            err = 0;
        } else
            err = -1;
        goto swap_exit;
    }
    
    if (PG_HIST_MAGIC != Swap32(hdr.ph_magic)) {
        err = -1;
        goto swap_exit;
    }
    
    // Open the output file and write the header
    if (-1 == (ofd = open(out, O_WRONLY|O_CREAT|O_TRUNC|O_EXCL, S_IRUSR|S_IWUSR))) {
        err = errno;
        goto swap_exit;
    }
    hdr.ph_byteorder = BYTE_ORDER;
    hdr.ph_magic = PG_HIST_MAGIC;
    hdr.ph_version = Swap32(hdr.ph_version);
    if (sizeof(hdr) != write(ofd, &hdr, sizeof(hdr))) {
        err = errno;
        goto swap_exit;
    }
    
    // Process the entries - XXX need to handle different versions here
    entries = malloc(PG_HIST_BLK_SIZE);
    if (!entries) {
        err = ENOMEM;
        goto swap_exit;
    }
    
    int bytesRead, bytesToWrite, done = 0;
    err = 0;
    while ((bytesRead = read(infd, entries, PG_HIST_BLK_SIZE)) > 0 && !done) {
        pg_hist_ent_t *ent = entries;
        done = (bytesRead % sizeof(pg_hist_ent_t));
        bytesRead -= sizeof(pg_hist_ent_t);
        bytesToWrite = 0;
        while (bytesRead >= done) {
            ent->ph_emagic = Swap16(ent->ph_emagic);
            ppassert(PG_HIST_ENT_MAGIC == ent->ph_emagic);
            ent->ph_timestamp = Swap64(ent->ph_timestamp);
            ent->ph_flags = Swap16(ent->ph_flags);
            ent->ph_port = Swap16(ent->ph_port);
            ent->ph_rport = Swap16(ent->ph_rport);
            ent++;
            bytesRead -= sizeof(pg_hist_ent_t);
            bytesToWrite += sizeof(pg_hist_ent_t);
        }
        
        if (bytesToWrite && (bytesToWrite != write(ofd, entries, bytesToWrite))) {
            err = errno;
            break;
        }
    }
    if (-1 == bytesRead && !done)
        err = errno;
    
swap_exit:
    if (entries)
        free(entries);
    if (-1 != infd)
        close(infd);
    if (-1 != ofd) {
        close(ofd);
        if (err)
            (void)unlink(out);
    }
    
    return (err);
}
