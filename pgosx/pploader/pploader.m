/*
    Copyright 2005-2008 Brian Bergstrand

    This program is free software; you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program;
    if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#import <Cocoa/Cocoa.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <unistd.h>
#import <sys/queue.h>
#import <sys/stat.h>
#import <sys/sysctl.h>

// in libSystem, but not headers - Tiger only
#ifndef _COPYFILE_H_
typedef struct _copyfile_state * copyfile_state_t;
typedef uint32_t copyfile_flags_t;
int copyfile(const char *from, const char *to, copyfile_state_t state, copyfile_flags_t flags);
#define COPYFILE_ACL	    (1<<0)
#define COPYFILE_STAT	    (1<<1)
#define COPYFILE_XATTR	    (1<<2)
#define COPYFILE_DATA	    (1<<3)

#define COPYFILE_SECURITY   (COPYFILE_STAT | COPYFILE_ACL)
#define COPYFILE_METADATA   (COPYFILE_SECURITY | COPYFILE_XATTR)
#define COPYFILE_ALL	    (COPYFILE_METADATA | COPYFILE_DATA)
#endif

#define PG_MAKE_TABLE_ID
#import "UKFileWatcher.h"
#import "UKKQueue.h"
#import "pplib.h"
#import "p2b.h"
#import "pgsb.h"
#import "pploader.h"
#import "PPKTool.h"
#import "PPAddrActionAskController.h"

PG_DEFINE_TABLE_ID(PG_TABLE_ID_ALLW_LOCAL,
0x85, 0x27, 0x3C, 0x05, 0x12, 0x52, 0x4E, 0x96, 0xAD, 0x9E, 0x13, 0x25, 0xB8, 0x34, 0x76, 0x6E);

@interface PPLoader : NSObject <PPLoaderDOServerExtensions> {
    NSTimer *updateTimer;
    NSMutableArray *lists, *internalLists, *tempAddresses;
    NSMutableDictionary *watchList;
    NSTimer *tempAddressScanTimer;
    NSMutableDictionary *downloads; // keyed on URL
    PPKTool *ppk;
}

- (int)loadPortRule:(NSString*)rule;
- (void)handleAddrActionAsk:(NSNotification*)note;
@end

@interface NSApplication (PPLoaderAppleScriptExtensions)
- (id)updateLists:(NSScriptCommand*)command;
@end

#define ListInternal(l) [[l objectForKey:@"Internal"] boolValue]
#define ListUser(l) [[l objectForKey:@"User Gen"] boolValue]
#define ListID(l) [l objectForKey:@"UUID"]
#define ListBlockHTTP(l) [[l objectForKey:@"Block HTTP"] boolValue]
#define ListAllow(l) [[l objectForKey:@"Allow"] boolValue]
#define ListLastMod(l) [l objectForKey:@"Last Update"]
#define ListCacheFile(l) [self cachePathForList:l]
#define ListIsTemp(l) [[l objectForKey:@"Temp"] boolValue]

#define ListSet(l, o, k) do { \
    [l setObject:o forKey:k]; \
}while(0)

//2004-08-27 04:12:10 GMT
static NSDate *LIST_EPOCH = nil;

#define OBSOLETE_LIST_ID_TEMP_ALLW 32767

#define PPList NSMutableDictionary
#define PPListClass [NSDictionary class]

// There seems to be trouble with retaining client objects. Actually,
// retaining is fine, it's releaseing that's the problem. At least when
// a cleint dies in notifyClientsUsingSelector exceptions are generated
// when dead clients are released.
struct ppclient {
    unsigned int cid;
    NSDistantObject<PPLoaderDOClientExtensions> *client;
    LIST_ENTRY(ppclient) link;
};
LIST_HEAD(client_head, ppclient) clients = LIST_HEAD_INITIALIZER(clients);
static unsigned int ppclientid = 0;
static BOOL ppdisabled = NO;
static NSConditionLock *dlLock = nil;
static NSMutableArray *dlQueue = nil;
#define DL_HAVE_WORK 1
#define DL_NO_WORK 0

#define BADOBJ (id)PP_BAD_ADDRESS

static void PGSCDynamicStoreCallBack(SCDynamicStoreRef scRef, CFArrayRef keys, void *context);

@implementation PPLoader

- (void)notifyClientsUsingSelector:(SEL)selc object:(id)obj object2:(id)obj2
{
    struct ppclient *client = LIST_FIRST(&clients), *dead;
    while(client) {
        @try {
            pptrace(@"sending to %d, %p\n", client->cid, client->client);
            if (BADOBJ == obj2)
                [client->client performSelector:selc withObject:obj];
            else
                [client->client performSelector:selc withObject:obj withObject:obj2];
            client = LIST_NEXT(client, link);
        } @catch (NSException *exception) {
            pptrace(@"Client exception: %@ '%@'\n", [exception name], [exception reason]);
            
            dead = client;
            client = LIST_NEXT(dead, link);
            
            if ([NSInvalidArgumentException isEqualToString:[exception name]])
                continue;
            
            LIST_REMOVE(dead, link);
            
            @try {
                [dead->client release];
            } @catch (NSException *exception) { }
            free(dead);
        }
    }
}

- (void)notifyClientsUsingSelector:(SEL)selc object:(id)obj
{
    [self notifyClientsUsingSelector:selc object:obj object2:BADOBJ];
}

- (NSURL*)firstURL:(PPList*)list
{
    NSArray *urls = ListURLs(list);
    NSURL *url = nil;
    if (urls && [urls count] > 0) {
        @try {
            url = [NSURL URLWithString:[urls objectAtIndex:0]];
        } @finally{}
    }
    return (url);
}

- (void)resetLastUpdateTime:(PPList*)list
{
    NSArray *keys = ListURLs(list);
    NSString *key = nil;
    NSEnumerator *en = [keys objectEnumerator];
    NSMutableDictionary *urls = [list objectForKey:@"URLs"];
    while((key = [en nextObject])) {
        @try {
            [urls setObject:LIST_EPOCH forKey:key];
        } @finally{}
    }
    // Reset the global date
    ListSet(list, LIST_EPOCH, @"Last Update");
}

#ifdef PPDBG
- (PPList*)listWithURL:(NSURL*)url
{
    NSEnumerator *en = [lists objectEnumerator];
    PPList *list;
    
    NSString *listURLString, *urlString = [url absoluteString];
    NSArray *urls;
    NSEnumerator *enURLs;
    while((list = [en nextObject])) {
        urls = ListURLs(list);
        enURLs = [urls objectEnumerator];
        while ((listURLString = [enURLs nextObject])) {
            if (listURLString && [listURLString isEqualToString:urlString])
                goto listURLFound;
        }
    }
    
listURLFound:
    return (list ? [list retain] : nil);
}
#endif

- (PPList*)listWithIDStr:(NSString*)listid
{
    NSEnumerator *en = [lists objectEnumerator];
    PPList *list;
    
    while((list = [en nextObject])) {
        if (NSOrderedSame == [listid caseInsensitiveCompare:ListID(list)])
            break;
    }
    
    return (list ? [list retain] : nil);
}

- (PPList*)listWithID:(const pg_table_id_t)listid
{
    NSEnumerator *en = [lists objectEnumerator];
    PPList *list;
    
    pg_table_id_t tid;
    while((list = [en nextObject])) {
        pg_table_id_fromcfstr((CFStringRef)ListID(list), tid);
        if (0 == pg_table_id_cmp(listid, tid))
            break;
    }
    
    return (list ? [list retain] : nil);
}

- (NSArray*)tempLists
{
    NSMutableArray *tmp = [NSMutableArray array];
    NSEnumerator *en = [lists objectEnumerator];
    PPList *list;
    
    while((list = [en nextObject])) {
        if (ListInternal(list) && ListIsTemp(list))
            [tmp addObject:list];
    }
    return (tmp);
}

- (PPList*)tempAllowList
{
    NSArray *tmp = [self tempLists];
    NSEnumerator *en = [tmp objectEnumerator];
    PPList *list;
    
    while((list = [en nextObject])) {
        if (ListAllow(list))
            break;
    }
    return (list);
}

#define CACHE_ROOT [[NSString stringWithFormat:@"~/Library/Caches/%s", PP_CTL_NAME] stringByExpandingTildeInPath]
- (NSString*)cachePathForList:(PPList*)list
{
    NSString *path = [list objectForKey:@"CacheFile"];
    NSString *root = CACHE_ROOT;
    NSRange r;
    if (path && [path length]
        // Earlier versions had a bug that could result in this condition.
        && NSNotFound == (r = [path rangeOfString:@"(null)"]).location) {
        path = [root stringByAppendingPathComponent:path];
    } else {
        NSString *listid = ListID(list);
        if (listid && [listid length]) {
            path = [root stringByAppendingPathComponent:listid];
            [list setObject:listid forKey:@"CacheFile"];
        }
    }
    return (path);
}

- (BOOL)setCacheFile:(NSString*)leafPath forList:(PPList*)list
{
    NSRange r;
    if (leafPath && [leafPath length]
        && NSNotFound == (r = [leafPath rangeOfString:@"/"]).location) {
        [list setObject:leafPath forKey:@"CacheFile"];
        return (YES);
    }
    return (NO);
}

- (NSString*)decompressFile:(NSString*)path
{
    static const u_int8_t k7zSig[] = {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C};
    static const u_int8_t kBz2Sig[] = {'B', 'Z', 'h'};
    static const u_int8_t kZipSig[] = {'P', 'K', 0x03, 0x04}; // 0x04034b50 (little endian)
    #define SIGBUF_SZ sizeof(k7zSig)
    u_int8_t sigbuf[SIGBUF_SZ] = {0,};
    int fd;
    if ((fd = open ([path fileSystemRepresentation], O_RDONLY)) >= 0) {
        (void)read(fd, sigbuf, sizeof(sigbuf));
        close(fd);
    } else {
        NSLog(@"Failed to read compression header for '%@': %d.\n", path, errno);
        return (nil);
    }
    
    NSString *util;
    NSArray *args;
    if (0 == memcmp(k7zSig, sigbuf, sizeof(k7zSig))) {
        util = [[NSBundle mainBundle] pathForResource:@"7za" ofType:nil];
        args = [NSArray arrayWithObjects:@"x", @"-bd", @"-y", @"-so", path, nil];
    } else if (0x1f == sigbuf[0] && 0x8b == sigbuf[1]) {
        util = @"/usr/bin/gzip";
        if (![[NSFileManager defaultManager] fileExistsAtPath:util])
            util = @"/bin/gzip";
        args = [NSArray arrayWithObjects:@"-dc", path, nil];
    } else if (0 == memcmp(kBz2Sig, sigbuf, sizeof(kBz2Sig))) {
        util = @"/usr/bin/bzip2";
        args = [NSArray arrayWithObjects:@"-dc", path, nil];
    } else if (0 == memcmp(kZipSig, sigbuf, sizeof(kZipSig))) {
        util = @"/usr/bin/unzip";
        args = [NSArray arrayWithObjects:@"-p", path, nil];
    } else
        return (path);
    
    NSTask *t = [[NSTask alloc] init];
    [t setLaunchPath:util];
    [t setArguments:args];
    [t setCurrentDirectoryPath:NSTemporaryDirectory()];
    
    NSFileHandle *fh = nil, *errh = nil;
    char tmp[] = "/tmp/pgdcXXXXXX";
    fd = mkstemp(tmp);
    if (-1 == fd) {
        NSLog(@"Failed to create decompression temp file: %d.\n", errno);
        return (nil);
    }
    
    NSString *output = [NSString stringWithCString:tmp encoding:NSASCIIStringEncoding];
    
    // Create stderr output
    @try {
        // XXX NSFileHandle is not thread safe, but out of order writes is not a bid deal for err output
        (void)[[NSFileManager defaultManager] createFileAtPath:@"/tmp/pgdecomp.log" contents:nil attributes:nil];
        errh = [NSFileHandle fileHandleForWritingAtPath:@"/tmp/pgdecomp.log"];
        [errh seekToEndOfFile];
    } @finally {}
    
    int err = 0;
    @try {
        fh = [[NSFileHandle alloc] initWithFileDescriptor:fd closeOnDealloc:YES];
        [t setStandardOutput:fh];
        if (errh)
            [t setStandardError:errh];
        [t launch];
        [t waitUntilExit];
        err = [t terminationStatus];
    } @catch (NSException *exception) {
        err = EINVAL;
        NSLog(@"Exception launching '%@': %@\n", util, exception);
    }
    [t release];
    [errh closeFile];
    
    if (fh)
        [fh release];
    else
        close(fd);
    
    if (err) {
        NSLog(@"Decompression of '%@' failed: %d.\n", path, err);
        return (nil);
    }
    
    pptrace(@"Decompress returning '%@'.\n", output);
    return (output);
}

- (int)loadList:(PPList*)list fromFile:(NSString*)path
{
    pp_msg_iprange_t *addrs;
    u_int8_t **names;
    int32_t count;
    u_int32_t addrcount;
    
    pptrace(@"Attempting to load '%@' from '%@'.\n", ListName(list), path);
    
#ifdef obsolete
    path = [self decompressFile:path];
#endif
    if (!path)
        return (ENOENT);
        
    if (ppdisabled)
        return (0);
    
    int err = p2b_parse([path fileSystemRepresentation], &addrs, &count, &addrcount, &names);
    if (!err) {
        NSString *tblname = ListName(list);
        if (!tblname || 0 == [tblname length])
            tblname = [[path lastPathComponent] stringByDeletingPathExtension];
        pg_table_id_t tableid;
        
        if ((err = pg_table_id_fromcfstr((CFStringRef)ListID(list), tableid))) {
            NSLog(@"'%@' contains an invalid id '%@'\n", tblname, ListID(list));
            goto load_bad_id;
        }
        
        // load the names into shared mem
        if ((err = pp_load_names(tableid, [tblname fileSystemRepresentation], names))) {
            NSLog(@"Failed to load names for table %@,%@: %d.\n", tblname, ListID(list), err);
            err = 0;// not fatal
        }
        
        // load them into the kernel
        int loadflags = 0;
        if (ListAllow(list))
            loadflags |= PP_LOAD_ALWYS_PASS;
        else if (ListBlockHTTP(list))
            loadflags |= PP_LOAD_BLOCK_STD;
        
        if ((err = pp_load_table(addrs, count, addrcount, tableid, loadflags)))
            NSLog(@"Failed to load list '%@': %d.\n", ListName(list), err);
        
load_bad_id:
        free(addrs);
        free(names);
    } else
        NSLog(@"Failed to parse list '%@' from file '%@': %d.\n",
            ListName(list), path, err);
    
    if (err)
        [self notifyClientsUsingSelector:@selector(listFailedUpdate:error:) object:list
            object2:[NSError errorWithDomain:NSPOSIXErrorDomain code:err userInfo:nil]];
    return (err);
}


- (int)unloadLists:(NSArray*)ulists makeInactive:(BOOL)makeInactive
{
    NSEnumerator *en = [ulists objectEnumerator];
    PPList *list;
    
    pg_table_id_t listid;
    int err = 0, allerr = 0;
    while ((list = [en nextObject])) {
        if (ListActive(list) && 0 == (err = pg_table_id_fromcfstr((CFStringRef)ListID(list), listid))) {
            if (0 == (err = pp_unload_table(listid))) {
                if (makeInactive)
                    ListSet(list, [NSNumber numberWithBool:NO], @"Active");
            }
        }
        if (err) {
            NSLog(@"Failed to unload list '%@' with err: %d.\n", ListName(list), err);
            allerr = err;
        }
    }
    return (allerr);
}

- (int)unloadList:(PPList*)list
{
    return ([self unloadLists:[NSArray arrayWithObject:list] makeInactive:NO]);
}

#define IsNetworkUp(flags) \
( ((flags) & kSCNetworkFlagsReachable) && (0 == ((flags) & kSCNetworkFlagsConnectionRequired) || \
  ((flags) & kSCNetworkFlagsConnectionAutomatic)) )

// HTTP Last-Modified time format
#define HTTP_DATE_FMT_RAW @"%a, %d %b %Y %H:%M:%S %Z"
- (void)downloadList:(PPList*)list
{
    NSArray *urls;
    NSURL *firstURL;
    @try {
        urls = ListURLs(list);
        firstURL = [NSURL URLWithString:[urls objectAtIndex:0]];
    } @catch (NSException *exception) {
        urls = nil;
    }
    BOOL isFile = [firstURL isFileURL];
    if (!urls || 0 == [urls count] || isFile) {
        if (NO == isFile)
            NSLog(@"List '%@' does not contain any valid URLs.\n", ListName(list));
        return;
    }
    
    SCNetworkReachabilityRef scref;
    scref = SCNetworkReachabilityCreateWithName(kCFAllocatorDefault,
        [[firstURL host] cStringUsingEncoding:NSASCIIStringEncoding]);

    if (scref) {
        SCNetworkConnectionFlags connectionFlags;
        BOOL up = (SCNetworkReachabilityGetFlags(scref, &connectionFlags)
            && IsNetworkUp(connectionFlags));
        CFRelease(scref);
        if (!up) {
            NSString *diagMsg;
            CFNetDiagnosticRef diag = CFNetDiagnosticCreateWithURL(kCFAllocatorDefault,
                (CFURLRef)firstURL);
            if (diag) {
                (void)CFNetDiagnosticCopyNetworkStatusPassively(diag, (CFStringRef*)&diagMsg);
                CFRelease(diag);
                (void)[diagMsg autorelease];
            } else
                diagMsg = @"A network connection does not seem to be available.";
            
            NSLog(@"%@ Aborting download '%@' (%x).\n", diagMsg, ListName(list), connectionFlags);
            return;
        }
    }
    
    NSDictionary *urlEntries = [list objectForKey:@"URLs"];
    NSEnumerator *en = [urls objectEnumerator];
    
    NSString *urlString;
    NSMutableURLRequest *request;
    NSMutableArray *requests = [NSMutableArray array];
    NSDate *lastUpdate;
    NSURL *url;
    while ((urlString = [en nextObject])) {
        url = nil;
        @try {
            url = [NSURL URLWithString:urlString];
        } @catch (NSException *e) {}
        // XXX: blocklist.org died in 2007
        if (url && NO == [url isFileURL] && NO == [[[url host] lowercaseString] hasSuffix:@"blocklist.org"]) {
            lastUpdate = [urlEntries objectForKey:urlString];
        } else {
            NSLog(@"URL '%@' of list '%@' is not valid - skipping.\n", url, ListName(list));
            continue;
        }
        
        request = [NSMutableURLRequest requestWithURL:url
                    cachePolicy:NSURLRequestReloadIgnoringCacheData
                    timeoutInterval:60.0];
        if (request) {
            if ([@"http" isEqualToString:[url scheme]]) {
                NSString *since = [lastUpdate descriptionWithCalendarFormat:HTTP_DATE_FMT_RAW
                    timeZone:[NSTimeZone timeZoneWithAbbreviation:@"GMT"] locale:nil];
                [request setHTTPMethod:@"GET"];
                if (since) {
                    // Set a conditional load
                    [request setValue:since forHTTPHeaderField:@"If-Modified-Since"];
                }
            }
            [requests addObject:request];
        } else
            NSLog(@"Failed to create URL reqeust for '%@' from list '%@'.\n",
                url, ListName(list));
    }
    
    int listCount = [requests count];
    if (0 == listCount) 
        return;
    
    NSMutableDictionary *completedDownloads = [NSMutableDictionary dictionary];
    NSMutableArray *downloadsNotUpdated = [NSMutableArray array];
    NSMutableArray *count = [NSMutableArray arrayWithObject:[NSNumber numberWithInt:listCount]];
    en = [requests objectEnumerator];
    while ((request = [en nextObject])) {
        NSURLDownload *dl = [[[NSURLDownload alloc] initWithRequest:request delegate:self] autorelease];
        if (dl) {
            // Delegate methods are called on the main thread, so while the dl may start before
            // the dict is setup, there is no race where the dl does not "exist".
            NSValue *dlkey = [NSValue valueWithPointer:(void*)dl];
            [downloads setObject:
                [NSMutableDictionary dictionaryWithObjectsAndKeys:
                    list, @"list",
                    count, @"dependencies",
                    [[request URL] absoluteString], @"original URL",
                    completedDownloads, @"completed",
                    downloadsNotUpdated, @"not updated",
                    nil]
                forKey:dlkey];
            pptrace(@"downloadList: dl:%@ = %@\n", dlkey, [downloads objectForKey:dlkey]);
        }
    }
}

- (void)updateLists:(NSTimer *)timer
{
    NSEnumerator *en = [lists objectEnumerator];
    PPList *list;
    
    while((list = [en nextObject])) {
        if (!ListActive(list))
            continue;
        
        [self downloadList:list];
    }
}

- (void)listUpdated:(PPList*)list withURLs:(NSArray*)urls
{
    NSDate *now = [NSDate date];
    ListSet(list, now, @"Last Update");
    
    NSEnumerator *en = [urls objectEnumerator];
    NSMutableDictionary *urlEntries = [list objectForKey:@"URLs"];
    ppassert([urlEntries isKindOfClass:[NSMutableDictionary class]]);
    NSArray *urlEntryKeys = [urlEntries allKeys];
    NSString *urlString;
    while ((urlString = [en nextObject])) {
        if ([urlEntryKeys containsObject:urlString])
            [urlEntries setObject:now forKey:urlString];
    }
    
    [self notifyClientsUsingSelector:@selector(listDidUpdate:) object:list];
}

- (void)listUpdated:(PPList*)list
{
    ListSet(list, [NSDate date], @"Last Update");
    [self notifyClientsUsingSelector:@selector(listDidUpdate:) object:list];
}

- (void)loadListWithUpdateDelayed:(NSDictionary*)info
{
    PPList *list = [info objectForKey:@"list"];
    [self notifyClientsUsingSelector:@selector(listWillUpdate:) object:list];
    if (0 == [self loadList:list fromFile:[info objectForKey:@"file"]])
        [self listUpdated:list];
    
}

static NSMutableDictionary *writeTimers = nil;
- (void)fileWriteExpired:(NSTimer*)timer
{
    [timer retain];
    [writeTimers removeObjectForKey:[[timer userInfo] objectForKey:@"file"]];
    
    [self performSelector:@selector(loadListWithUpdateDelayed:)
        withObject:[timer userInfo] afterDelay:0.5];
    [timer release];
}

- (void)watcher:(id<UKFileWatcher>)kq receivedNotification:(NSString*)nm forPath:(NSString*)fpath
{
    PPList *list = [watchList objectForKey:fpath];
    
    pptrace(@"kq watcher received '%@' for '%@'.\n", nm, fpath);
    
    if (!list)
        NSLog(@"Received '%@' for '%@', yet no list was found.\n", nm, fpath);
    
#ifdef PPDBG
    PPList *foundList = [self listWithURL:[NSURL fileURLWithPath:fpath]];
    ppassert(list == foundList);
    [foundList release];
#endif
    
    if ([UKFileWatcherRenameNotification isEqualToString:nm] ||
        [UKFileWatcherDeleteNotification isEqualToString:nm]) {
        [watchList removeObjectForKey:fpath];
        ListSet(list, [NSNumber numberWithBool:NO], @"Active");
        [self notifyClientsUsingSelector:@selector(listDidUpdate:) object:list];
    } else if ([UKFileWatcherWriteNotification isEqualToString:nm]) {
        if (list) {
            // There's no real solution for finding out when a file is done writing,
            // so we have to hack something up based on an arbitrary timeout.
            if (!writeTimers)
                writeTimers = [[NSMutableDictionary alloc] init];
            
            NSTimer *wtimer = [writeTimers objectForKey:fpath];
            if (wtimer) {
                [wtimer retain];
                [writeTimers removeObjectForKey:fpath];
                [wtimer invalidate];
                [wtimer release];
            }
            
            NSDictionary *info = [NSDictionary dictionaryWithObjectsAndKeys:
                list, @"list", fpath, @"file", nil];
            wtimer = [NSTimer scheduledTimerWithTimeInterval:2.5 target:self
                selector:@selector(fileWriteExpired:) userInfo:info repeats:NO];
            [writeTimers setObject:wtimer forKey:fpath];
        }
    }
}

- (void)watchFile:(NSString*)path forList:(PPList*)list
{
    ppassert(nil != path);
    
    UKKQueue *q = [UKKQueue sharedQueue];
    if (!watchList) {
        watchList = [[NSMutableDictionary alloc] init];
        [q setDelegate:self];
    }
    
    if (path && nil == [watchList objectForKey:path]) {
        [q addPath:path];
        [watchList setObject:list forKey:path];
    }
}

#ifdef notused
- (void)ignoreFile:(NSString*)path
{
    ppassert(nil != path);
    
    if (path && watchList && [watchList objectForKey:path]) {
        [watchList removeObjectForKey:path];
        [[UKKQueue sharedQueue] removePathFromQueue:path];
    }
}
#endif

- (void)ignoreWatchedFiles:(PPList*)list
{
    NSEnumerator *en = [[watchList allKeys] objectEnumerator];
    NSString *path;
    while ((path = [en nextObject])) {
        if (list == [watchList objectForKey:path]) {
            [watchList removeObjectForKey:path];
            [[UKKQueue sharedQueue] removePathFromQueue:path];
        }
    }
}

- (void)loadListFromCache:(PPList*)list
{
    NSString *path = ListCacheFile(list);
    if (path && [[NSFileManager defaultManager] fileExistsAtPath:path])
        (void)[self loadList:list fromFile:path];
    else
        [self resetLastUpdateTime:list]; // Make sure we go to the network
}

- (void)prime
{
    NSFileManager *fm = [NSFileManager defaultManager];
    NSEnumerator *en = [lists objectEnumerator];
    PPList *list;
    
    NSURL *url;
    NSString *path;
    while((list = [en nextObject])) {
        if (!ListActive(list))
            continue;
          
        if (!(url = [self firstURL:list]) || !(path = [url path])) {
            NSLog(@"List '%@' contains an invalid URL.\n", ListName(list));
            continue;
        }
        
        if ([url isFileURL]) {
            [self watchFile:path forList:list];
        } else {
            path = ListCacheFile(list);
        }
        
        if (path && [fm fileExistsAtPath:path])
            (void)[self loadList:list fromFile:path];
        else {
            [self resetLastUpdateTime:list]; // Make sure we go to the network
            pptrace(@"No cache file for '%@' at '%@'.\n", ListName(list), path);
        }
    }
}

- (void)handleQuit:(NSNotification*)note
{
    [NSApp terminate:nil];
}

- (void)kextDidLoad:(NSNotification*)note
{
    [self prime];
    [self loadPortRule:[[NSUserDefaults standardUserDefaults] objectForKey:PP_PREF_PORT_RULES]];
    [updateTimer fire];
}

- (void)kextFailedLoad:(NSNotification*)note
{
    NSNumber *error = [[note userInfo] objectForKey:PP_KEXT_FAILED_ERR_KEY];
    int err = EINVAL; // default
    if (error)
        err = [error intValue];
    NSError *errDesc = [NSError errorWithDomain:NSPOSIXErrorDomain code:err userInfo:nil];
    NSLog(@"Kext failed to load: %@ (%d).\n", [errDesc localizedDescription], err);
}

- (void)kextFailedUnload:(NSNotification*)note
{
    NSNumber *error = [[note userInfo] objectForKey:PP_KEXT_FAILED_ERR_KEY];
    int err = EINVAL; // default
    if (error)
        err = [error intValue];
    NSError *errDesc = [NSError errorWithDomain:NSPOSIXErrorDomain code:err userInfo:nil];
    NSLog(@"Kext failed to unload: %@ (%d).\n", [errDesc localizedDescription], err);
}

- (void)didWake:(NSNotification*)note
{
    static BOOL fire = NO;
    
    if (!fire) {
        fire = YES;
        // network may not quite be ready yet.
        [self performSelector:@selector(didWake:) withObject:note afterDelay:5.0];
        return;
    }
    fire = NO;
    
    [updateTimer fire];
}

- (void)syncPrefs:(NSTimer*)timer
{
    [[NSUserDefaults standardUserDefaults] setObject:lists forKey:@"Lists"];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

- (void)setUpdateInterval:(NSTimeInterval)i
{
    static NSTimeInterval lastInterval = -1.0;
    
    if (i != lastInterval) {
        [updateTimer invalidate];
        if (0.0 != i) {
            if (i < 3600.0 || i > 86400.0)
                i = 10800.0;
            updateTimer = [NSTimer scheduledTimerWithTimeInterval:i target:self
                selector:@selector(updateLists:)userInfo:nil repeats:YES];
        } else
            updateTimer = nil;
        [[NSUserDefaults standardUserDefaults] setInteger:(int)i forKey:@"Update Interval"];
        lastInterval = i;
    }
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    static int setup = 1;
    
    if (setup) {
        // delaying setup allows the DO server to service connections
        // right away
        setup = 0;
        [self performSelector:@selector(applicationDidFinishLaunching:)
            withObject:notification afterDelay:0.75];
        return;
    }
    
    // create cache root
    if (![[NSFileManager defaultManager] fileExistsAtPath:CACHE_ROOT])
        [[NSFileManager defaultManager] createDirectoryAtPath:CACHE_ROOT attributes:nil];
    
    // create download service threads
    int mib[2] = {CTL_HW, HW_AVAILCPU};
    unsigned cpus;
    size_t sz = sizeof(cpus);
    if (-1 == sysctl(mib, 2, &cpus, &sz, nil, 0))
        cpus = 1;
    if (cpus <= 1)
        ++cpus;
    else if (cpus > 4)
        cpus = 4; // cap at 4 threads
    
    dlLock = [[NSConditionLock alloc] initWithCondition:DL_NO_WORK];
    dlQueue = [[NSMutableArray alloc] initWithCapacity:4];
    for (; cpus > 0; --cpus) {
        [NSThread detachNewThreadSelector:@selector(mergeListsThread:) toTarget:self withObject:nil];
    } 
    
    downloads = [[NSMutableDictionary alloc] init];
    
    ppk = [[PPKTool alloc] init];
    BOOL haveKext = [ppk isLoaded];

    NSTimeInterval i = (NSTimeInterval)[[NSUserDefaults standardUserDefaults]
        integerForKey:@"Update Interval"];
    [self setUpdateInterval:i];
    
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
        selector:@selector(didWake:) name:NSWorkspaceDidWakeNotification object:nil];
    
    [[NSTimer scheduledTimerWithTimeInterval:30.0 target:self selector:@selector(syncPrefs:)
        userInfo:nil repeats:YES] fire];
    
    // Register for IPv4/DNS changes
    SCDynamicStoreContext ctx;
    bzero(&ctx, sizeof(ctx));
    ctx.info = self;
    
    SCDynamicStoreRef dynStoreRef = SCDynamicStoreCreate(kCFAllocatorDefault,
        CFSTR("PeerGuardian"), PGSCDynamicStoreCallBack, &ctx);
    if (dynStoreRef) {
        CFMutableArrayRef sckeys = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
        if (sckeys) {
            CFStringRef key;
            #ifdef notyet
            key = SCDynamicStoreKeyCreateNetworkServiceEntity(kCFAllocatorDefault,
                kSCDynamicStoreDomainState, kSCCompAnyRegex, kSCEntNetDNS);
            CFArrayAppendValue(sckeys, key);
            CFRelease(key);
            #endif
            
            key = SCDynamicStoreKeyCreateNetworkGlobalEntity(kCFAllocatorDefault,
                kSCDynamicStoreDomainState, kSCEntNetDNS);
            CFArrayAppendValue(sckeys, key);
            CFRelease(key);
            
            key = SCDynamicStoreKeyCreateNetworkGlobalEntity(kCFAllocatorDefault,
                kSCDynamicStoreDomainState, kSCEntNetIPv4);
            CFArrayAppendValue(sckeys, key);
            CFRelease(key);
            
            SCDynamicStoreSetNotificationKeys(dynStoreRef, sckeys, NULL);

            CFRunLoopSourceRef dynStoreRLRef = SCDynamicStoreCreateRunLoopSource(kCFAllocatorDefault,
                dynStoreRef, 0);

            CFRunLoopAddSource ([[NSRunLoop currentRunLoop] getCFRunLoop], dynStoreRLRef, kCFRunLoopCommonModes);
            CFRelease(dynStoreRLRef);
            
            // Get the initial settings
            ppdisabled = YES; // We set this so the list does not load yet
            PGSCDynamicStoreCallBack(dynStoreRef, sckeys, self);
            ppdisabled = NO;
            CFRelease(sckeys);
        } else
            NSLog(@"Failed to allocate SC key array!");
            
        CFRelease(dynStoreRef);
    } else
        NSLog(@"Failed to create SC dynamic store!");
    
    if (haveKext) {
#ifdef PPDBG
        // I don't believe this is a large problem, and if it is suspected, we can always recommended to
        // the user to "Disable Filters" which unloads all tables.

        // Unload all tables, so we make sure only our defined tables are active
        (void)pp_unload_table(PP_TABLE_ID_ALL);
#endif
        // make sure Temp table is not active
        (void)[self unloadLists:[self tempLists] makeInactive:YES];
        
        [self kextDidLoad:nil];
    } else
        [ppk load];
}

// As of 4/26/2006, the bluetack .txt lists are no more.
- (void)updateBluetackURLs:(PPList*)list
{
    NSArray *keys = ListURLs(list);
    NSString *key = nil;
    NSEnumerator *en = [keys objectEnumerator];
    NSMutableDictionary *urls = [list objectForKey:@"URLs"];
    while((key = [en nextObject])) {
        @try {
            NSURL *url = [NSURL URLWithString:key];
            if ([[url host] isEqualToString:@"www.bluetack.co.uk"]) {
                NSMutableString *newKey = [key mutableCopy];
                if (1 == [newKey replaceOccurrencesOfString:@".txt" withString:@".zip"
                    options:NSCaseInsensitiveSearch range:NSMakeRange(0, [newKey length])]) {
                    [urls setObject:[urls objectForKey:key] forKey:newKey];
                    [urls removeObjectForKey:key];
                }
                [newKey release];
            }
        } @finally{}
    }
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification
{
    LIST_EPOCH = [[NSDate dateWithTimeIntervalSince1970:(NSTimeInterval)1093597930] retain];
    
    NSDictionary * defaultPrefs = [NSDictionary dictionaryWithContentsOfFile:
        [[NSBundle mainBundle] pathForResource:@"defaults" ofType:@"plist"]];
	
    NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
    [ud registerDefaults:defaultPrefs];
    
    if ([ud boolForKey:@"RestoreFilterState"])
        ppdisabled = ![ud boolForKey:@"FilterEnabled"];
    
    NSArray *mylists = [ud arrayForKey:@"Lists"];
    lists = [[NSMutableArray alloc] initWithCapacity:[mylists count]];
    internalLists = [[NSMutableArray alloc] init];
    NSEnumerator *en = [mylists objectEnumerator];
    PPList *list;
    while((list = [[en nextObject] mutableCopy])) {
        // Create UUID if necesssary
        if (nil == ListID(list)) {
            NSString *uustr = nil;
            pg_table_id_t listid;
            pg_table_id_create(listid, (CFStringRef*)&uustr);
            if (uustr) {
                ListSet(list, uustr, @"UUID");
                [uustr release];
            }
        }
        if (nil == ListID(list))
            ListSet(list, [NSNumber numberWithBool:NO], @"Active");
        
        // Convert temp list
        u_int32_t oldid = [[list objectForKey:@"ID"] unsignedIntValue];
        if (OBSOLETE_LIST_ID_TEMP_ALLW == oldid)
            ListSet(list, [NSNumber numberWithBool:YES], @"Temp");
        
        [list removeObjectForKey:@"ID"]; // Obsolete
        
        // Convert old @"URL" & @"Secondary URLs"
        NSURL *url;
        NSMutableDictionary *newEntries;
        if (nil == ListURLs(list) && (url = [list objectForKey:@"URL"])) {
            newEntries = [NSMutableDictionary dictionaryWithObject:
                LIST_EPOCH forKey:url];
            NSDictionary *secondaryURLs = [list objectForKey:@"Secondary URLs"];
            if (secondaryURLs && [secondaryURLs count])
                [newEntries addEntriesFromDictionary:secondaryURLs];
            
            if (newEntries && [newEntries count] > 0) {
                [list setObject:newEntries forKey:@"URLs"];
                [list removeObjectForKey:@"URL"];
                [list removeObjectForKey:@"Secondary URLs"];
                
                // reset epoch so we get fresh downloads
                [self resetLastUpdateTime:list];
                [self updateBluetackURLs:list];
            }
        } else {
            // mutableCopy does not "deep" copy
            newEntries = [[list objectForKey:@"URLs"] mutableCopy];
            if (newEntries) {
                ListSet(list, newEntries, @"URLs");
                [newEntries release];
            }
        }
        
        [lists addObject:list];
        if (ListInternal(list))
            [internalLists addObject:list];
        [list release];
    }
    
    // DO Server
    [[NSConnection defaultConnection] registerName:
        [NSString stringWithFormat:@"%s+%ul", PP_CTL_NAME, getuid()]];
    [[NSConnection defaultConnection] setRootObject:
        [NSProtocolChecker protocolCheckerWithTarget:self protocol:
            @protocol(PPLoaderDOServerExtensions)]];
    [[NSConnection defaultConnection] setReplyTimeout:5.0];
    [[NSConnection defaultConnection] setRequestTimeout:5.0];
    [[NSConnection defaultConnection] setIndependentConversationQueueing:YES];
    
    NSDistributedNotificationCenter *nc = [NSDistributedNotificationCenter defaultCenter];
    [nc addObserver:self selector:@selector(kextDidLoad:) name:PP_KEXT_DID_LOAD object:nil];
    [nc addObserver:self selector:@selector(kextFailedLoad:) name:PP_KEXT_FAILED_LOAD object:nil];
    [nc addObserver:self selector:@selector(kextFailedUnload:) name:PP_KEXT_FAILED_UNLOAD object:nil];
    [nc addObserver:self selector:@selector(handleQuit:) name:PP_HELPER_QUIT object:nil];
    [nc addObserver:self selector:@selector(handleAddrActionAsk:) name:PP_ADDR_ACTION_ASK object:nil];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSNotification*)note
{
    return (NSTerminateNow);
}

- (void)applicationWillTerminate:(NSNotification*)note
{
    [self syncPrefs:nil];
    
    NSArray *tmp = [self tempLists];
    (void)[self unloadLists:tmp makeInactive:YES];
    
    [self syncPrefs:nil];
}

// DO protocol
- (unsigned int)registerClient:(NSDistantObject*)client
{
    [client setProtocolForProxy:@protocol(PPLoaderDOClientExtensions)];
    struct ppclient *cl = (struct ppclient*)malloc(sizeof(*cl));
    if (cl) {
        bzero(cl, sizeof(*cl));
        cl->cid = ppclientid;
        cl->client = [client retain];
        pptrace(@"registered: %d, %p\n", cl->cid, client);
        ppclientid++;
        LIST_INSERT_HEAD(&clients, cl, link);
        return (cl->cid);
    } else
        return (-1);
}

- (void)deregisterClient:(unsigned int)clientID
{
    struct ppclient *cl;
    
    if (-1 == clientID)
        return;
    
    LIST_FOREACH(cl, &clients, link) {
        if (cl->cid = clientID)
            break;
    }
    
    if (cl) {
        LIST_REMOVE(cl, link);
        free(cl);
    }
}

- (NSArray*)lists
{
    NSMutableArray *public = [lists mutableCopy];
    [public removeObjectsInArray:internalLists];
    return ([public autorelease]);
}

- (NSMutableDictionary*)createList
{
    NSMutableDictionary *list = [NSMutableDictionary dictionary];
    
    NSNumber *no = [NSNumber numberWithBool:NO];
    NSNumber *yes = [NSNumber numberWithBool:YES];
    
    char buf[64];
    pg_table_id_string(PP_LIST_NEWID, buf, sizeof(buf), 1);
    NSString *uustr = [NSString stringWithUTF8String:buf];
    [list setObject:uustr forKey:@"UUID"];
    
    [list setObject:yes forKey:@"Active"];
    [list setObject:no forKey:@"Allow"];
    [list setObject:no forKey:@"Block HTTP"];
    [list setObject:@"" forKey:@"Description"];
    [list setObject:[NSMutableDictionary dictionary] forKey:@"URLs"];
    [list setObject:LIST_EPOCH forKey:@"Last Update"];
    [list setObject:yes forKey:@"User Gen"];
    return (list);
}

- (NSMutableDictionary*)userList:(BOOL)blockList
{
    PPList *list;
    NSURL *listURL;
    NSEnumerator *en = [lists objectEnumerator];
    while((list = [en nextObject])) {
        if (ListUser(list) && (!blockList) == ListAllow(list) && ListActive(list)) {
            listURL = [self firstURL:list];
            if (listURL && [listURL isFileURL])
                break;
        }
    }
    return (list);
}

- (oneway void)updateAllLists
{
    [self performSelector:@selector(updateLists:) withObject:nil afterDelay:0.0];
}

- (oneway void)updateList:(id)list
{
    if (list && [list isKindOfClass:[NSDictionary class]]) {
        if ((list = [self listWithIDStr:ListID(list)])) {
            [self downloadList:list];
            [list release];
        }
    }
}

// change/add list
- (void)validateList:(PPList*)list
{
    NSDate *mod = ListLastMod(list);
    if (!mod || [mod isGreaterThan:[NSDate date]])
        [self resetLastUpdateTime:list]; // Make sure we go to the network
}

- (void)listChanged:(id)list watch:(BOOL)watch
{
    pptrace(@"listChanged: '%@'", ListName(list));
    [self notifyClientsUsingSelector:@selector(listDidChange:) object:list];
    
    if (!ListActive(list))
        return;
    
    NSURL *url = [self firstURL:list];
    if (!url)
        NSLog(@"Null URL for list: '%@'!\n", ListName(list));
    if ([url isFileURL]) {
        NSString *path = [url path];
        if (watch)
            [self watchFile:path forList:list];
        if (0 == [self loadList:list fromFile:path])
            [self listUpdated:list];
    } else {
        // reload from cache incase the download is not modified
        [self loadListFromCache:list];
        [self downloadList:list];
    }
}

- (void)listChanged:(id)list
{
    [self listChanged:list watch:YES];
}

- (BOOL)addList:(id)list
{
    BOOL good = NO;
    NSArray *urls = ListURLs(list);
    if (list && [list isKindOfClass:PPListClass] && urls && [urls count] > 0) {
        pg_table_id_t listid;
        good = (0 == pg_table_id_fromcfstr((CFStringRef)ListID(list), listid));
        
        if (!good || 0 != pg_table_id_cmp(PP_LIST_NEWID, listid))
            return (NO);
        
        NSString *uustr = nil;
        pg_table_id_create(listid, (CFStringRef*)&uustr);
        if (!uustr)
            return (NO);
        
        ppassert(nil == [self listWithID:listid]);

        ListSet(list, uustr, @"UUID");
        ListSet(list, [NSNumber numberWithBool:YES], @"User Gen");
        [uustr release];            
        
        NSMutableDictionary *urls = [[[list objectForKey:@"URLs"] mutableCopy] autorelease];
        ListSet(list, urls, @"URLs");

        [lists addObject:list];
        [self validateList:list];
        
        pptrace(@"addList: '%@' with id: %@", ListName(list), ListID(list));
        
        // since we are in a sync method, perform updates next time through the event loop
        [self performSelector:@selector(listChanged:) withObject:list afterDelay:0.0];
        good = YES;
    }
    return (good);
}

- (BOOL)changeList:(id)list
{
    BOOL good = NO;
    NSArray *urls = ListURLs(list);
    if (list && [list isKindOfClass:PPListClass] && urls && [urls count] > 0 && !ListInternal(list)) {
        pg_table_id_t listid;
        good = (0 == pg_table_id_fromcfstr((CFStringRef)ListID(list), listid));
        if (!good || 0 == pg_table_id_cmp(PP_LIST_NEWID, listid))
            return (NO);
        PPList *oldList = [self listWithID:listid];
        
        [self validateList:list];
        
        if (oldList) {
            unsigned idx = [lists indexOfObject:oldList];
            if (NSNotFound != idx) {// double check
                good = YES;
                NSMutableDictionary *urls = [[[list objectForKey:@"URLs"] mutableCopy] autorelease];
                ListSet(list, urls, @"URLs");
                [lists replaceObjectAtIndex:idx withObject:list];
                pptrace(@"changeList: '%@' with id: %@", ListName(list), ListID(list));
                // since we are in a sync method, perform updates next time through the event loop
                [self performSelector:@selector(listChanged:) withObject:list afterDelay:0.0];
                
                if (ListActive(oldList) && !ListActive(list))
                    (void)pp_unload_table(listid);
            }
            [oldList release];
        }
    }
    return (good);
}

- (oneway void)removeList:(id)list
{
    if (list && [list isKindOfClass:[NSDictionary class]] && !ListInternal(list)) {
        if ((list = [self listWithIDStr:ListID(list)])) {
            if (ListActive(list)) {
                if (0 != [self unloadList:list])
                    return;
            }
            
            [lists removeObject:list];
            [self notifyClientsUsingSelector:@selector(listWasRemoved:) object:list];
            
            [self ignoreWatchedFiles:list];
            [list release];
            
            [self syncPrefs:nil];
        }
    }
}

- (int)loadPortRule:(NSString*)rule
{
    const char *rp = [rule cStringUsingEncoding:NSASCIIStringEncoding];
    if (!rp)
        return (EINVAL);
    
    int len = strlen(rp);
    if (!len)
        return (ENOENT);
    
    char *buf = malloc(len+1);
    if (!buf)
        return (ENOMEM);
    
    strcpy(buf, rp);
    
    pp_msg_iprange_t *ports;
    int32_t count;
    int err = p2b_parseports(buf, len, &ports, &count);
    
    free(buf);
    if (err)
        return (err);
    
    if (!(err = pp_load_table(ports, count, 0, PP_TABLE_ID_PORTS, PP_LOAD_PORT_RULE)))
        [[NSUserDefaults standardUserDefaults] setObject:rule forKey:PP_PREF_PORT_RULES];
    else
        NSLog(@"Failed to load port list: %d.\n", err);
    
    free(ports);
    return (err);
}

- (int)setPortRule:(NSString*)rule
{
    NSString *current = [[NSUserDefaults standardUserDefaults] objectForKey:PP_PREF_PORT_RULES];
    if (rule && [current isEqualToString:rule])
        return (0);
    
    return ([self loadPortRule:rule]);
}

- (NSMutableDictionary*)createUserAllowList
{
    NSString *path = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
    if (path)
        path = [path stringByAppendingPathComponent:@"PeerGuardianAllow.p2b"];
    else
        return (nil);

    NSMutableDictionary *list = [self createList];
    pg_table_id_t tid;
    NSString *str;
    pg_table_id_create(tid, (CFStringRef*)&str);
    if (str) {
        ListSet(list, str, @"UUID");
        [str release];
    } else
        return (nil);
    
    ListSet(list, [NSNumber numberWithBool:YES], @"Active");
    ListSet(list, [NSNumber numberWithBool:YES], @"User Gen");
    ListSet(list, [NSNumber numberWithBool:YES], @"Allow");
    str = [NSString stringWithFormat:
        NSLocalizedString(@"%@'s allowed addresses", "user's allowed addresses"), NSFullUserName()];
    ListSet(list, str, @"Description");
    ListSet(list, [NSNumber numberWithBool:NO], @"Internal");
    ListSet(list, [NSMutableDictionary dictionaryWithObject:[NSDate date]
        forKey:[[NSURL fileURLWithPath:path] absoluteString]],
        @"URLs");
    [self validateList:list];
    
    ppassert(nil == [self listWithID:tid]);
    [lists addObject:list];
    [self listChanged:list];
    return (list);
}

// get pref setting
- (id)objectForKey:(NSString*)key
{
    id pref = nil;
    if ([key isEqualToString:@"CacheRoot"])
        pref = CACHE_ROOT;
    else if (![key isEqualToString:@"Lists"])
        pref = [[NSUserDefaults standardUserDefaults] objectForKey:key];
    return (pref);
}

- (oneway void)setObject:(id)obj forKey:(NSString*)key
{
    if (obj && key && ![key isEqualToString:@"Lists"] && ![key isEqualToString:@"Next UID"]
        && [obj isKindOfClass:[NSNumber class]]) {
        BOOL good = YES;
        if ([key isEqualToString:@"Update Interval"]) {
            [self setUpdateInterval:[obj doubleValue]];
            [updateTimer fire];
        } else if ([key isEqualToString:@"Log Allowed"]) {
            [[NSUserDefaults standardUserDefaults] setBool:[obj boolValue] forKey:key];
        } else if ([key isEqualToString:@"Auto-Allow DNS"]) {
            [[NSUserDefaults standardUserDefaults] setBool:[obj boolValue] forKey:key];
            
            PPList *allowList = [self listWithID:PG_TABLE_ID_ALLW_LOCAL];
            if ([obj boolValue] && allowList) {
                ListSet(allowList, [NSNumber numberWithBool:YES], @"Active");
                NSString *path = [[self firstURL:allowList] path];
                if (path) 
                    [self loadList:allowList fromFile:path];
            } else if (allowList) {
                ListSet(allowList, [NSNumber numberWithBool:NO], @"Active");
                pp_unload_table(PG_TABLE_ID_ALLW_LOCAL);
            }
        } else
            good = NO;
        
        if (good) {
            [self syncPrefs:nil];
            [[NSDistributedNotificationCenter defaultCenter]
                postNotificationName:PP_PREF_CHANGED
                object:nil userInfo:[NSDictionary dictionaryWithObjectsAndKeys:
                    key, PP_PREF_KEY,
                    obj, PP_PREF_VALUE,
                    nil]];
        }
    }
}

- (void)handleAddrActionAsk:(NSNotification*)note
{
    id win = [[PPAddrActionAskController alloc] initWithAddressSpecification:
        [note userInfo] delegate:self];
    [NSApp activateIgnoringOtherApps:YES];
    [win showWindow:self];
}

- (void)expireTempAllowAddresses:(NSTimer*)timer
{
    PPList *list = [self tempAllowList];
    if (!list) {
        NSLog(@"%s - missing list!\n", __FUNCTION__);
        return;
    }
    
    NSEnumerator *en = [tempAddresses objectEnumerator];
    NSTimeInterval now = [[NSDate date] timeIntervalSince1970];
    NSMutableArray *removed = [[NSMutableArray alloc] init];
    NSDictionary *entry;
    while ((entry = [en nextObject])) {
        NSTimeInterval expires = [[entry objectForKey:PP_ADDR_SPEC_EXPIRE] timeIntervalSince1970];
        if (expires <= now)
            [removed addObject:entry];
    }
    
    if ([removed count]) {
        [tempAddresses removeObjectsInArray:removed];
        if ([tempAddresses count] > 0) {
            NSArray *addrsCopy = [tempAddresses copy];
            int err = [self commitAddressSpecifications:addrsCopy
                toFile:[[self firstURL:list] path] inBackground:YES];
            [addrsCopy release];
            if (err)
                NSLog(@"%s - commit failed with: %d\n", __FUNCTION__, err);
        }
    }
    
    [removed release];
    
    if (0 == [tempAddresses count]) {
        [tempAddressScanTimer invalidate];
        tempAddressScanTimer = nil;
        if (0 == [self unloadList:list])
            ListSet(list, [NSNumber numberWithBool:NO], @"Active");
    }
}

- (int)addTempAllowAddress:(NSDictionary*)spec
{
    NSNumber *expire;
    NSString *ip4, *addrName;
    BOOL allow;
    
    ip4 = [spec objectForKey:PP_ADDR_SPEC_START];
    expire = [spec objectForKey:PP_ADDR_SPEC_EXPIRE];
    allow = (![spec objectForKey:PP_ADDR_SPEC_BLOCK] || NO == [[spec objectForKey:PP_ADDR_SPEC_BLOCK] boolValue]);
    if (!ip4 || !expire || !allow /*don't currently support temp blocks*/) {
        NSLog(@"%s - invalid spec: %@\n", __FUNCTION__, spec);
        return (EINVAL);
    }
    
    addrName = [spec objectForKey:PP_ADDR_SPEC_NAME];
    
    unsigned secs = [expire unsignedIntValue];
    NSString *path;
    if (UINT_MAX == secs) {
        // perm allow, find the first user list and add it
        PPList *list = [self userList:NO];
        if (!list)
            list = [self createUserAllowList];
        if (!list) {
            NSLog(@"%s - no permanent allow list found, can't add: %@\n", __FUNCTION__, spec);
            return (ENOENT);
        }
        NSURL *listURL = [self firstURL:list];
        
        path = [listURL path];
        NSMutableArray *ranges = [self addressSpecificationsFromFile:path];
        if (ranges)
            [ranges addObject:spec];
        else
            ranges = [NSMutableArray arrayWithObject:spec];
        [self commitAddressSpecifications:ranges toFile:path inBackground:YES];
        return (0);
    }
    
    PPList *allowList = [self tempAllowList];
    pg_table_id_t listid;
    if (!allowList) {
        allowList = [self createList];
        NSString *uustr = nil;
        pg_table_id_create(listid, (CFStringRef*)&uustr);
        if (uustr) {
            ListSet(allowList, uustr, @"UUID");
            [uustr release];
        } else
            return (ENOMEM);
        
        ListSet(allowList, [NSNumber numberWithBool:NO], @"User Gen");
        ListSet(allowList, [NSNumber numberWithBool:YES], @"Allow");
        ListSet(allowList, [NSNumber numberWithBool:YES], @"Temp");
        ListSet(allowList, NSLocalizedString(@"Temporary Allow", ""), @"Description");
        ListSet(allowList, [NSNumber numberWithBool:YES], @"Internal");
        path = [CACHE_ROOT stringByAppendingPathComponent:@".pgtempallow.p2b"];
        ListSet(allowList, [NSMutableDictionary dictionaryWithObject:[NSDate date]
            forKey:[[NSURL fileURLWithPath:path] absoluteString]],
            @"URLs");
        [self validateList:allowList];
        
        ppassert(nil == [self listWithID:listid]);
        [lists addObject:allowList];
        [internalLists addObject:allowList];
    } else if (NO == ListActive(allowList))
        ListSet(allowList, [NSNumber numberWithBool:YES], @"Active");
    
    if (!tempAddresses)
        tempAddresses = [[NSMutableArray alloc] init];
        
    NSMutableDictionary *specCopy = [spec mutableCopy];
    NSDate *expireDate = [NSDate dateWithTimeIntervalSinceNow:(NSTimeInterval)secs];
    [specCopy setObject:expireDate forKey:PP_ADDR_SPEC_EXPIRE];
    if (!addrName)
        [specCopy setObject:NSLocalizedString(@"Temporary Allow", "") forKey:PP_ADDR_SPEC_NAME];
    [tempAddresses addObject:specCopy];
    [specCopy release];
    
    path = [[self firstURL:allowList] path];
    int err = [self commitAddressSpecifications:tempAddresses toFile:path];
    if (0 == err) {
        // All changes come through this method, so there's no need to watch the file
        [self listChanged:allowList watch:NO];
    }
    
    if (!tempAddressScanTimer)
        tempAddressScanTimer = [NSTimer scheduledTimerWithTimeInterval:60.0 target:self
            selector:@selector(expireTempAllowAddresses:) userInfo:nil repeats:YES];
    
    return (err);
}

