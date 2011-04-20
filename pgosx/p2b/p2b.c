/*
    Copyright 2005,2006,2008 Brian Bergstrand

    This program is free software; you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program;
    if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#ifdef __APPLE__
#define HAVECF 1
#endif

#ifdef HAVECF
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "p2b.h"
#include "pplib.h"

// Flags for p2b_open()
#define P2B_OPEN_TRUNCATE 0x0001
#define P2B_OPEN_NOCACHE  0x0002

#define P2B_OPEN_MAKE_VERSION(flags, version) ((version) |= ((flags) << 16))

static const u_int8_t p2b_hdr_sig[] = {0xff,0xff,0xff,0xff,'P','2','B'};
struct p2bhdr {
    u_int8_t sig[sizeof(p2b_hdr_sig)];
    u_int8_t version;
};

#define P2B_MAX_FILE_SIZE (128 << 10 << 10)

struct p2iaddr {
    u_int32_t ipstart;
    u_int32_t ipend;
    u_int8_t *name;
    struct p2iaddr *next;
};

static int
p2b_range_compare(const void *r1, const void *r2)
{
    if (((const pp_msg_iprange_t*)r1)->p2_ipstart == ((const pp_msg_iprange_t*)r2)->p2_ipstart)
        return (0);
    else if (((const pp_msg_iprange_t*)r1)->p2_ipstart < ((const pp_msg_iprange_t*)r2)->p2_ipstart)
        return (-1);
    else
        return (1);
}

#ifdef HAVECF
static Boolean NameCacheValuesEqual(const void *value1, const void *value2)
{
    return ((int)value1 == (int)value2);
}
#endif

// returns NULL if successful, otherwise returns new head ptr
static struct p2iaddr*
p2b_list_to_arrays(struct p2iaddr *head, int ct, pp_msg_iprange_t *addrs, u_int8_t **names, int *namect)
{
    struct p2iaddr *entry = head;
    int i, namei;
    *namect = 0;
    
#ifdef HAVECF
    CFDictionaryValueCallBacks vcb = {0};
    vcb.equal = NameCacheValuesEqual;
    CFMutableDictionaryRef nameCache = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
        &kCFTypeDictionaryKeyCallBacks, &vcb);
#endif
    
    // setup the first name ptr
    names[0] = ((u_int8_t*)names + ((ct+1) * sizeof(u_int8_t*)));
    for (i=0, namei=0; entry; ++i) {
#ifdef HAVECF
        CFStringRef name = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, (char*)entry->name,
            kCFStringEncodingUTF8, kCFAllocatorNull);
        if (!name || FALSE == CFDictionaryContainsKey(nameCache, name)) {
#endif
            // not found, make a new entry
            strcpy((char*)names[namei], (char*)entry->name);
            // setup the ptr for the next name
            names[namei+1] = names[namei] + (strlen((char*)names[namei]) + 1);
            addrs[i].p2_name_idx = namei;
            
#ifdef HAVECF
            if (name) {
                CFRelease(name);
                name = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, (char*)names[namei],
                    kCFStringEncodingUTF8, kCFAllocatorNull);
                if (name)
                    CFDictionaryAddValue(nameCache, name, (void*)namei);
            }
#endif            
            namei++;
            (*namect)++;
#ifdef HAVECF
        } else {
            int idx = (int)CFDictionaryGetValue(nameCache, name);
            addrs[i].p2_name_idx = idx;
        }
        
        if (name)
            CFRelease(name);
#endif        

        addrs[i].p2_ipstart = entry->ipstart;
        addrs[i].p2_ipend = entry->ipend;
        
        head = entry->next;
        free(entry);
        entry = head;
    };
    
#ifdef HAVECF
    CFRelease(nameCache);
#endif
    
    return (entry);
}

static
int p2b_parsev2(u_int8_t *buf, int sz, pp_msg_iprange_t **addrspp, int32_t *addrcount, u_int8_t ***namesppp)
{
    int err = 0;
    u_int32_t entryct = 0, totalNameBytes = 0; 
    u_int8_t *p = buf;
    struct p2iaddr *head = NULL, *last = NULL, *entry;
    do {
        entry = malloc(sizeof(*head));
        if (!entry) {
            err = ENOMEM;
            break;
        }
        // utf8 name first
        if (strlen((char*)p))
            entry->name = p;
        else
            entry->name = (typeof(entry->name))"Unknown";
        do {} while(*p++ && (p < (buf+sz)));
        if (p >= (buf+sz)) {
            free(entry);
            break;
        }
        
        entry->ipstart = ntohl(*(u_int32_t*)p);
        p += sizeof(u_int32_t);
        entry->ipend = ntohl(*(u_int32_t*)p);
        p += sizeof(u_int32_t);
        
        if ((entry->ipstart > entry->ipend)
            || (0 == entry->ipstart && 0 == entry->ipend)) {
            fprintf(stderr, "-p2b_parsev2- invalid entry: %s:%x-%x\n", entry->name,
                entry->ipstart, entry->ipend);
            free(entry);
            continue;
        }
        
        if (!entry->ipend)
            entry->ipend = entry->ipstart;
        
        entryct++;
        totalNameBytes += strlen((char*)entry->name) + 1 /*NULL*/;
        
        if (last) {
            last->next = entry;
            last = entry;
        } else
            head = last = entry;
    } while(p < (buf+sz));
    
    if (!head)
        err = ERANGE;
    
    if (!err) {
        last->next = NULL;
        u_int8_t **names = malloc((sizeof(u_int8_t*)*(entryct+1)) + totalNameBytes);
        pp_msg_iprange_t *addrs = NULL;
        err = ENOMEM;
        if (names) {
            addrs = malloc(sizeof(*addrs) * (entryct+1));
            if (addrs) {
                // p2b_list_to_arrays frees the list entries
                int namect;
                if (!(head = p2b_list_to_arrays(head, entryct, addrs, names, &namect))) {
                    bzero(&addrs[entryct], sizeof(*addrs));
                    addrs[entryct].p2_name_idx = -1;
                    names[entryct] = names[namect] = NULL;
                    *addrspp = addrs;
                    *addrcount = entryct;
                    *namesppp = names;
                    return (0);
                }
            }
        }
        // if we get here, something failed
        if (names)
            free(names);
        if (addrs)
            free(addrs);
    }
    
    entry = head;
    while (entry) {
        head = entry->next;
        free(entry);
        entry = head;
    }
    
    return (err);
}

