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
#import <sys/types.h>
#import <sys/stat.h>
#import <fcntl.h>
#import <unistd.h>
#import <netdb.h>
#import <sys/sysctl.h>
#import <pthread.h>
#import <mach/mach_init.h>

#import <Growl/Growl.h>

#define PG_MAKE_TABLE_ID
#import "pplib.h"
#import "pgsb.h"
#import "pploader.h"
#import "pghistory.h"

// Returns a NSBundle or NSString object (or nil)
static id pp_path_for_pid(int32_t pid, BOOL *isURL);

static int logfd = -1;
static pp_log_handle_t logh = NULL;
static BOOL logallowed = YES;
static unsigned logCycleSize = (10 << 10 << 10);
static unsigned logCycleMax = (128 << 10 << 10);

#define PP_GROWL_BLOCK      @"Blocked Address"
#define PP_GROWL_TBL_CHANGE @"List Update"
#define PP_GROWL_BLOCK_PRI 0
#define PP_GROWL_BLOCK_HTTP_PRI 1
#define PP_GROWL_LIST_PRI -1

static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", 0};
static const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", 0};
static int argmax = 0;

static NSMutableDictionary *portNames = nil;
static NSString *growlLabelTime = nil;
static NSString *growlLabelFrom = nil;
static NSString *growlLabelTo = nil;
static NSString *growlLabelProtocol = nil;
static NSString *growlLabelApplication = nil;
static NSString *growlLabelRange = nil;
static NSString *growlBlockTitle = nil;
static NSString *ppFilterStallLabel = nil;
static NSString *ppDynTableLabel = nil;
static NSString* const ppRepeatEvent = @"last event repeated %u times";
static NSString *ppPortFilterName = nil;

static NSData *blockIconData = nil;
static NSSet *growlIgnore = nil;
static int dumpstats = 0;

@interface PPLogger : NSObject <GrowlApplicationBridgeDelegate> {
}

- (void)cycleLog:(id)obj;
- (void)startLog:(NSNotification*)note;
- (void)serviceGrowlQueue:(id)obj;

@end

static PPLogger *pplog = nil;
static NSConditionLock *growlLock = nil;
static NSMutableArray *growlQueue = nil;
#define PP_NO_GROWL_MSG 0
#define PP_NEW_GROWL_MSG 1

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

static inline
unsigned int ExchangeAtomic(register unsigned int newVal, unsigned int volatile *addr)
{
    register unsigned int val;
    do {
        val = *addr;
    } while (val != newVal && FALSE == CompareAndSwap(val, newVal, (UInt32*)addr));
    return (val);
}

static
NSString * pid_name(int32_t pid)
{
    NSString *app = nil;
    if (pid > 0) {
        id path;
        BOOL isURL;
        path = pp_path_for_pid(pid, &isURL);
        if (path) {
            CFStringRef displayName;
            if (isURL && 0 == LSCopyDisplayNameForURL((CFURLRef)path, &displayName)) {
                app = [(NSString*)displayName autorelease];
            } else {
                if (isURL)
                    path = [(NSURL*)path absoluteString];
                app = [[path lastPathComponent] stringByDeletingPathExtension];
            }
        }
    } else if (0 == pid)
        app = @"kernel";
    
    if (nil == app)
        app = @"unkw";
    
    return (app);
}

static unsigned int histFlush __attribute__ ((aligned (4))) = 0;
static int histFD __attribute__ ((aligned (4))) = -1;

static void hist_write_entry(const pg_hist_ent_t *ent, int len, int now)
{
    static NSMutableData *cache = nil;
    static pthread_mutex_t lck = PTHREAD_MUTEX_INITIALIZER;
    
    pthread_mutex_lock(&lck);
    
    if (!cache) {
        if (!ent) {
            pthread_mutex_unlock(&lck);
            return;
        }
        cache = [[NSMutableData alloc] initWithCapacity:PG_HIST_BLK_SIZE];
    }
    
    if (ent && len)
        [cache appendBytes:ent length:len];
    
    id cwrite = nil;
    if (histFD > -1 && (now /*|| [cache length] >= PG_HIST_BLK_SIZE*/)) {
        struct timeval now;
        gettimeofday(&now, NULL);
        (void)ExchangeAtomic(now.tv_sec, &histFlush);
        
        cwrite = cache;
        cache = nil;
        (void)write(histFD, [cwrite bytes], [cwrite length]);
    }
    
    pthread_mutex_unlock(&lck);
    
    if (cwrite) {
        [cwrite release];
        (void)fsync(histFD);
        [[NSDistributedNotificationCenter defaultCenter]
            postNotificationName:@PG_HIST_FILE_DID_UPDATE object:nil userInfo:nil
            #ifdef notyet
            options:NSNotificationPostToAllSessions
            #endif
            ];
    }
}