- (int)addAllowedLocalAddress:(NSDictionary*)spec clearCurrentEntries:(BOOL)clear load:(BOOL)load
{
    NSString *ip4 = [spec objectForKey:PP_ADDR_SPEC_START];
    if (!ip4) {
        NSLog(@"%s - invalid spec: %@\n", __FUNCTION__, spec);
        return (EINVAL);
    }
    
    PPList *allowList = [self listWithID:PG_TABLE_ID_ALLW_LOCAL];
    if (!allowList) {
        allowList = [self createList];
        
        char strbuf[64];
        pg_table_id_string(PG_TABLE_ID_ALLW_LOCAL, strbuf, 0, 1);
        NSString *uustr = [[NSString alloc] initWithUTF8String:strbuf];
        if (uustr) {
            ListSet(allowList, uustr, @"UUID");
            [uustr release];
        } else
            return (ENOMEM);
        
        ListSet(allowList, [[NSUserDefaults standardUserDefaults] objectForKey:@"Auto-Allow DNS"], @"Active");
        ListSet(allowList, [NSNumber numberWithBool:NO], @"User Gen");
        ListSet(allowList, [NSNumber numberWithBool:YES], @"Allow");
        ListSet(allowList, NSLocalizedString(@"Allowed network configuration addresses", ""), @"Description");
        ListSet(allowList, [NSNumber numberWithBool:YES], @"Internal");
        NSString *path = [CACHE_ROOT stringByAppendingPathComponent:@".pglocalallow.p2b"];
        ListSet(allowList, [NSMutableDictionary dictionaryWithObject:[NSDate date]
            forKey:[[NSURL fileURLWithPath:path] absoluteString]],
            @"URLs");
        [self validateList:allowList];
        
        ppassert(nil == [self listWithID:PG_TABLE_ID_ALLW_LOCAL]);
        [lists addObject:allowList];
        [internalLists addObject:allowList];
    }
    
    NSMutableDictionary *specCopy = [spec mutableCopy];
    NSString *path = [[self firstURL:allowList] path];
    NSMutableArray *ranges = NO == clear ? [self addressSpecificationsFromFile:path] : nil;
    if (ranges)
        [ranges addObject:specCopy];
    else
        ranges = [NSMutableArray arrayWithObject:specCopy];
    [specCopy release];
    
    int err = [self commitAddressSpecifications:ranges toFile:path];
    if (load && 0 == err && ListActive(allowList)) {
        // All changes come through this method, so there's no need to watch the file
        [self listChanged:allowList watch:NO];
    }
    
    return (err);
}