static
int p2b_parsev3(u_int8_t *buf, int sz, pp_msg_iprange_t **addrspp, int32_t *addrcount, u_int8_t ***namesppp)
{
    u_int32_t namect = ntohl(*(u_int32_t*)buf);
    int err = 0;
    if (namect) {
        u_int32_t i;
        u_int8_t *p = buf;
        p += sizeof(u_int32_t); // for namect
        
        u_int32_t totalNameBytes = 0;
        for (i=0; i < namect; ++i) {
            totalNameBytes += strlen((char*)(p+totalNameBytes)) + 1;
        }
        // once more to copy the names
        u_int8_t **names = malloc((sizeof(u_int8_t*)*(namect+1)) + totalNameBytes); // array of ptrs
        if (!names)
            return (ENOMEM);
        names[0] = ((u_int8_t*)names + (namect * sizeof(u_int8_t*)));
        for (i=0; i < namect; ++i) {
            strcpy((char*)names[i], (char*)p);
            p += (strlen((char*)names[i]) + 1);        
            // setup the ptr for the next name
            names[i+1] = names[i] + (strlen((char*)names[i]) + 1);
        }
        
        // Range count
        u_int32_t rangect = ntohl(*(u_int32_t*)p);
        if (!rangect) {
            free(names);
            return (ERANGE);
        }
        p += sizeof(u_int32_t);
        
        pp_msg_iprange_t *addrs = malloc(sizeof(*addrs) * (rangect+1));
        if (!addrs) {
            free(names);
            return (ENOMEM);
        }
        for (i=0; i < rangect; ++i) {
            addrs[i].p2_name_idx = ntohl(*(u_int32_t*)p);
            p += sizeof(u_int32_t);
            
            if (addrs[i].p2_name_idx >= namect) {
                err = E2BIG;
                break;
            }
            
            addrs[i].p2_ipstart = ntohl(*(u_int32_t*)p);
            p += sizeof(u_int32_t);
            addrs[i].p2_ipend = ntohl(*(u_int32_t*)p);
            p += sizeof(u_int32_t);
            
            if ((addrs[i].p2_ipstart > addrs[i].p2_ipend)
                || (0 == addrs[i].p2_ipstart && 0 == addrs[i].p2_ipend)) {
                fprintf(stderr, "-p2b_parsev3- invalid entry: %s:%x-%x\n",
                    names[addrs[i].p2_name_idx], addrs[i].p2_ipstart, addrs[i].p2_ipend);
                addrs[i].p2_ipstart = addrs[i].p2_ipend = INADDR_NONE;
                addrs[i].p2_name_idx = -1;
            }
            
            if (0 == addrs[i].p2_ipend)
                addrs[i].p2_ipend = addrs[i].p2_ipstart;
        }
        
        if (!err) {
            addrs[rangect].p2_name_idx = -1;
            names[namect] = NULL;
            *addrspp = addrs;
            *addrcount = rangect;
            *namesppp = names;
        } else {
            free(names);
            free(addrs);
        }
    } else {
        err = ERANGE;
    }
    return (err);
}