#define HIST_FLUSH 1
static void* hist_cache_flush(void *arg)
{
    // open the hist file and become the flush thread
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    const char *path = [[@PG_HIST_FILE stringByExpandingTildeInPath] fileSystemRepresentation];
    int fd = pg_hist_open(path, 0, (512 * 1024 * 1024));
    if (-1 == fd) {
        NSLog(@"failed to open history file: %d\n", errno);
        [pool release];
        return (NULL);
    }
    [pool release];
    (void)ExchangeAtomic(fd, (void*)&histFD);
    
    do {
        sleep(HIST_FLUSH);
        
        struct timeval now;
        gettimeofday(&now, NULL);
        
        unsigned int last = histFlush;
        if ((now.tv_sec - HIST_FLUSH) > last) {
            NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
            hist_write_entry(NULL, 0, 1);
            [pool release];
        }
    } while(1);
    return (NULL);
}

static void hist_create_entry(const pp_log_entry_t *ep, pg_hist_ent_t *hent)
{
    hent->ph_timestamp = ep->pplg_timestamp;
    
    u_int32_t flags = ep->pplg_flags;
    hent->ph_emagic = PG_HIST_ENT_MAGIC;
    hent->ph_flags = 0;
    if (flags & PP_LOG_RMT_TO) {
        hent->ph_port = ntohs(ep->pplg_fromport);
        hent->ph_rport = ntohs(ep->pplg_toport);
    } else {
        hent->ph_port = ntohs(ep->pplg_toport);
        hent->ph_rport = ntohs(ep->pplg_fromport);
        hent->ph_flags |= PG_HEF_RCV;
    }
    
    if (flags & PP_LOG_BLOCKED)
        hent->ph_flags |= PG_HEF_BLK;
    
    if (flags & PP_LOG_TCP)
        hent->ph_flags |= PG_HEF_TCP;
    else if (flags & PP_LOG_UDP)
        hent->ph_flags |= PG_HEF_UDP;
    else if (flags & PP_LOG_ICMP)
        hent->ph_flags |= PG_HEF_ICMP;
    
    if (flags & PP_LOG_IP6)
        hent->ph_flags |= PG_HEF_IPv6;
}