- (BOOL)isEnabled
{
    return (NO == ppdisabled);
}

- (int)disable
{
    int err = 0;
    if (!ppdisabled) {
        if (0 == (err = pp_unload_table(PP_TABLE_ID_ALL))) {
            ppdisabled = YES;
            [[NSDistributedNotificationCenter defaultCenter]
                postNotificationName:PP_FILTER_CHANGE
                object:nil userInfo:[NSDictionary dictionaryWithObjectsAndKeys:
                    [NSNumber numberWithBool:NO], PP_FILTER_CHANGE_ENABLED_KEY, 
                    nil]];
            [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"FilterEnabled"];
        }
    }
    return (err);
}

- (oneway void)enable
{
    if (ppdisabled) {
        ppdisabled = NO;
        [self performSelector:@selector(kextDidLoad:) withObject:nil afterDelay:0.0];
        [[NSDistributedNotificationCenter defaultCenter]
            postNotificationName:PP_FILTER_CHANGE
            object:nil userInfo:[NSDictionary dictionaryWithObjectsAndKeys:
                [NSNumber numberWithBool:YES], PP_FILTER_CHANGE_ENABLED_KEY, 
                nil]];
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"FilterEnabled"];
    }
}

// Download support

static NSString* LocalCacheFile(NSString *urlString)
{
    NSURL *url = [NSURL URLWithString:urlString];
    if (url) {
        NSMutableString *path = [[[url host] mutableCopy] autorelease];
        [path appendString:[url path]];
        NSRange r = {0};
        r.length = [path length];
        [path replaceOccurrencesOfString:@"/" withString:@"_" options:0 range:r];
        r.length = [path length];
        [path replaceOccurrencesOfString:@":" withString:@"_" options:0 range:r];
        [path insertString:@"." atIndex:0];
        return ([CACHE_ROOT stringByAppendingPathComponent:path]);
    }
    
    return (nil);
}