static
int p2b_parsetxt(u_int8_t *buf, int sz, pp_msg_iprange_t **addrspp, int32_t *addrcount,
    int32_t *badaddrs, u_int8_t ***namesppp)
{
    int err = 0;
    u_int32_t entryct = 0, totalNameBytes = 0; 
    u_int8_t *p = buf;
    u_int8_t *nextRecord, *ipstart, *ipend;
    in_addr_t ip4;
    struct p2iaddr *head = NULL, *last = NULL, *entry = NULL;
    *badaddrs = 0;
    do {
        if (entry) {
            *badaddrs += 1;
            free(entry);
        }
        
        entry = malloc(sizeof(*entry));
        if (!entry) {
            err = ENOMEM;
            break;
        }
        
        nextRecord = p;
        (void)strsep((char**)&nextRecord, "\n\r");
        if (nextRecord && (*nextRecord == '\n' || *nextRecord == '\r'))
            nextRecord++; // DOS CR+LF, skip over
        
        // Text lists can contain the record separator token (':') in the description (dumb).
        // So search backwards for it and stop at the first one found.
        ipstart = (p + (strlen((char*)p) - 1));
        for (; ipstart >= p && *ipstart != ':'; --ipstart) /* dead loop */ ;
        if (ipstart < p) {
            goto parse_txt_bad; // bad record
        }
        *ipstart = 0; // lose the ':'
        ipstart++;
        
        ipend = ipstart;
        (void)strsep((char**)&ipend, "-");
        if (!ipend) {
            goto parse_txt_bad; // bad record
        }
        
        if (strlen((char*)p))
            entry->name = p;
        else
            entry->name = (typeof(entry->name))"Unknown";
        
        // Catches entries such as:
        // Bogon:203.56.0.0-20
        // These are invalid for us, but apparently, inet_addr doesn't mind.
        int dots = 0;
        char *nextchr = (char*)ipstart;
        while ((nextchr = strchr(nextchr, '.'))) {
            nextchr++;
            dots++;
        }
        if (3 != dots) {
            fprintf(stderr, "-p2b_parsetxt- invalid format: %s:%s-%s\n", entry->name,
                ipstart, ipend);
            goto parse_txt_bad; // bad record
        }
        dots = 0;
        nextchr = (char*)ipend;
        // For ipend, we check for any extraneous junk after the address
        while (isdigit(*nextchr) || '.' == *nextchr) {
            nextchr++;
            if ('.' == *nextchr)
                dots++;
        }
        if (3 != dots) {
            fprintf(stderr, "-p2b_parsetxt- invalid format: %s:%s-%s\n", entry->name,
                ipstart, ipend);
            goto parse_txt_bad; // bad record
        }
        if (*nextchr)
            *nextchr = 0;
        
        // Convert addrs to binary
        if (0 == inet_aton((char*)ipstart, (struct in_addr*)&ip4)) {
            goto parse_txt_bad; // bad record
        }
        entry->ipstart = ntohl(ip4);
        
        if (0 == inet_aton((char*)ipend, (struct in_addr*)&ip4)) {
            goto parse_txt_bad; // bad record
        }
        entry->ipend = ntohl(ip4);
        
        if ((entry->ipstart > entry->ipend)
            || (0 == entry->ipstart && 0 == entry->ipend)) {
            fprintf(stderr, "-p2b_parsetxt- invalid entry: %s:%x-%x\n", entry->name,
                entry->ipstart, entry->ipend);
            goto parse_txt_bad; // bad record
        }

        if (0 == entry->ipend)
            entry->ipend = entry->ipstart;
        
        entryct++;
        totalNameBytes += strlen((char*)entry->name) + 1 /*NULL*/;
        
        if (last) {
            last->next = entry;
            last = entry;
        } else
            head = last = entry;
        
        entry = NULL;
        
parse_txt_bad:
        p = nextRecord;
    } while(p && p < (buf+sz));
    
    if (!head)
        err = ERANGE;
    
    if (!err) {
        last->next = NULL;
        u_int8_t **names = malloc((sizeof(u_int8_t*)*(entryct+1)) + totalNameBytes);
        pp_msg_iprange_t *addrs = NULL;
        err = ENOMEM;
        if (names) {
            addrs = malloc(sizeof(*addrs) * (entryct+1));
            if (addrs) {
                // p2b_list_to_arrays frees the list entries
                int namect;
                if (!(head = p2b_list_to_arrays(head, entryct, addrs, names, &namect))) {
                    bzero(&addrs[entryct], sizeof(*addrs));
                    addrs[entryct].p2_name_idx = -1;
                    names[entryct] = names[namect] = NULL;
                    *addrspp = addrs;
                    *addrcount = entryct;
                    *namesppp = names;
                    return (0);
                }
            }
        }
        // if we get here, something failed
        if (names)
            free(names);
        if (addrs)
            free(addrs);
    }
    
    entry = head;
    while (entry) {
        head = entry->next;
        free(entry);
        entry = head;
    }
    
    return (err);
}