#define PP_REPEAT_INTERVAL 5
#define LOG_STAT_DUMP_INTERVAL 30
static
void newlogentry(const pp_log_entry_t *ep, const char *entryName, const char *tableName)
{
    static pp_log_entry_t lastEvent = {0};
    static unsigned int lastRepeatTime __attribute__ ((aligned (4))) = 0xffffffff - PP_REPEAT_INTERVAL;
    static unsigned int eventRepeatCount __attribute__ ((aligned (4))) = 0;
    static unsigned int lastStatDump __attribute__ ((aligned (4))) = 0;
    
    // create history entry
    pg_hist_ent_t hent;
    hist_create_entry(ep, &hent);
    hist_write_entry(&hent, sizeof(hent), 0);
    
    struct pp_log_stats logstats;
    time_t secs = (time_t)(ep->pplg_timestamp / 1000000ULL);
    
    unsigned int lastVal = lastRepeatTime + PP_REPEAT_INTERVAL;
    if (lastVal <= secs || 0 == pp_log_entry_cmp(&lastEvent, ep)) {
        (void)ExchangeAtomic(0xffffffff - PP_REPEAT_INTERVAL, &lastRepeatTime);
        // XXX - This should be done under a lock,
        // but we know that pplib will only call us from a single thread
        bcopy(ep, &lastEvent, sizeof(*ep));
    } else {
        (void)ExchangeAtomic((unsigned int)secs, &lastRepeatTime);
        (void)IncrementAtomic((SInt32*)&eventRepeatCount);
        return;
    }
    
    struct timeval now;
    gettimeofday(&now, NULL);
    
    int msecs = (int)((ep->pplg_timestamp % 1000000ULL) / 1000);
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    if ((lastVal = ExchangeAtomic(0, &eventRepeatCount))) {
        const char *repeatMsg = [[[NSString stringWithFormat:ppRepeatEvent, lastVal]
            stringByAppendingString:@"\n"] UTF8String];
        // XXX - again, following should be under a lock
        (void)write(logfd, repeatMsg, strlen(repeatMsg));
    }
    
    u_int32_t flags = ep->pplg_flags;
    
    char *type;
    if (flags & PP_LOG_ALLOWED) {
        if (!logallowed)
            return;
        type = "Allw"; // XXX - [PeerProtector fileDataAvailable:] relies on this string
    } else if (flags & PP_LOG_BLOCKED)
        type = "Blck"; // XXX - ditto
    else
        type = "unkw";
        
    char *protocol;
    if (flags & PP_LOG_TCP)
        protocol = 0 == (flags & PP_LOG_IP6) ? "tcp4" : "tcp6";
    else if (flags & PP_LOG_UDP)
        protocol = 0 == (flags & PP_LOG_IP6) ? "udp4" : "udp6";
    else if (flags & PP_LOG_ICMP)
        protocol = 0 == (flags & PP_LOG_IP6) ? "icmp4" : "icmp6";
    else
        protocol = "unkw";
    
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
    char timestr[36];
    snprintf(timestr, sizeof(timestr)-1, "%s %s %2d %d %02d:%02d:%02d.%-3d %s", days[tm.tm_wday],
        months[tm.tm_mon], tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
        msecs, tm.tm_zone);
    timestr[sizeof(timestr)-1] = 0;
    unsigned char tzoff = strlen(timestr) - strlen(tm.tm_zone) - 1;
    
    char nameStr[512];
    if (0 == pg_table_id_cmp(PG_TABLE_ID_NULL, ep->pplg_tableid)) {
        // Could have been an error getting the tableName, but we'll just assume this
        // was not in any table (i.e. most allows).
        nameStr[0] = 0;
    } else if (flags & PP_LOG_DYN) {
        strcpy(nameStr, [ppDynTableLabel UTF8String]);
    } else if (flags & PP_LOG_FLTR_STALL) {
        strcpy(nameStr, [ppFilterStallLabel UTF8String]);
    } else {
        if (tableName) {
            // entryName may be NULL, but tableName will be valid
            if (entryName)
                snprintf(nameStr, sizeof(nameStr)-1, "%s:%s", entryName, tableName);
            else
                snprintf(nameStr, sizeof(nameStr)-1, "%u:%s", ep->pplg_name_idx, tableName);
        } else {
            char namebuf[64];
            snprintf(nameStr, sizeof(nameStr)-1, "%s:%d",
                pg_table_id_string(ep->pplg_tableid, namebuf, sizeof(namebuf), 1),
                ep->pplg_name_idx);
        }
        nameStr[sizeof(nameStr)-1] = 0;
    }
    
    NSString *toPortName, *fromPortName;
    typeof(ep->pplg_toport) toPort = ntohs(ep->pplg_toport);
    typeof(ep->pplg_fromport) fromPort = ntohs(ep->pplg_fromport);
    @synchronized(portNames) {
        NSNumber *key = [NSNumber numberWithUnsignedShort:toPort];
        toPortName = [portNames objectForKey:key];
        struct servent *sent = NULL;
        if (!toPortName && (toPort > 0 && toPort <= 1024)) {
            sent = getservbyport(ep->pplg_toport, NULL); // port must be in network order
            if (sent) {
                toPortName = [NSString stringWithCString:sent->s_name encoding:NSASCIIStringEncoding];
                [portNames setObject:toPortName forKey:key];
            }
            endservent();
        }
        if (toPortName)
            toPortName = [NSString stringWithFormat:@"%u (%@)", toPort, toPortName];
        else
            toPortName = [NSString stringWithFormat:@"%u", toPort];
        
        key = [NSNumber numberWithUnsignedShort:fromPort];
        fromPortName = [portNames objectForKey:key];
        if (!fromPortName && (fromPort > 0 && fromPort <= 1024)) {
            sent = getservbyport(ep->pplg_fromport, NULL); // port must be in network order
            if (sent) {
                fromPortName = [NSString stringWithCString:sent->s_name encoding:NSASCIIStringEncoding];
                [portNames setObject:fromPortName forKey:key];
            }
            endservent();
        }
        if (fromPortName)
            fromPortName = [NSString stringWithFormat:@"%u (%@)", fromPort, fromPortName];
        else
            fromPortName = [NSString stringWithFormat:@"%u", fromPort];
    }
    
    NSString *app = pid_name(ep->pplg_pid);
    NSString *appAndPID;
    if (app)
        appAndPID = [NSString stringWithFormat:@"%@ (%d)", app, ep->pplg_pid];
    else
        appAndPID = app = [NSString stringWithFormat:@"(%d)", ep->pplg_pid]; // Process has disappeared already
    
    char buf[1536];
    // XXX - [PeerProtector fileDataAvailable:] relies on "-type-"
    int off = snprintf(buf, sizeof(buf)-1, "%s -%s- %s:%s -> %s:%s %s '%s'", timestr, type, fromstr,
        [fromPortName UTF8String], tostr, [toPortName UTF8String], protocol, [appAndPID UTF8String]);
    if (strlen((const char*)nameStr))
        snprintf(&buf[off], sizeof(buf)-1-off, " (%s)\n", nameStr);
    else
        snprintf(&buf[off], sizeof(buf)-1-off, "\n");
    buf[sizeof(buf)-1] = 0;
    (void)write(logfd, buf, strlen(buf));
    
    unsigned growlEntries = 0;
    if ((flags & PP_LOG_BLOCKED) && NO == [growlIgnore containsObject:app]) {
        timestr[tzoff] = 0; // We don't need to display the TZ for the user
        NSString *msg = [NSString stringWithFormat:@"%@: %s\n%@: %s:%@\n%@: %s:%@\n%@: %s\n%@: %@\n%@: %s",
            growlLabelTime, timestr,
            growlLabelFrom, fromstr, fromPortName, 
            growlLabelTo, tostr, toPortName,
            growlLabelProtocol, protocol,
            growlLabelApplication, appAndPID,
            growlLabelRange, nameStr];
        
        const char *remote = (char*)(flags & PP_LOG_RMT_FRM ? fromstr : tostr);
        NSDictionary *addrSpec = [NSDictionary dictionaryWithObjectsAndKeys:
            [NSString stringWithCString:remote encoding:NSASCIIStringEncoding], PP_ADDR_SPEC_START,
            [NSString stringWithCString:(char*)nameStr encoding:NSUTF8StringEncoding], PP_ADDR_SPEC_NAME,
            nil];
        
        int priority = PP_GROWL_BLOCK_PRI;
        if (flags & PP_LOG_RMT_TO) {
            if (80 == toPort || 443 == toPort)
                priority = PP_GROWL_BLOCK_HTTP_PRI;
        }
        NSDictionary *args = [NSDictionary dictionaryWithObjectsAndKeys:
            msg, @"msg",
            addrSpec, @"addrSpec",
            growlBlockTitle, @"title",
            blockIconData, @"icon",
            [NSNumber numberWithInt:priority], @"priority",
            PP_GROWL_BLOCK, @"note name",
            nil];
        [growlLock lock];
        growlEntries = [growlQueue count];
        [growlQueue addObject:args];
        [growlLock unlockWithCondition:PP_NEW_GROWL_MSG];
    }
    
    if (dumpstats && (lastStatDump + LOG_STAT_DUMP_INTERVAL) <= now.tv_sec ) {
        if (0 == pp_log_stats(logh, &logstats)) {
            (void)ExchangeAtomic((unsigned int)now.tv_sec, &lastStatDump);
            NSLog(@"-log stats- queued: %u, received: %u, processed: %u, growl q: %u",
                logstats.pps_entriesQueued, logstats.pps_entriesReceived, logstats.pps_entriesProcessed, growlEntries);
        }
    }
    
    struct stat sb;
    if (0 == fstat(logfd, &sb)) {
        if (sb.st_size >= (typeof(sb.st_size))logCycleMax) {
            int fd = logfd;
            @synchronized(pplog) {
                logfd = -1;
            }
            close(fd);
            [pplog performSelectorOnMainThread:@selector(cycleLog:) withObject:nil waitUntilDone:YES];
            [pplog startLog:nil];
        }
    }
    
    [pool release];
}