- (void)mergeListsDidComplete:(NSMutableDictionary*)args
{
    (void)[args retain];
    
    PPList *list = (id)[args objectForKey:@"list"];
    NSString *output = [args objectForKey:@"output"];
    int err = [[args objectForKey:@"err"] intValue];
    if (0 == err)
        err = [self loadList:list fromFile:output];
    else
        NSLog(@"Merging list '%@' failed with: %d\n", ListName(list), err);
    
    NSMutableArray *files = [[[[args objectForKey:@"inputs"] allValues] mutableCopy] autorelease];
    if (0 == err) {
        NSString *outleaf = [output lastPathComponent];
        if (NO == [[ListCacheFile(list) lastPathComponent] isEqualToString:outleaf]) {
            [self setCacheFile:outleaf forList:list];
            // add in the old cache file for deletion
            if ([args objectForKey:@"CacheFile"])
                [files addObject:[args objectForKey:@"CacheFile"]];
        }
        NSArray *completed = [[args objectForKey:@"inputs"] allKeys];
        [self listUpdated:list withURLs:completed];
    } else {
        NSError *error = [NSError errorWithDomain:NSPOSIXErrorDomain code:err userInfo:nil];
        [self notifyClientsUsingSelector:@selector(listFailedUpdate:error:) object:list object2:error];
    }
    
    // Removed completed files
    NSFileManager *fm = [NSFileManager defaultManager];
    NSEnumerator *en = [files objectEnumerator];
    NSString *path;
    while ((path = [en nextObject]))
        (void)[fm removeFileAtPath:path handler:nil];
    
    [args release];
}