static
void p2b_sort(pp_msg_iprange_t *addrs, int32_t *rangecount, u_int32_t *addrcount)
{
    // sort the array by starting ip address
    qsort(addrs, *rangecount, sizeof(*addrs), p2b_range_compare);
    // The bluetack files have quite a few duplicates and overlapping ranges,
    // while blocklist.org files are generally well behaved.
    u_int32_t addrct = 0;
    int i, j, count = *rangecount;
    pp_msg_iprange_t *rp = addrs;
    for (i = 0, j = 1; i < count && rp[j].p2_name_idx != -1; ++j) {
        // Merge any duplicate start addresses
        if (rp[i].p2_ipstart == rp[j].p2_ipstart) {
            if (rp[i].p2_ipend < rp[j].p2_ipend)
                rp[i].p2_ipend = rp[j].p2_ipend;
            // Set entry addrs to invalid -- when we re-sort, this entry
            // will bubble to the top
            rp[j].p2_ipstart = rp[j].p2_ipend = INADDR_NONE;
            *rangecount -= 1;
            continue; // don't bump i as j+n may be dups too
        }
        // Merge any overlapping/inclusive ranges.
    #ifdef PPDBG
        if (rp[i].p2_ipstart >= rp[j].p2_ipstart)
            trap();
    #endif
        if (rp[j].p2_ipstart < rp[i].p2_ipend && rp[j].p2_ipend > rp[i].p2_ipend)
            rp[i].p2_ipend = rp[j].p2_ipend;
        if (rp[i].p2_ipend >= rp[j].p2_ipend) {
            rp[j].p2_ipstart = INADDR_NONE;
            *rangecount -= 1;
            continue; // don't bump i as j+n may be dups too
        }
        
        addrct += (rp[i].p2_ipend - rp[i].p2_ipstart) + 1;
        if (j-i != 1)
            i=j;
        else
            ++i;
    }
    if (i < count)
        addrct += (rp[i].p2_ipend - rp[i].p2_ipstart) + 1; // add the last range
    
    if (*rangecount < count) {
        // some dup ranges found, re-sort the array
        qsort(addrs, count, sizeof(*addrs), p2b_range_compare);
        // Set the token at the new EOF
        rp[*rangecount].p2_name_idx = -1;
    }
#ifdef PPDBG
    u_int32_t addrct2 = 0;
    count = *rangecount;
    for (i = 0; i < count; ++i)
        addrct2 += (rp[i].p2_ipend - rp[i].p2_ipstart) + 1;
    if (addrct != addrct2)
        trap();
#endif
    if (addrcount)
        *addrcount = addrct;
}