static
void table_event(const pp_log_entry_t *ep, const char *entryName __unused, const char *tableName)
{
    time_t secs = (time_t)(ep->pplg_timestamp / 1000000ULL);
    int msecs = (int)((ep->pplg_timestamp % 1000000ULL) / 1000);
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    struct tm tm;
    (void)localtime_r(&secs, &tm);
    char timestr[36];
    snprintf(timestr, sizeof(timestr)-1, "%s %s %2d %d %02d:%02d:%02d.%-3d", days[tm.tm_wday],
        months[tm.tm_mon], tm.tm_mday, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
        msecs);
    timestr[sizeof(timestr)-1] = 0;
    
    NSString *tableMoniker;
    if (tableName)
        tableMoniker = [NSString stringWithUTF8String:tableName];
    else {
        if (0 == pg_table_id_cmp(PG_TABLE_ID_PORTS, ep->pplg_tableid))
            tableMoniker = ppPortFilterName ;
        else
            tableMoniker = NSLocalizedString(@"Unknown", "");
    }
    ppassert(tableMoniker != nil);
    
    NSString *msg = [NSString stringWithFormat:@"%@: %s\n%@: %@\n%@: %@",
        growlLabelTime, timestr,
        growlLabelApplication, pid_name(ep->pplg_pid),
        NSLocalizedString(@"List", ""), tableMoniker];
    
    NSString *title;
    int flags = ep->pplg_flags;
    if (flags & PP_LOG_TBL_LD)
        title = NSLocalizedString(@"Peer Guardian List Load", "");
    else if (flags & PP_LOG_TBL_ULD)
        title = NSLocalizedString(@"Peer Guardian List Unload", "");
    else // if (PP_LOG_TBL_RLD)
        title = NSLocalizedString(@"Peer Guardian List Reload", "");
    
    NSDictionary *args = [NSDictionary dictionaryWithObjectsAndKeys:
        msg, @"msg",
        title, @"title",
        PP_GROWL_TBL_CHANGE, @"note name",
        [NSNumber numberWithInt:PP_GROWL_LIST_PRI], @"priority",
        nil];
    [growlLock lock];
    [growlQueue addObject:args];
    [growlLock unlockWithCondition:PP_NEW_GROWL_MSG];
    
    [pool release];
}