- (void)mergeListsThread:(id)obj
{
    /* Merging aggregates is tricky, because there may have been URLs
       that were not modified or had an error and thus were not downloaded.
       
       We also have a Catch 22 in that some ranges may have been removed from newly
       downloaded lists, but they still exist in the list Cache file. Thus merging in the
       existing cache file keeps ranges that should have been discarded.
       
       To handle both of these situations, a cache of each individual URL is kept and
       used as necessary.
     */
    
    do {
    
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSMutableDictionary *args;
merge_wait_for_work:
    [dlLock lockWhenCondition:DL_HAVE_WORK];
    unsigned qCount = [dlQueue count];
    if (qCount) {
        args = [[[dlQueue objectAtIndex:0] retain] autorelease];
        [dlQueue removeObjectAtIndex:0];
        --qCount;
    } else
        args = nil;
    [dlLock unlockWithCondition:(qCount > 0 ? DL_HAVE_WORK : DL_NO_WORK)];
    if (!args)
        goto merge_wait_for_work;
    
    // NSLog(@"Thread %p got work %p\n", (void*)[NSThread currentThread], args);
    
    const char *output = [[args objectForKey:@"output"] fileSystemRepresentation];
    NSArray *files = [[args objectForKey:@"inputs"] allValues];
    NSArray *notUpdated = [args objectForKey:@"not updated"];
    
    int sz = sizeof(char*) * ([files count] + [notUpdated count] + 2), err;
    char **paths = malloc(sz);
    if (!paths) {
        err = ENOMEM;
        goto mergeListsError;
    }
    bzero(paths, sz);
    
    int i = 0;
    NSMutableArray *decompressedFiles = [NSMutableArray array];
    NSEnumerator *en = [files objectEnumerator];
    NSString *path, *origPath;
    NSMutableArray *badURLs = [NSMutableArray array];
    while ((origPath = path = [en nextObject])) {
        if ((path = [self decompressFile:path])) {
            paths[i] = (char*)[path fileSystemRepresentation];
            ++i;
            if (path != origPath && NO == [path isEqualToString:origPath])
                [decompressedFiles addObject:path];
        } else {
            NSArray *failedURLs = [[args objectForKey:@"inputs"] allKeysForObject:origPath];
            if ([failedURLs count] > 0) {
                notUpdated = [notUpdated arrayByAddingObject:[failedURLs objectAtIndex:0]];
                [badURLs addObjectsFromArray:failedURLs];
            }
            NSLog(@"Failed to decompress %@ ('%@')\n", failedURLs, origPath);
        }
    }
    if ([badURLs count] > 0) {
        [[args objectForKey:@"inputs"] removeObjectsForKeys:badURLs];
    }
    
    // Add in cache files for URLs that were not updated (due to failure or not modified)
    // NSFileManager is not thread safe, so use POSIX
    struct stat sb;
    en = [notUpdated objectEnumerator]; // Array of URL strings
    while ((path = [en nextObject])) {
        pptrace(@"Attempting to merge %@ from local cache.\n", path);
        path = LocalCacheFile(path);
        if (path && 0 == stat([path fileSystemRepresentation], &sb)
            && (path = [self decompressFile:path])) {
            paths[i] = (char*)[path fileSystemRepresentation];
            ++i;
            if (path != origPath && NO == [path isEqualToString:origPath])
                [decompressedFiles addObject:path];
        }
    }
    
    err = p2b_merge(output, P2B_VERSION2, paths);
#ifdef PPDBG
    if (err && ERANGE != err && i)
        dbgbrk();
#endif

mergeListsError:
    [args setObject:[NSNumber numberWithInt:err] forKey:@"err"];

    // Move completed files to local cache
    const char *from, *to;
    NSDictionary *inputs = [args objectForKey:@"inputs"];
    en = [[inputs allKeys] objectEnumerator];
    while ((path = [en nextObject])) {
        from = (char*)[[inputs objectForKey:path] fileSystemRepresentation];
        to = (char*)[LocalCacheFile(path) fileSystemRepresentation];
        if (from && to) {
            if (0 == stat(to, &sb))
                (void)unlink(to);
            // try a hard link first, if that fails, copy the file
            if (-1 == link(from, to)) {
                #if 0
                // fails with EPERM for some reason (originating from openx_np)
                if (0 != (err = copyfile(from, to, NULL, COPYFILE_STAT|COPYFILE_DATA)))
                #endif
                if (0 != (err = p2b_exchange(from, to)))
                    NSLog(@"Failed to create local cache file for '%@' (%d).\n", path, err);
            }
        }
    }
    
    [self performSelectorOnMainThread:@selector(mergeListsDidComplete:) withObject:args waitUntilDone:NO];
    
    // Remove decompressed temp files - input files are removed by mergeListsDidComplete:
    en = [decompressedFiles objectEnumerator];
    while ((path = [en nextObject]))
        (void)unlink([path fileSystemRepresentation]);
    
    // NSLog(@"Thread %p finished work %p\n", (void*)[NSThread currentThread], args);
    
    [pool release];
    
    if (paths)
        free(paths);
    
    } while(1);
}