static
int p2b_fparse(int fd, pp_msg_iprange_t **addrspp, int32_t *rangecount,
    u_int32_t *addrcount, u_int8_t ***namesppp)
{
    struct p2bhdr hdr;
    int err, trytext = 0;
    
    u_int8_t *buf = NULL;
    int bytes = pread(fd, &hdr, sizeof(hdr), 0);
    if (bytes != sizeof(hdr)
        || 0 != memcmp(p2b_hdr_sig, hdr.sig, sizeof(p2b_hdr_sig))) {
        trytext = 1;
    }
    
    struct stat sb;
    if (-1 == fstat(fd, &sb)) {
        err = errno;
        goto parse_exit;
    }
    if (sb.st_size > P2B_MAX_FILE_SIZE) {
        err = EFBIG;
        goto parse_exit;
    }
    
    if (!trytext)
        bytes = (int)sb.st_size - sizeof(hdr);
    else
        bytes = (int)sb.st_size;
    if (!(buf = malloc(bytes + 4 /*fudge for the text case*/))) {
        err = ENOMEM;
        goto parse_exit;
    }
            
    bytes = pread(fd, buf, bytes, !trytext ? sizeof(hdr) : 0);
    if (-1 == bytes) {
        err = errno;
        goto parse_exit;
    }
    
    switch (hdr.version) {
        case P2B_VERSION2:
            err = p2b_parsev2(buf, bytes, addrspp, rangecount, namesppp);
        break;
        
        case P2B_VERSION3:
            err = p2b_parsev3(buf, bytes, addrspp, rangecount, namesppp);
        break;
        
        default:
            if (trytext) {
                int32_t badaddrs;
                strcpy((char*)&buf[bytes], "\r\n"); // make sure we have a stop point
                err = p2b_parsetxt(buf, bytes, addrspp, rangecount, &badaddrs, namesppp);
            } else
                err = ENOTSUP;
        break;
    }
    
    if (!err)
        p2b_sort(*addrspp, rangecount, addrcount);

parse_exit:
    if (buf)
        free(buf);
    
    return (err);
}

int p2b_parse(const char *path, pp_msg_iprange_t **addrspp, int32_t *rangecount,
    u_int32_t *addrcount, u_int8_t ***namesppp)
{
    int fd, err;
    
    if (-1 == (fd = open(path, O_RDONLY, 0)))
        return (errno);
    
    (void)fcntl(fd, F_NOCACHE, 1);
    (void)fcntl(fd, F_RDAHEAD, 1);
    err = p2b_fparse(fd, addrspp, rangecount, addrcount, namesppp);
    
    close(fd);
    return (err);
}

int p2b_parseports(char *portRule, int portRuleLen, pp_msg_iprange_t **addrspp, int32_t *rangecount)
{
    char *p = portRule;
    char *nextRecord, *endport;
    in_addr_t port;
    struct p2iaddr *head = NULL, *last = NULL, *entry = NULL;
    int err = 0, entryct = 0;
    
    *addrspp = NULL;
    do {
        entry = malloc(sizeof(*head));
        if (!entry) {
            err = ENOMEM;
            break;
        }
        entry->name = NULL;
        
        nextRecord = p;
        (void)strsep((char**)&nextRecord, ",");
        
        endport = p;
        (void)strsep((char**)&endport, "-");
        
        int i;
        for (i = 0; p[i] != 0; ++i) {
            if (!isdigit(p[i]) && ' ' != p[i]) {
                p[0] = ';'; // make sure strtoul fails
                break;
            }
        }
        port = strtoul(p, NULL, 10);
        if (!port && errno) {
            err = errno;
            break;
        }
        if (port > 65535) {
            err = EINVAL;
            break;
        }
        entry->ipstart = port;
        
        if (endport) {
            for (i = 0; endport[i] != 0; ++i) {
                if (!isdigit(endport[i]) && ' ' != p[i]) {
                    p[0] = ';'; // make sure strtoul fails
                    break;
                }
            }
            port = strtoul(endport, NULL, 10);
            if (!port && errno) {
                err = errno;
                break;
            }
            if (port > 65535) {
                err = EINVAL;
                break;
            }
            entry->ipend = port;
        } else
            entry->ipend = entry->ipstart;
        
        if (entry->ipend && entry->ipstart > entry->ipend) {
            port = entry->ipstart;
            entry->ipstart = entry->ipend;
            entry->ipend = port;
        }
        
        if (!entry->ipstart)
            break;
        
        if (!entry->ipend)
            entry->ipend = entry->ipstart;
        
        if (last) {
            last->next = entry;
            last = entry;
        } else
            head = last = entry;
        
        entry = NULL;
        ++entryct;
        p = nextRecord;
        
    } while(p && p < (portRule+portRuleLen));
    
    if (entry)
        free(entry);
    
    if (!head)
        err = ERANGE;
    
    if (!err) {
        last->next = NULL;
        
        pp_msg_iprange_t *addrs = malloc(sizeof(*addrs) * (entryct+1));
        err = ENOMEM;
        if (addrs) {
            // copy the entries to the array
            int i;
            entry = head;
            for (i=0; entry; ++i) {
                addrs[i].p2_ipstart = entry->ipstart;
                addrs[i].p2_ipend = entry->ipend;
                addrs[i].p2_name_idx = 0;
                
                head = entry->next;
                free(entry);
                entry = head;
            }
            bzero(&addrs[entryct], sizeof(*addrs));
            addrs[entryct].p2_name_idx = -1;
            
            p2b_sort(addrs, &entryct, NULL);
            *rangecount = entryct;
            *addrspp = addrs;
            return (0);
        }
    }
    
    entry = head;
    while (entry) {
        head = entry->next;
        free(entry);
        entry = head;
    }
    
    return (err);
}