@implementation PPLogger

-(void)growlNotificationWasClicked:(CFPropertyListRef)clickContext
{
    if (clickContext) {
        // Let pploader handle the user interaction
        [[NSDistributedNotificationCenter defaultCenter]
            postNotificationName:PP_ADDR_ACTION_ASK
            object:nil userInfo:(NSDictionary*)clickContext];
    }
}

- (void)serviceGrowlQueue:(id)obj
{
    do {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        
        NSMutableArray *queue;
        NSMutableArray *newGrowlQueue = [[NSMutableArray alloc] init];
        
        [growlLock lockWhenCondition:PP_NEW_GROWL_MSG];
        queue = growlQueue;
        growlQueue = newGrowlQueue;
        [growlLock unlockWithCondition:PP_NO_GROWL_MSG];
        newGrowlQueue = nil;
        
        NSEnumerator *en = [queue objectEnumerator];
        NSDictionary *obj;
        
        while((obj = [en nextObject])) {
            // Growl can become extremely backlogged if we send too fast,
            // so check for lag and throttle if necessary.
            struct timeval begin, end;
            gettimeofday(&begin, NULL);
            
            int priority = [obj objectForKey:@"priority"] ? [[obj objectForKey:@"priority"] intValue] : 0;
            [GrowlApplicationBridge
                notifyWithTitle:[obj objectForKey:@"title"]
                description:[obj objectForKey:@"msg"]
                notificationName:[obj objectForKey:@"note name"]
                iconData:[obj objectForKey:@"icon"]
                priority:priority
                isSticky:NO
                clickContext:[obj objectForKey:@"addrSpec"]];
                        
            gettimeofday(&end, NULL);
            unsigned int secs;
            if ((secs = (unsigned int)(end.tv_sec - begin.tv_sec)) >= 1)
                sleep(secs << 1);
            else {
                // rate limit to a max of 5 msgs/sec
                static const struct timespec ts = {0,200000000}; 
                nanosleep(&ts, NULL);
            }
        }
        
        [queue release];
        
        [pool release];
    } while(1);
}

- (void)taskDidTerminate:(NSNotification*)note
{
    [[NSNotificationCenter defaultCenter] removeObserver:self
        name:NSTaskDidTerminateNotification object:[note object]];
    [[note object] autorelease];
}