- (void)downloadDidFinish:(PPList*)list withTempFiles:(NSDictionary*)completed withNotUpdated:(NSArray*)notUpdated
{
    NSString *cachePath = ListCacheFile(list);
    if (!cachePath) {
        NSLog(@"Could not create cache path for '%@'. Aborting download.\n", list);
        return;
    }
    
    NSString *output = [[cachePath stringByDeletingPathExtension]
        stringByAppendingPathExtension:@"p2b"];
    
    if (NO == [[NSFileManager defaultManager] fileExistsAtPath:cachePath])
        cachePath = nil;
    
    pptrace(@"downloadDidFinish: for '%@' with completions: %@\nwith not updated: %@",
        ListName(list), completed, notUpdated);
    
    NSMutableDictionary *args = [NSMutableDictionary dictionaryWithObjectsAndKeys:
        list, @"list",
        [[completed mutableCopy] autorelease], @"inputs",
        output, @"output",
        notUpdated, @"not updated",
        cachePath, @"CacheFile", // can be nil, so must be last
        nil];

#ifdef stepping_threads_in_gdb_is_not_fun
    [self mergeListsThread:args];
#else
    [dlLock lock];
    [dlQueue addObject:args];
    [dlLock unlockWithCondition:DL_HAVE_WORK];
#endif
}