// compares two files -- returns 0 if ==, -1 if != and > 0 if an error occured
int p2b_compare(const char *path1, const char *path2)
{
    int err;
    pp_msg_iprange_t *r1 = NULL, *r2 = NULL;
    u_int8_t **names1 = NULL, **names2 = NULL;
    int32_t count1, count2;
    
    if (!(err = p2b_parse(path1, &r1, &count1, NULL, &names1))) {
        err = p2b_parse(path2, &r2, &count2, NULL, &names2);
        if (!err && count1 != count2)
            err = -1;
        if (0 != err)
            goto p2b_compare_exit;
        
        int i;
        // the counts are the same so there is no need to check i < count2
        for (i = 0; i < count1; ++i) {
            if (r1[i].p2_ipstart != r2[i].p2_ipstart || r1[i].p2_ipend != r2[i].p2_ipend
                || 0 != strcmp((char*)names1[r1[i].p2_name_idx], (char*)names2[r2[i].p2_name_idx])) {
                err = -1;
                break;
            }
        }
    }
    
p2b_compare_exit:
    if (r1) free(r1);
    if (r2) free(r2);
    if (names1) free(names1);
    if (names2) free(names2);
    
    if (ERANGE == err) // no entries in file
        err = -1;
    
    return (err);
}

int p2b_exchange(const char *source, const char *target)
{
    int err = 0;
    int srcFile = open(source, O_RDONLY);
    int tgtFile = -1;
    struct stat sb;
    if (srcFile > -1) {
        if (0 == fstat(srcFile, &sb)) {
            if ((tgtFile = open(target, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR)) > -1) {
                char *buf = malloc(sb.st_blksize);
                if (buf) {
                    (void)fcntl(srcFile, F_NOCACHE, 1);
                    (void)fcntl(srcFile, F_RDAHEAD, 1);
                    (void)fcntl(tgtFile, F_NOCACHE, 1);
                    if (0 == ftruncate(tgtFile, sb.st_size)) {
                        while (-1 != (err = read(srcFile, buf, sb.st_blksize)) && err > 0) {
                            if (-1 == write(tgtFile, buf, err)) {
                                err = errno;
                                break;
                            }
                        }
                        if (-1 == err)
                            err = errno;
                    } else
                        err = errno;
                    free(buf);    
                } else
                    err = ENOMEM;
            } else
                err = errno;
        } else
            err = errno;
    } else
        err = errno;
    
    if (0 == err) {
        struct timeval tv[2];
        tv[0].tv_sec = sb.st_atimespec.tv_sec;
        tv[0].tv_usec = sb.st_atimespec.tv_nsec / 1000;
        tv[1].tv_sec = sb.st_mtimespec.tv_sec;
        tv[1].tv_usec = sb.st_mtimespec.tv_nsec / 1000;
        (void)futimes(tgtFile, tv); // failure not critical
    }
    
    if (-1 != srcFile) close(srcFile);
    if (-1 != tgtFile) close(tgtFile);
    
    return (err);
}

int p2b_merge(const char *outfile, int version, char* const *files)
{
    int err;
    p2b_handle_t h;
    
    if (!outfile || !files || !files[0])
        return (EINVAL);
    
    char tmp[] = "/tmp/pguardXXXXXX";
    mktemp(tmp);
    
    if (0 != (err = p2b_open(tmp, version, &h)))
        return (err);
    
    int i, fileIdx = 0;
    pp_msg_iprange_t *addrs;
    u_int8_t **names;
    int32_t count;
    while (files[fileIdx]) {
        if (0 != (err = p2b_parse(files[fileIdx], &addrs, &count, NULL, &names))) {
            if (ERANGE == err) {
                fprintf(stderr, "%s: Warning: failed to parse %s - ignoring.\n",
                    __FUNCTION__, files[fileIdx]);
                fileIdx++;
                err = 0;
                continue;
            } else
            	break;
        }
        
        for (i = 0; i < count && !err; ++i)
            err = p2b_add(h, (char*)names[addrs[i].p2_name_idx], &addrs[i]);
        
        free(addrs);
        free(names);
        fileIdx++;
    }
    (void)p2b_close(h);
    
    // The output file probably contains dupe/overlapping ranges now. Prune them.
    if (0 == err && 0 == (err = p2b_parse(tmp, &addrs, &count, NULL, &names))) {
        if (0 == (err = p2b_open(tmp, P2B_OPEN_MAKE_VERSION(P2B_OPEN_TRUNCATE|P2B_OPEN_NOCACHE, version), &h))) {
            for (i = 0; i < count && !err; ++i)
                err = p2b_add(h, (char*)names[addrs[i].p2_name_idx], &addrs[i]);
            (void)p2b_close(h);
        }
        free(addrs);
        free(names);
    }
    
    if (!err)
        err = p2b_exchange(tmp, outfile);
    
    (void)unlink(tmp);
    
    return (err);
}