- (void)cycleLog:(id)obj
{
    NSFileManager *fm = [NSFileManager defaultManager];
    NSString *path = PP_LOGFILE;
    if ([fm fileExistsAtPath:path]
        && [[fm fileAttributesAtPath:path traverseLink:YES] fileSize] >= (u_int64_t)logCycleSize) {
        NSString *ext = [PP_LOGFILE pathExtension];
        NSString *newname = [[[PP_LOGFILE stringByDeletingLastPathComponent] stringByAppendingPathComponent:
            [NSString stringWithFormat:@"%@_%@", [[PP_LOGFILE lastPathComponent] stringByDeletingPathExtension],
                [[NSCalendarDate date] descriptionWithCalendarFormat:@"%Y%m%d%H%M%S"]]]
            stringByAppendingPathExtension:ext];
        (void)[fm movePath:PP_LOGFILE toPath:newname handler:nil];
        if ([fm fileExistsAtPath:newname]) {
            NSTask *t = [[NSTask alloc] init];
            [t setLaunchPath:@"/usr/bin/nice"];
            [t setArguments:[NSArray arrayWithObjects:@"-15",
                @"/usr/bin/bzip2", @"-9", [newname lastPathComponent], nil]];
            [t setCurrentDirectoryPath:[newname stringByDeletingLastPathComponent]];
            [t setStandardOutput:[NSFileHandle fileHandleWithNullDevice]];
            [[NSNotificationCenter defaultCenter] addObserver:self 
                selector:@selector(taskDidTerminate:) 
                name:NSTaskDidTerminateNotification object:t];
            @try {
                [t launch];
            } @catch (NSException *exception) {
                [[NSNotificationCenter defaultCenter] removeObserver:self
                    name:NSTaskDidTerminateNotification object:t];
                [t release];
            }
        }
    }
}

- (void)handleQuit:(NSNotification*)note
{
    [NSApp terminate:nil];
}