- (int)downloadDependendencies:(id)key
{
    NSMutableArray *container = [[downloads objectForKey:key] objectForKey:@"dependencies"];
    return ([[container objectAtIndex:0] intValue]);
}

- (void)decrementDownloadDependendencies:(id)key
{
    NSDictionary *context = [downloads objectForKey:key];
    ppassert(nil != context);
    
    NSMutableArray *container = [context objectForKey:@"dependencies"];
    NSNumber *count = [container objectAtIndex:0];
    count = [NSNumber numberWithInt:[count intValue] - 1]; 
    [container replaceObjectAtIndex:0 withObject:count];
    
    if ([count intValue] <= 0) {
        NSMutableDictionary *completions = [context objectForKey:@"completed"];
        if ([completions count] > 0)
            [self downloadDidFinish:[context objectForKey:@"list"] withTempFiles:completions
                withNotUpdated:[context objectForKey:@"not updated"]];
    }
}

// AppleScript support
// Useful debug info: defaults write NSGlobalDomain NSScriptingDebugLogLevel 1
- (BOOL)application:(NSApplication *)sender delegateHandlesKey:(NSString *)key
{
    BOOL handled = [key isEqualToString:@"filtersEnabled"];
    pptrace(@"wants key: %@ (%d)\n", key, handled);
    return (handled);
}

- (BOOL)filtersEnabled
{
    return ([self isEnabled]);
}

- (void)setFiltersEnabled:(BOOL)enable
{
    if (enable)
        [self enable];
    else
        (void)[self disable];
}

// URLDownload delegate methods
- (void)download:(NSURLDownload *)download didReceiveResponse:(NSURLResponse *)response
{
    pptrace(@"-%s- (%@)\n", __FUNCTION__, [[download request] URL]);
    
    NSValue *dlkey = [NSValue valueWithPointer:(void*)download];
    NSDictionary *context;
    if (nil == (context = [downloads objectForKey:dlkey]))
        dbgbrk();
    
    if ([response respondsToSelector:@selector(statusCode)]
        && 304 == [(NSHTTPURLResponse*)response statusCode]) {
        NSValue *dlkey = [NSValue valueWithPointer:(void*)download];
        pptrace(@"'%@' not modified, canceling download.\n", [[download request] URL]);
        NSMutableArray *notUpdated = [context objectForKey:@"not updated"];
        [notUpdated addObject:[context objectForKey:@"original URL"]];
        [self decrementDownloadDependendencies:dlkey];
        [downloads removeObjectForKey:dlkey];
        [download cancel];
    }
}

- (void)download:(NSURLDownload *)download decideDestinationWithSuggestedFilename:(NSString *)filename
{
    NSString *path;
#ifdef PPDBG
    NSURL *url = [[download request] URL];
    pptrace(@"-%s- (%@)\n", __FUNCTION__, url);
#endif
    
    // find the download tracker
    NSValue *dlkey = [NSValue valueWithPointer:(void*)download];
    NSMutableDictionary *d = [downloads objectForKey:dlkey];
    if (d) {
        // Create a unique name incase the user somehow ended up with duplicate URL's.
        char tmp[] = "pgdlXXXXXX";
        (void*)mktemp(tmp);
        path = [NSTemporaryDirectory() stringByAppendingPathComponent:
            [NSString stringWithCString:tmp encoding:NSASCIIStringEncoding]];
        [download setDestination:path allowOverwrite:NO];
        [download setDeletesFileUponFailure:YES];
    } else {
        dbgbrk();
        NSLog(@"decideFileName: Failed to find download for URL '%@'\n", [[download request] URL]);
        return;
    }
}

