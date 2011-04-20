/*
    Copyright 2005,2006 Brian Bergstrand

    This program is free software; you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program;
    if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#import <Foundation/Foundation.h>
#import <sys/types.h>
#import <sys/socket.h>
#import <netinet/in.h>
#import <arpa/inet.h>

#import "pplib.h"
#import "p2b.h"

@implementation NSObject (PPLibExtensions)

- (NSMutableArray*)addressSpecificationsFromFile:(NSString*)path
{
    NSMutableArray *ranges = [[NSMutableArray alloc] init];
    
    pp_msg_iprange_t *addrs;
    int32_t count;
    u_int8_t **names;
    int err = p2b_parse([path fileSystemRepresentation], &addrs, &count, NULL, &names);
    if (!err) {
        int i;
        for (i=0; i < count; ++i) {
            char ipstart[32], ipend[32];
            struct in_addr addr;
            addr.s_addr = htonl(addrs[i].p2_ipstart);
            pp_inet_ntoa(addr, ipstart, sizeof(ipstart)-1);
            ipstart[sizeof(ipstart)-1] = 0;
            addr.s_addr = htonl(addrs[i].p2_ipend);
            pp_inet_ntoa(addr, ipend, sizeof(ipend)-1);
            ipend[sizeof(ipend)-1] = 0;
            
            NSMutableDictionary *d = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
                [NSString stringWithUTF8String:(const char*)names[addrs[i].p2_name_idx]],
                PP_ADDR_SPEC_NAME,
                [NSString stringWithUTF8String:ipstart], PP_ADDR_SPEC_START,
                [NSString stringWithUTF8String:ipend], PP_ADDR_SPEC_END,
                nil];
            [ranges addObject:d];
            [d release];
        }
    }
    
    if (err && ERANGE != err) {
        [ranges release];
        return (nil);
    }
    
    return ([ranges autorelease]); // return even an empty array
}

- (void)commitAddressSpecifications:(NSMutableDictionary*)args
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSArray *ranges = [args objectForKey:@"Ranges"];
    NSString *path = [args objectForKey:@"Path"];
    
    NSString *tmppath;
    NSFileManager *fm = [NSFileManager defaultManager];
    if ([fm fileExistsAtPath:path]) {
        char tmpbuf[] = "/tmp/pprotXXXXXX";
        tmppath = [NSString stringWithUTF8String:mktemp(tmpbuf)];
    } else
        tmppath = path;
    
    p2b_handle_t h = NULL;
    int err = p2b_open([tmppath fileSystemRepresentation], P2B_VERSION2, &h);
    if (!err) {
        pp_msg_iprange_t range;
        NSEnumerator *en = [ranges objectEnumerator];
        NSDictionary *r;
        NSString *tmp;
        while ((r = [en nextObject])) {
            tmp = [r valueForKey:PP_ADDR_SPEC_START];
            if (!tmp || 0 == [tmp length] || 0 == inet_aton([tmp UTF8String], (struct in_addr*)&range.p2_ipstart)) {
                err = EINVAL;
                break;
            }
            #ifdef BYTE_ORDER == LITTLE_ENDIAN
            // There's no real need for this compile conditional, but it does save
            // BIG_ENDIAN compiles a memory store (and possibly load).
            else
                range.p2_ipstart = ntohl(range.p2_ipstart);
            #endif            
            
            tmp = [r valueForKey:PP_ADDR_SPEC_END];
            if (!tmp || 0 == [tmp length])
                range.p2_ipend = range.p2_ipstart;
            else if (0 == inet_aton([tmp UTF8String], (struct in_addr*)&range.p2_ipend)) {
                err = EINVAL;
                break;
            }
            #ifdef BYTE_ORDER == LITTLE_ENDIAN
            // There's no real need for this compile conditional, but it does save
            // BIG_ENDIAN compiles a memory store (and possibly load).
            else
                range.p2_ipend = ntohl(range.p2_ipend);
            #endif
            
            if (range.p2_ipend < range.p2_ipstart) {
                err = EINVAL;
                break;
            }
            
            tmp = [r valueForKey:PP_ADDR_SPEC_NAME];
            if (!tmp || 0 == [tmp length]) {
                // p2b_add requires a name
                tmp = NSLocalizedString(@"Unknown", "");
            }
            err = p2b_add(h, [tmp UTF8String], &range);
            if (err)
                break;
        }
    }
    
    if (tmppath == path)
        tmppath = nil;
    
    if (h)
        p2b_close(h);
    
    if (!err && tmppath) {
        const char *tmpFile = [tmppath fileSystemRepresentation];
        const char *origFile = [path fileSystemRepresentation];
        if (-1 == (err = p2b_compare(tmpFile, origFile))) {
            // files are different
            err = p2b_exchange(tmpFile, origFile);
        }
    }
    if (tmppath && [fm fileExistsAtPath:tmppath])
        (void)[fm removeFileAtPath:tmppath handler:nil];
    
    [args setObject:[NSNumber numberWithInt:err] forKey:@"err"];
    [pool release];
}

- (int)commitAddressSpecifications:(NSArray*)ranges toFile:(NSString*)path
{
    return ([self commitAddressSpecifications:ranges toFile:path inBackground:NO]);
}

// ranges = Array of address specifications (dictionary)
- (int)commitAddressSpecifications:(NSArray*)ranges toFile:(NSString*)path inBackground:(BOOL)background
{
    NSMutableDictionary *args = [NSMutableDictionary dictionaryWithObjectsAndKeys:
        ranges, @"Ranges",
        path, @"Path",
        nil];
    
    if (!background) {
        [self commitAddressSpecifications:args];
        return ([[args objectForKey:@"err"] intValue]);
    } else {
        [NSThread detachNewThreadSelector:@selector(commitAddressSpecifications:) toTarget:self withObject:args];
        return (0);
    }
}
    
@end