- (void)startLog:(NSNotification*)note
{
    pptrace(@"startLog:");
    int err = 0;
    @synchronized(self) {
        if (-1 == logfd) {
            logfd = open([PP_LOGFILE fileSystemRepresentation], O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
            if (-1 != logfd) {
                fcntl(logfd, F_NOCACHE, 1);
                lseek(logfd, 0LL, SEEK_END);
                [[NSDistributedNotificationCenter defaultCenter]
                    postNotificationName:PP_LOGFILE_CREATED object:nil];
            } else
                NSLog(@"Failed to open log file with: %d\n", errno);
        }
        if (!logh)
            err = pp_log_listen(1, newlogentry, table_event, &logh);
    }
    if (err)
        NSLog(@"Failed to open log handle with: %d\n", err);
}

- (void)stopLog:(NSNotification*)note
{
    pptrace(@"stopLog:");
    int err = 0;
    @synchronized(self) {
        if (logh) {
            if (0 == (err = pp_log_stop(logh)))
                logh = NULL;
        }
    }
    if (err)
        NSLog(@"Failed to close log handle with: %d\n", err);
}

- (void)prefChanged:(NSNotification*)note
{
    NSDictionary *d = [note userInfo];
    if (d) {
        NSString *key = [d objectForKey:PP_PREF_KEY];
        if ([key isEqualToString:@"Log Allowed"])
            logallowed = [[d objectForKey:PP_PREF_VALUE] boolValue];
    #ifdef notyet // needs lock protection
        if ([key isEqualToString:@"GrowlIgnore"]) {
            [growlIgnore release]
            growlIgnore = [[NSSet alloc] initWithArray:val];
        }
    #endif
    }
}

- (NSDictionary *)registrationDictionaryForGrowl
{
    NSArray *notifications = [NSArray arrayWithObjects:PP_GROWL_BLOCK, PP_GROWL_TBL_CHANGE, nil];
    return ( [NSDictionary dictionaryWithObjectsAndKeys:
        @"Peer Guardian", GROWL_APP_NAME,
        [NSNumber numberWithInt:1], GROWL_NOTIFICATION_PRIORITY,
        notifications, GROWL_NOTIFICATIONS_ALL,
        notifications, GROWL_NOTIFICATIONS_DEFAULT,
        [[NSImage imageNamed:@"app.icns"] TIFFRepresentation], GROWL_APP_ICON,
        nil] );
}

- (NSImage*)blockIcon
{
    NSImage *app = [[NSImage imageNamed:@"app.icns"] copy];
    NSImage *badge = [[NSImage imageNamed:@"disabled.tif"] copy];
    [app setScalesWhenResized:YES];
    [badge setScalesWhenResized:YES];
    
    NSImage *icon = [[NSImage alloc] initWithSize:NSMakeSize(128, 128)];
    NSPoint p;
    [icon lockFocus];
    
    p.x = p.y = 0.0;
    [app compositeToPoint:p operation:NSCompositeSourceOver];
    [badge compositeToPoint:p operation:NSCompositeSourceOver];
    
    [icon unlockFocus];
    
    [badge release];
    [app release];
    
    return ([icon autorelease]);
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification
{    
    portNames = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
        @"kzaa", [NSNumber numberWithInt:1214],
        @"msnm", [NSNumber numberWithInt:1863],
        @"slsk", [NSNumber numberWithInt:2234],
        @"edky", [NSNumber numberWithInt:4661], // server
        @"edky", [NSNumber numberWithInt:4662], // p2p
        @"aim ", [NSNumber numberWithInt:5190],
        @"slsk", [NSNumber numberWithInt:5534],
        @"mdns", [NSNumber numberWithInt:5353],
        @"gnut", [NSNumber numberWithInt:6346],
        @"gnut", [NSNumber numberWithInt:6347],
        @"torr", [NSNumber numberWithInt:6881],
        @"torr", [NSNumber numberWithInt:6882],
        @"torr", [NSNumber numberWithInt:6883],
        @"torr", [NSNumber numberWithInt:6884],
        @"torr", [NSNumber numberWithInt:6885],
        @"torr", [NSNumber numberWithInt:6886],
        @"torr", [NSNumber numberWithInt:6887],
        @"torr", [NSNumber numberWithInt:6888],
        @"torr", [NSNumber numberWithInt:6889],
        @"http", [NSNumber numberWithInt:8080],
        nil];
    
    growlLabelTime = [NSLocalizedString(@"Time", "") retain];
    growlLabelFrom = [NSLocalizedString(@"From", "") retain];
    growlLabelTo = [NSLocalizedString(@"To", "") retain];
    growlLabelProtocol = [NSLocalizedString(@"Protocol", "") retain];
    growlLabelApplication = [NSLocalizedString(@"Application", "") retain];
    growlLabelRange = [NSLocalizedString(@"Range", "") retain];
    growlBlockTitle = [NSLocalizedString(@"Peer Guardian Block", "") retain];
    
    ppFilterStallLabel = [NSLocalizedString(@"PP Filter Stall", "") retain];
    ppDynTableLabel = [NSLocalizedString(@"PP Dynamic List", "") retain];
    // Not sure about localizing this msg - on the one hand it's good to support multiple
    // languages, but on the other, it makes for parsing a log file with a script almost
    // impossible. So, for now, no localization.
    //ppRepeatEvent = [NSLocalizedString(@"last event repeated %u times", "") retain];
    
    ppPortFilterName = NSLocalizedString(@"Port Filter", "");
    
    blockIconData = [[[self blockIcon] TIFFRepresentation] retain];
    
    // register for events
    NSDistributedNotificationCenter *nc = [NSDistributedNotificationCenter defaultCenter];
    [nc addObserver:self selector:@selector(startLog:) name:PP_KEXT_DID_LOAD object:nil];
    [nc addObserver:self selector:@selector(stopLog:) name:PP_KEXT_WILL_UNLOAD object:nil];
    [nc addObserver:self selector:@selector(startLog:) name:PP_KEXT_FAILED_UNLOAD object:nil];
    [nc addObserver:self selector:@selector(prefChanged:) name:PP_PREF_CHANGED object:nil];
    [nc addObserver:self selector:@selector(handleQuit:) name:PP_HELPER_QUIT object:nil];
    
    // Add in the pploader suite to get the initial values
    [[NSUserDefaults standardUserDefaults] addSuiteNamed:@"xxx.qnation.pploader"];
    id val = [[NSUserDefaults standardUserDefaults] objectForKey:@"Log Allowed"];
    if (val && [val isKindOfClass:[NSNumber class]])
    	logallowed = [val boolValue];
    val = [[NSUserDefaults standardUserDefaults] objectForKey:@"LogCycleSize"];
    if (val && [val isKindOfClass:[NSNumber class]] && [val unsignedIntValue] >= 0x100000)
        logCycleSize = [val unsignedIntValue];
    val = [[NSUserDefaults standardUserDefaults] objectForKey:@"LogCycleMax"];
    if (val && [val isKindOfClass:[NSNumber class]] && [val unsignedIntValue] >= 0x1000000)
        logCycleMax = [val unsignedIntValue];
    val = [[NSUserDefaults standardUserDefaults] objectForKey:@"GrowlIgnore"];
    if (val && [val isKindOfClass:[NSArray class]])
        growlIgnore = [[NSSet alloc] initWithArray:val];
    else
        growlIgnore = [[NSSet alloc] init];
    [[NSUserDefaults standardUserDefaults] removeSuiteNamed:@"xxx.qnation.pploader"];
    
    [NSThread detachNewThreadSelector:@selector(serviceGrowlQueue:) toTarget:self withObject:nil];
    
    [GrowlApplicationBridge setGrowlDelegate:self];
    
    pthread_t th;
    (void)pthread_create(&th, NULL, hist_cache_flush, NULL);
    (void)pthread_detach(th);
    
    [self cycleLog:nil];
    [self startLog:nil];
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
    [self stopLog:nil];
    hist_write_entry(NULL, 0, 1);
    close(histFD);
    (void)ExchangeAtomic((unsigned)-1, (void*)&histFD);
    
    if (logfd > -1) {
        close(logfd);
        logfd = -1;
    }
    signal(SIGUSR1, SIG_IGN);
}

@end

static void handle_sig(int sigraised)
{
    if (SIGUSR1 == sigraised) {
        dumpstats = !dumpstats ? 1 : 0;
    }
}

int main (int argc, char *argv[])
{
    pgsbinit();
    
    int mib[2] = {CTL_KERN, KERN_ARGMAX};
    size_t arglen = sizeof(argmax);
    if ((0 != sysctl(mib, 2, &argmax, &arglen, NULL, 0)))
        argmax = 1024;
    
    signal(SIGUSR1, handle_sig);
    
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    pplog = [[PPLogger alloc] init];
    growlLock = [[NSConditionLock alloc] initWithCondition:PP_NO_GROWL_MSG];
    growlQueue = [[NSMutableArray alloc] init];

    [[NSRunLoop currentRunLoop] configureAsServer];
    [[NSApplication sharedApplication] setDelegate:pplog];
    
    [pool release];
    
    [NSApp run];
    
    return (0);
}

static NSString* pp_cfm_path(const char *args, size_t arglen)
{
    /*
     * CFM application.  Get the basename of the first argument and
     * use that as the command string.
     */
    
    /* Skip the saved exec_path. */
    const char *cp = (args + sizeof(int));
    for (; cp < &args[arglen]; cp++) {
        if (*cp == '\0') {
            /* End of exec_path reached. */
            break;
        }
    }
    if (cp == &args[arglen]) {
        return (nil);
    }

    /* Skip trailing '\0' characters. */
    for (; cp < &args[arglen]; cp++) {
        if (*cp != '\0') {
            /* Beginning of first argument reached. */
            break;
        }
    }
    if (cp == &args[arglen]) {
        return (nil);
    }
    const char *command_beg = cp;

    /*
     * Make sure that the command is '\0'-terminated.  This protects
     * against malicious programs; under normal operation this never
     * ends up being a problem..
     */
    for (; cp < &args[arglen]; cp++) {
        if (*cp == '\0') {
            /* End of first argument reached. */
            break;
        }
    }
    if (cp == &args[arglen]) {
        return (nil);
    }
    const char *command_end = cp;
    
    int len = command_end - command_beg;
    
    // Leopard includes the binary path as the first args
    if (strnstr(command_beg, "/LaunchCFMApp", len+1))
        return (pp_cfm_path(command_beg, arglen-len));
    
    return ([[[NSString alloc] initWithBytes:command_beg length:len encoding:NSUTF8StringEncoding] autorelease]);
}

static id pp_path_for_pid(int32_t pid, BOOL *isURL)
{
    size_t arglen;
    int mib[4] = {CTL_KERN, 0, 0, 0};
    
    NSString *path = nil;
    mib[1] = KERN_PROCARGS2;
    mib[2] = pid;
    arglen = argmax;
    char *cmdargs = malloc(argmax+1);
    if (cmdargs && 0 == sysctl(mib, 3, cmdargs, &arglen, NULL, 0)) {
        *(cmdargs+(arglen-1)) = '\0'; // Just in case
        // argc is first for KERN_PROCARGS2, so skip the size of an int
        // the command path is next
        if (0 == strstr((cmdargs + sizeof(int)), "/LaunchCFMApp") || nil == (path = pp_cfm_path(cmdargs, arglen)))
            path = [NSString stringWithCString:(cmdargs + sizeof(int)) encoding:NSUTF8StringEncoding];
        *isURL = YES;
    } else if (EINVAL == errno) {
        // sysctl prevents non-root procs from accessing other users proc args
        mib[1] = KERN_PROC;
        mib[2] = KERN_PROC_PID;
        mib[3] = pid;
        struct kinfo_proc p;
        arglen = sizeof(p);
        if (0 == sysctl(mib, 4, &p, &arglen, NULL, 0))
            path = [NSString stringWithCString:p.kp_proc.p_comm encoding:NSUTF8StringEncoding];
        *isURL = NO;
    }
    if (cmdargs)
        free(cmdargs);
    
    return (path ? (*isURL ? [NSURL fileURLWithPath:path] : path) : nil);
}