- (void)download:(NSURLDownload *)download didCreateDestination:(NSString*)filename
{
    // find the download tracker
    NSValue *dlkey = [NSValue valueWithPointer:(void*)download];
    NSMutableDictionary *d = [downloads objectForKey:dlkey];
    if (d)
        [d setObject:filename forKey:@"path"];
    else {
        dbgbrk();
        NSLog(@"didCreate: Failed to find download for URL '%@'\n", [[download request] URL]);
    }
}

- (NSURLRequest *)download:(NSURLDownload *)download willSendRequest:(NSURLRequest *)request
    redirectResponse:(NSURLResponse *)redirectResponse
{
    // On Leopard, the system will notify us if it modifies the URL before any connection has been made.
    // In this case, redirectResponse will be nil
    NSURL *originalURL = [[download request] URL];
    pptrace(@"Redirect from '%@' (('%@')) to '%@'.\n", originalURL,
        [redirectResponse URL], [request URL]);
    
    NSValue *dlkey = [NSValue valueWithPointer:(void*)download];
    NSMutableDictionary *d = [downloads objectForKey:dlkey];
    if (d) {
        if ([@"http" isEqualToString:[[request URL] scheme]]
            && [@"http" isEqualToString:[originalURL scheme]]) {
            NSDictionary *hdrs = [[download request] allHTTPHeaderFields];
            if (hdrs) {
                NSMutableURLRequest *req = [request mutableCopy];
                if (req) {
                    [req setAllHTTPHeaderFields:hdrs];
                    request = [req autorelease];
                }
            }
        }
    } else {
        dbgbrk();
        NSLog(@"willSend: Failed to find download for URL '%@'\n", originalURL);
    }
    
    return (request);
}

- (void)downloadDidBegin:(NSURLDownload *)download
{
    NSValue *dlkey = [NSValue valueWithPointer:(void*)download];
    PPList *list = [[downloads objectForKey:dlkey] objectForKey:@"list"];
    pptrace(@"Downloading '%@' from '%@'.\n", ListName(list), [[download request] URL]);
    if (list)
        [self notifyClientsUsingSelector:@selector(listWillUpdate:) object:list];
#ifdef PPDBG
    else
        dbgbrk();
#endif
}

- (void)downloadDidFinish:(NSURLDownload *)download
{
    NSValue *dlkey = [NSValue valueWithPointer:(void*)download];
    NSDictionary *context = [downloads objectForKey:dlkey]; 
    PPList *list = [context objectForKey:@"list"];
    pptrace(@"Download from '%@' - '%@' done.\n", ListName(list), [[download request] URL]);
    if (list) {
        NSString *myPath = [context objectForKey:@"path"];
        NSFileManager *fm = [NSFileManager defaultManager];
        if (!myPath || ![fm fileExistsAtPath:myPath]) {
            [self download:download didFailWithError:
                [NSError errorWithDomain:NSPOSIXErrorDomain code:ENOENT userInfo:nil]];
            return; // didFailWithError peforms the necessary cleanup
        }
        
        NSMutableDictionary *completions = [context objectForKey:@"completed"];
        [completions setObject:myPath forKey:[context objectForKey:@"original URL"]];
        [self decrementDownloadDependendencies:dlkey];
    }
#ifdef PPDBG
    else
        dbgbrk();
#endif
    [downloads removeObjectForKey:dlkey];
}

- (void)download:(NSURLDownload *)download didFailWithError:(NSError *)error
{
    NSValue *dlkey = [NSValue valueWithPointer:(void*)download];
    NSDictionary *context = [downloads objectForKey:dlkey];
    PPList *list = [context objectForKey:@"list"];
    NSString *tempFile = [context objectForKey:@"path"];
    if (!tempFile)
        tempFile = @"";
    NSLog(@"Download from '%@' (URL:'%@', File:'%@') failed with '%@'.\n",
        ListName(list), [[download request] URL], tempFile, error);
    
    NSMutableArray *notUpdated = [context objectForKey:@"not updated"];
    [notUpdated addObject:[context objectForKey:@"original URL"]];
    [self decrementDownloadDependendencies:dlkey];
    [downloads removeObjectForKey:dlkey];
    
    NSDictionary *why = [NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithInt:[error code]], @"code",
        [error localizedDescription], NSLocalizedDescriptionKey,
        nil];
        
    if (list) {
        [self notifyClientsUsingSelector:@selector(listFailedUpdate:error:) object:list object2:why];
    }
#ifdef PPDBG
    else
        dbgbrk();
#endif
}

@end

@implementation NSApplication (PPLoaderAppleScriptExtensions)

- (id)updateLists:(NSScriptCommand*)command
{
    [[self delegate] performSelector:@selector(updateAllLists) withObject:nil afterDelay:0.0];
    return (nil);
}

@end

static void PGSCDynamicStoreCallBack(SCDynamicStoreRef scRef, CFArrayRef keys, void *context)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    static NSMutableArray *dnsCache = nil;
    static NSMutableArray *interfaceCache = nil;
    
    @try {
    NSDictionary *vals = (NSDictionary*)SCDynamicStoreCopyMultiple(scRef, keys, NULL);
    if (!vals)
        goto exit;
    
    NSEnumerator *en = [(NSArray*)keys objectEnumerator];
    NSString *key;
    BOOL commit = NO;
    NSMutableArray *source = nil;
    NSDictionary *entry;
    while ((key = [en nextObject])) {
        NSDictionary *info = [vals objectForKey:key];
        NSArray *addrs;
        NSString *name;
        if (NSNotFound != [key rangeOfString:(NSString*)kSCEntNetDNS].location) {
            if (!dnsCache)
                dnsCache = [[NSMutableArray alloc] init];
            source = dnsCache;
            
            addrs = [info objectForKey:(NSString*)kSCPropNetDNSServerAddresses];
            if (!(name = [info objectForKey:(NSString*)kSCPropNetDNSDomainName]))
                name = NSLocalizedString(@"DNS Server", "");
        } else if (NSNotFound != [key rangeOfString:(NSString*)kSCEntNetIPv4].location) {
            // We have to copy the service and then the service values to get the info we want
            NSString *service = [info objectForKey:(NSString*)kSCDynamicStorePropNetPrimaryService];
            if (service) {
                CFStringRef skey = SCDynamicStoreKeyCreateNetworkServiceEntity(kCFAllocatorDefault,
                    kSCDynamicStoreDomainState, (CFStringRef)service, kSCEntNetIPv4);
                if (key) {
                    NSDictionary *svals = (NSDictionary*)SCDynamicStoreCopyValue(scRef, skey);
                    if (svals) {
                        if (!interfaceCache)
                            interfaceCache = [[NSMutableArray alloc] init];
                        source = interfaceCache;
                        
                        addrs = [svals objectForKey:(NSString*)kSCPropNetIPv4Addresses];
                        NSString *router = [svals objectForKey:(NSString*)kSCPropNetIPv4Router];
                        if (router && addrs) {
                            NSMutableArray *tmpAddrs = [addrs mutableCopy];
                            [tmpAddrs addObject:router];
                            addrs = (NSArray*)tmpAddrs;
                        } else if (router && !addrs)
                            addrs = [NSArray arrayWithObject:router];
                        
                        NSArray *broadcastAddrs = [svals objectForKey:(NSString*)kSCPropNetIPv4BroadcastAddresses];
                        if (broadcastAddrs) {
                            addrs = [addrs arrayByAddingObjectsFromArray:broadcastAddrs];
                        }
                        
                        if (!(name = [svals objectForKey:(NSString*)kSCPropInterfaceName]))
                            name = NSLocalizedString(@"Local Interface", "");
                        [svals autorelease];
                    }
                    CFRelease(skey);
                }
            } else
                continue;
        } else
            continue;
        
        if (addrs && [addrs count] > 0) {
            commit = YES;
            [source removeAllObjects];
            unsigned i, count = [addrs count];
            for (i = 0; i < count; ++i) {
                entry = [[NSDictionary alloc] initWithObjectsAndKeys:
                    [addrs objectAtIndex:i], PP_ADDR_SPEC_START,
                    [addrs objectAtIndex:i], PP_ADDR_SPEC_END,
                    name, PP_ADDR_SPEC_NAME, nil];
                [source addObject:entry];
                [entry release];
            }
        }
    } // while (key)
    PPList *allowList = [(id)context listWithID:PG_TABLE_ID_ALLW_LOCAL];
    if (allowList && !ListActive(allowList))
        commit = NO;
    if (commit /*will only be true if a source changed*/) {
        // Add the entries from the caches
        NSMutableArray* sources[] = {dnsCache, interfaceCache, nil};
        BOOL clear = YES;
        int i = 0;
        for (source = sources[0]; source; ++i, source = sources[i]) {
            en = [source objectEnumerator];
            while ((entry = [en nextObject])) {
                if (0 != [(id)context addAllowedLocalAddress:entry clearCurrentEntries:clear load:NO]) {
                    /* likely a ip6 address */
                    NSLog(@"%s: Invalid addr spec: %@\n", __FUNCTION__, entry);
                }
                clear = NO;
            }
        }
        
        if ((allowList = [(id)context listWithID:PG_TABLE_ID_ALLW_LOCAL]))
            [(id)context listChanged:allowList watch:NO];
        else
            NSLog(@"%s: Failed to create local allow list!\n", __FUNCTION__, entry);
    } // allowList && commit
    [vals release];
    } @catch(NSException *e) {
        NSLog(@"%s: -Exception- %@\n", __FUNCTION__, e);
    }
    
exit:
    [pool release];
}

int main (int argc, const char *argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    // This breaks AuthorizationExecuteWithPrivileges for some reason; even just loading an "(allow default)" rule.
    #ifdef breaks_AuthServices
    pgsbinit();
    #endif
    
    PPLoader *load = [[PPLoader alloc] init];
    [[NSApplication sharedApplication] setDelegate:load];
    
    [pool release];
    
    return NSApplicationMain(argc, argv);
}