#define P2B_HANDLE_MAGIC 0xdaacd00d
struct p2b_handle_
{
    int magic;
    int fd;
    int ver;
    int32_t count;
    int (*entry_size)(p2b_handle_t, int, const struct pp_msg_iprange*);
    int (*make_entry)(p2b_handle_t, const char*, int, const struct pp_msg_iprange*, u_int8_t*);
    // cached values from p2p entry_size -- used by p2p make_entry
    char ipstartstr[64];
    char ipendstr[64];
};

static int p2bv2_entrysize(p2b_handle_t h __unused, int namelen, const struct pp_msg_iprange *rp __unused)
{
    return (namelen +  1 /*NULL*/ + (sizeof(u_int32_t) * 2));
}

static int p2bv2_makeentry(p2b_handle_t h __unused, const char *name, int namelen,
    const struct pp_msg_iprange *rp, u_int8_t *buf)
{
    strcpy((char*)buf, name);
    u_int32_t *range = (u_int32_t*)(buf+(namelen+1));
    range[0] = htonl(rp->p2_ipstart);
    if (rp->p2_ipend)
        range[1] = htonl(rp->p2_ipend);
    else
        range[1] = htonl(rp->p2_ipstart);
    return (0);
}

static int p2p_entrysize(p2b_handle_t h, int namelen, const struct pp_msg_iprange *rp)
{
    struct in_addr start, end;            
    start.s_addr = htonl(rp->p2_ipstart);
    end.s_addr = htonl(rp->p2_ipend);
    
    (void)pp_inet_ntoa(start, h->ipstartstr, sizeof(h->ipstartstr)-1);
    (void)pp_inet_ntoa(end, h->ipendstr, sizeof(h->ipendstr)-1);
    h->ipstartstr[sizeof(h->ipstartstr)-1] = h->ipendstr[sizeof(h->ipendstr)-1] = 0;
    
    return (namelen + 1 /*':'*/ + strlen(h->ipstartstr) + 1 /*'-'*/ + strlen(h->ipendstr) + 1 /*'\n'*/);
}

static int p2p_makeentry(p2b_handle_t h, const char *name, int namelen,
    const struct pp_msg_iprange *rp, u_int8_t *buf)
{
    // strip ':' from name
    strcpy((char*)buf, name);
    char *tok = (char*)buf;
    while ((tok = strchr(tok, ':')))
        *tok = '_';
#ifdef PPDBG
    if (namelen != strlen((char*)buf)) {
        trap();
    }
#endif
    sprintf((char*)(buf+namelen), ":%s-%s\n", h->ipstartstr, h->ipendstr);
    return (0);
}

int p2b_open(const char *path, int version, p2b_handle_t *hp)
{
    int flags = (version & 0xffff0000) >> 16;
    version = (version & 0x0000ffff);
    
    if (P2B_VERSION2 != version && P2P_VERSION != version)
        return (ENOTSUP);
    
    int err = 0;
    p2b_handle_t h = (p2b_handle_t)malloc(sizeof(*h));
    if (!h) {
        err = ENOMEM;
        goto p2b_open_exit;
    }
    
    int newfile = 0;
    flags = O_RDWR | ((flags & P2B_OPEN_TRUNCATE) ? O_TRUNC : 0);
    if (flags & O_TRUNC)
        newfile = 1;
    int fd = open(path, flags, S_IRUSR|S_IWUSR);
    if (-1 == fd && ENOENT == errno) {
        fd = open(path, flags|O_CREAT, S_IRUSR|S_IWUSR);
        newfile = 1;
    }
    if (-1 == fd) {
        err = errno;
        goto p2b_open_exit;
    }
    
    if (flags & P2B_OPEN_NOCACHE)
        (void)fcntl(fd, F_NOCACHE, 1);
    
    if (P2B_VERSION2 == version) {
        struct p2bhdr hdr;
        int bytes;
        if (!newfile) {
            bytes = pread(fd, &hdr, sizeof(hdr), 0);
            if (bytes != sizeof(hdr) ||
                0 != memcmp(p2b_hdr_sig, hdr.sig, sizeof(p2b_hdr_sig))) {
                err = EINVAL;
                goto p2b_open_exit;
            }
        } else {
            bcopy(p2b_hdr_sig, hdr.sig, sizeof(p2b_hdr_sig));
            hdr.version = version;
            bytes = pwrite(fd, &hdr, sizeof(hdr), 0);
            if (bytes != sizeof(hdr)) {
                err = EINVAL;
                goto p2b_open_exit;
            }
        }
        h->entry_size = p2bv2_entrysize;
        h->make_entry = p2bv2_makeentry;
    } else { // p2p text
        h->entry_size = p2p_entrysize;
        h->make_entry = p2p_makeentry;
    }
    
    h->magic = P2B_HANDLE_MAGIC;
    h->fd = fd;
    fd = -1;
    h->ver = version;
    *hp = h;
    h = NULL;
    
p2b_open_exit:
    if (h)
        free(h);
    
    if (-1 != fd)
        close(fd);
    
    return (err);
}

int p2b_close(p2b_handle_t h)
{
    if (P2B_HANDLE_MAGIC != h->magic)
        return (EINVAL);
    close(h->fd);
    free(h);
    return (0);
}

int p2b_add(p2b_handle_t h, const char *name, const struct pp_msg_iprange *rp)
{
    int namelen = strlen(name);
    if (0 == namelen) {
        name = "Unknown";
        namelen = strlen(name);
    } else if (namelen > 255)
        namelen = 255;
    
    if (P2B_HANDLE_MAGIC != h->magic || rp->p2_ipstart > rp->p2_ipend)
        return (EINVAL);
    
    // Add is fairly simple (for v2/text), we just write the record at EOF
    struct stat sb;
    if (-1 == fstat(h->fd, &sb))
        return (errno);
    if (sb.st_size > P2B_MAX_FILE_SIZE)
        return (EFBIG);
    
    int bytes = h->entry_size(h, namelen, rp);
    u_int8_t *buf = malloc(bytes+1);
    if (!buf)
        return (ENOMEM);
    bzero(buf, bytes);
    
    h->make_entry(h, name, namelen, rp, buf);
    
    int err = 0;
    if (-1 == pwrite(h->fd, buf, bytes, sb.st_size))
        err = errno;
    
    free(buf);
    
    return (0);
}

int p2b_remove(p2b_handle_t h, const char *name __unused, const struct pp_msg_iprange *rp)
{
    if (P2B_HANDLE_MAGIC != h->magic || !rp->p2_ipstart ||
        rp->p2_ipstart > rp->p2_ipend)
        return (EINVAL);
#if 0
    int namelen = strlen(name);
    if (0 == namelen || namelen > 255)
        return (EINVAL);
#endif
    
    // A remove could be in the middle of the file, so load the whole thing into mem
    // and then write back each valid range after removal.
    int err;
    pp_msg_iprange_t *addrs;
    int addrcount;
    u_int8_t **names;
    if (0 == (err = p2b_fparse(h->fd, &addrs, &addrcount, NULL, &names))) {
        // Find the range to remove
        pp_msg_iprange_t *rrp;
        rrp = (pp_msg_iprange_t*)pp_match_addr(addrs, addrcount, rp->p2_ipstart);
        if (rrp && rrp->p2_ipend != rp->p2_ipend) {
            rrp = (pp_msg_iprange_t*)pp_match_addr(addrs, addrcount, rp->p2_ipend);
            if (rrp && rrp->p2_ipstart != rp->p2_ipstart)
                rrp = NULL;
        }
        if (rrp) {
            rrp->p2_name_idx = (typeof(rrp->p2_name_idx))-1;
            // now write the remaining entries back out
            err = ftruncate(h->fd, ((P2B_VERSION2 == h->ver) ? sizeof(struct p2bhdr) : 0));
            
            int i;
            for (i=0; !err && i < addrcount; ++i) {
                if ((typeof(rrp->p2_name_idx))-1 != addrs[i].p2_name_idx)
                    err = p2b_add(h, (char*)names[addrs[i].p2_name_idx], &addrs[i]);
            }
        } else
            err = ENOENT;
        
        free(addrs);
        free(names);
    }

    return (err);
}
