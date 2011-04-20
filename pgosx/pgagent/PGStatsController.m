/*
    Copyright 2006,2007 Brian Bergstrand

    This program is free software; you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program;
    if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#import <sys/time.h>
#import <sys/stat.h>
#import <sys/sysctl.h>
#import <fcntl.h>
#import <unistd.h>
#import "PGStatsController.h"
#import <SM2DGraphView/SM2DGraphView.h>

#define PG_MAKE_TABLE_ID
#import "pplib.h"
#import "pghistory.h"
#import "pploader.h"

static pp_session_t sp = NULL;
static int isUnloading = 0;
#define PG_HIST_POINTS 60

enum {
    kGraphRealTime = 0,
    kGraph5m,
    kGraph15m,
    kGraph30m,
    kGraph1h,
    kGraph12h,
    kGraph1d,
    kGraph1w,
    kGraph1mt,
    kGraph1y,
    kGraphAll,
    kGraphTypeCount
};

static const unsigned graphResolution[kGraphTypeCount] = {
60, 300, 900, 1800, 3600, 43200, 86400, 7 * 86400, 30 * 86400, 365 * 86400, 0};

static const unsigned graphUpdateInterval[kGraphTypeCount] = {
1, 60, 60, 60, 60, 120, 180, 300, 300, 600, 600};

static int lastGraphUpdate = 0;

static PGStatsController *shared = nil;

@implementation PGStatsController

+ (PGStatsController*)sharedInstance
{
    return (shared ? shared : (shared = [[PGStatsController alloc] init]));
}

- (void)updateLineData:(NSArray*)args
{
    [lineData release];
    lineData = [[args objectAtIndex:0] retain];
    
    
    NSSize graphSize = [graphView frame].size;
    
    unsigned maxTotal = [[args objectAtIndex:1] unsignedIntValue];
    maxY = (double)maxTotal;
    maxX = PG_HIST_POINTS;
    
    int i = graphSize.height * 0.05;
    [graphView setNumberOfTickMarks:(i < maxTotal ? i : maxTotal) forAxis:kSM2DGraph_Axis_Y_Left];
    i = graphSize.width * 0.025;
    [graphView  setNumberOfTickMarks:i forAxis:kSM2DGraph_Axis_X];
    
    char moniker = 's';
    unsigned seconds = [[args objectAtIndex:2] unsignedIntValue];
    seconds /= PG_HIST_POINTS;
    NSString *tmp;
    if (seconds < 60)
        tmp = [NSString stringWithFormat:@"%u", seconds];
    else {
        seconds /= 60;
        if (seconds < 60) {
            moniker = 'm';
            tmp = [NSString stringWithFormat:@"%u", seconds];
        } else {
            tmp = [NSString stringWithFormat:@"%u.%u", seconds / 60, (unsigned)((((float)(seconds % 60)) / 60) * 10)];
            moniker = 'h';
        }
        
    }
    
    [graphView setLabel:[NSString stringWithFormat:NSLocalizedString(@"Time (%u intervals of %@%c)", ""),
        PG_HIST_POINTS, tmp, moniker] forAxis:kSM2DGraph_Axis_X];
    
    struct timeval now;
    (void)gettimeofday(&now, NULL);
    lastGraphUpdate = now.tv_sec;
    [graphView reloadData];
}

- (void)createLineDataWithHistoryEntries:(NSData*)d resolution:(unsigned)seconds
{
    const pg_hist_ent_t *he = (pg_hist_ent_t*)(([d bytes] + [d length]) - sizeof(*he));
    unsigned ect = [d length] / sizeof(*he);
    
    u_int64_t resolution = (u_int64_t)(seconds / PG_HIST_POINTS);
    resolution *= 1000000ULL; // convert to microseconds
    
    int i = ect;
    for(; i > 0 && PG_HIST_ENT_MAGIC != he->ph_emagic; --he, --i, --ect) ; // XXX dead loop
    
    if (i <= 0)
        return; // bad data all around
    
    if (0 == resolution) {
        resolution = he->ph_timestamp - ((typeof(he))[d bytes])->ph_timestamp;
        resolution -= he->ph_timestamp % 1000000ULL;
        seconds = (typeof(seconds))(resolution / 1000000ULL);
        resolution /= PG_HIST_POINTS;
    }
    
    unsigned total = 0, allow = 0, block = 0, maxTotal = 0; //, maxAllow = 0, maxBlock = 0;
    int resIdx = PG_HIST_POINTS - 1;
    NSMutableData *lineTotal = [NSMutableData dataWithLength:PG_HIST_POINTS * sizeof(NSPoint)];
    NSMutableData *lineAllow = [NSMutableData dataWithLength:PG_HIST_POINTS * sizeof(NSPoint)];
    NSMutableData *lineBlock = [NSMutableData dataWithLength:PG_HIST_POINTS * sizeof(NSPoint)];
    NSPoint *ptTotal = ([lineTotal mutableBytes] + [lineTotal length]) - sizeof(NSPoint);
    NSPoint *ptAllow = ([lineAllow mutableBytes] + [lineAllow length]) - sizeof(NSPoint);
    NSPoint *ptBlock = ([lineBlock mutableBytes] + [lineBlock length]) - sizeof(NSPoint);
    
    int tcp = [[NSUserDefaults standardUserDefaults] integerForKey:@"TCPFilter"];
    int udp = [[NSUserDefaults standardUserDefaults] integerForKey:@"UDPFilter"];
    
    u_int64_t epoch = he->ph_timestamp - (he->ph_timestamp % resolution);
    for (i = 0; i < ect && resIdx >= 0;) {
        if (PG_HIST_ENT_MAGIC == he->ph_emagic) {
            if (he->ph_timestamp < epoch) {
                if (total > maxTotal)
                    maxTotal = total;
                #ifdef notyet
                if (allow > maxAllow)
                    maxAllow = allow;
                if (block > maxBlock)
                    maxBlock = block;
                #endif
                
                ptTotal->y = (float)total;
                ptAllow->y = (float)allow;
                ptBlock->y = (float)block;
                
                total = allow = block = 0;
            }
                
            while (he->ph_timestamp < epoch) {
                ptTotal->x = ptAllow->x = ptBlock->x = (float)resIdx;
                
                --ptTotal;
                --ptAllow;
                --ptBlock;
                --resIdx;
                epoch -= resolution;
            }
            
            if (0 == filteredPort || he->ph_port == filteredPort
                || he->ph_rport == filteredPort) {
                if (0 == ((!tcp && (he->ph_flags & PG_HEF_TCP))
                    || (!udp && (he->ph_flags & PG_HEF_UDP)))) { 
                       ++total;
                    if (0 == (he->ph_flags & PG_HEF_BLK))
                        ++allow;
                    else
                        ++block;
                }
            }
            
            --he;
            ++i;
        }
    }
    if (resIdx >= 0) {
        // Set the tail data
        if (total > maxTotal)
            maxTotal = total;
        ptTotal->y = (float)total;
        ptAllow->y = (float)allow;
        ptBlock->y = (float)block;
        ptTotal->x = ptAllow->x = ptBlock->x = (float)resIdx;
       
        --ptTotal;
        --ptAllow;
        --ptBlock;
        --resIdx;
        for (; resIdx >= 0;) {
            ptTotal->x = ptAllow->x = ptBlock->x = (float)resIdx;
            --ptTotal;
            --ptAllow;
            --ptBlock;
            --resIdx;
        }
    }
    
    NSArray *args = [NSArray arrayWithObjects:
        [NSArray arrayWithObjects:lineTotal, lineAllow, lineBlock, nil],
        [NSNumber numberWithUnsignedInt:maxTotal],
        [NSNumber numberWithUnsignedInt:seconds],
        nil];
    [self performSelectorOnMainThread:@selector(updateLineData:) withObject:args waitUntilDone:NO];
    
    return;
}

- (void)processHistoryThread:(id)arg
{
    do {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        
        [wlock lockWhenCondition:1];
        NSArray *args = [[[work objectAtIndex:0] retain] autorelease];
        [work removeObjectAtIndex:0];
        [wlock unlockWithCondition:0];
        
        #ifdef notyet
        if ([[args objectAtIndex:0] isEqualTo:[NSNull null]]) {
            [NSThread exit];
            return;
        }
        #endif
        
        #ifdef PPDBG
        struct timeval now;
        (void)gettimeofday(&now, NULL);
        #endif
        
        unsigned resolution = [[args objectAtIndex:1] unsignedIntValue];
        u_int64_t off, ts = [[args objectAtIndex:0] unsignedLongLongValue];
        size_t bytes;
        const char *path = [[@PG_HIST_FILE stringByExpandingTildeInPath] fileSystemRepresentation];
        int fd = pg_hist_open(path, 1, 0);
        if (fd > -1) {
            if (0 == pg_hist_find_entry(fd, ts, &off)) {
                struct stat sb;
                (void)fstat(fd, &sb);
                if ((sb.st_size - off) < (u_int64_t)UINT_MAX)
                    bytes = (size_t)(sb.st_size - off);
                else
                    bytes = maxHistData;
                    
                if (bytes > maxHistData) {
                    off = sb.st_size - maxHistData;
                    bytes = maxHistData;
                }
                
                char *buf = bytes > 0 ? malloc(bytes) : NULL;
                if (buf) {
                    if (bytes == pread(fd, buf, bytes, off)) {
                        NSData *d = [NSData dataWithBytesNoCopy:buf length:bytes]; // owns buf now
                        [self createLineDataWithHistoryEntries:d resolution:resolution];
                    } else
                        free(buf);
                }
            }
            
            close(fd);
        }

        #ifdef PPDBG
        struct timeval end, delta;
        (void)gettimeofday(&end, NULL);
        timersub(&end, &now, &delta);
        NSLog(@"hist process time (%u): %u.%u secs\n", graphDataType, delta.tv_sec, delta.tv_usec);
        #endif
        
        [pool release];
    } while(1);
}

- (void)histFileDidUpdate:(NSNotification*)note
{
    struct timeval now;
    (void)gettimeofday(&now, NULL);
    ppassert(graphDataType < kGraphTypeCount);
    if (lastGraphUpdate + graphUpdateInterval[graphDataType] > now.tv_sec) {
        now.tv_usec = 0;
        return;
    }
    
    u_int64_t ts;
    unsigned resolution = graphResolution[graphDataType];
    if (resolution)
        ts = (typeof(ts))(now.tv_sec - resolution) * 1000000ULL;
    else
        ts = 0;
    
    NSArray *args = [NSArray arrayWithObjects:
        [NSNumber numberWithUnsignedLongLong:ts], [NSNumber numberWithUnsignedInt:resolution], nil];
    [wlock lock];
    [work addObject:args];
    [wlock unlockWithCondition:1];
}

- (void)zeroGraph
{
    NSMutableData *lineTotal = [lineData objectAtIndex:0];
    NSMutableData *lineAllow = [lineData objectAtIndex:1];
    NSMutableData *lineBlock = [lineData objectAtIndex:2];
    NSPoint *ptTotal = [lineTotal mutableBytes];
    NSPoint *ptAllow = [lineAllow mutableBytes];
    NSPoint *ptBlock = [lineBlock mutableBytes];

    int i = 0;
    for (; i < PG_HIST_POINTS; ++i) {
        ptTotal->y = ptAllow->y = ptBlock->y = 0.0;
        ++ptTotal;
        ++ptAllow;
        ++ptBlock;
    }
    ptTotal->y = ptAllow->y = ptBlock->y = 0.0;

    NSArray *args = [NSArray arrayWithObjects:
        lineData, // XXX this would be a problem, but placing it in the array adds an extra ref
        [NSNumber numberWithUnsignedInt:0],
        [NSNumber numberWithUnsignedInt:graphResolution[graphDataType]],
        nil];
    typeof(lastGraphUpdate) savedUpdate = lastGraphUpdate;
    [self updateLineData:args];
    lastGraphUpdate = savedUpdate;
}

- (void)realTimeGraphPing
{
    if (kGraphRealTime != graphDataType || maxY <= 0.0 || !lineData)
        return;
    
    struct timeval now;
    (void)gettimeofday(&now, NULL);
    if (lastGraphUpdate + graphUpdateInterval[graphDataType] <= now.tv_sec) {
        // trim off one data point (entry)
        NSMutableData *lineTotal = [lineData objectAtIndex:0];
        NSMutableData *lineAllow = [lineData objectAtIndex:1];
        NSMutableData *lineBlock = [lineData objectAtIndex:2];
        NSPoint *ptTotal = [lineTotal mutableBytes];
        NSPoint *ptAllow = [lineAllow mutableBytes];
        NSPoint *ptBlock = [lineBlock mutableBytes];
        
        int i = 0;
        float maxTotal = 0.0;
        NSPoint *tmp;
        for (; i < (PG_HIST_POINTS-1); ++i) {
            tmp = ptTotal + 1;
            ptTotal->y = tmp->y;
            tmp = ptAllow + 1;
            ptAllow->y = tmp->y;
            tmp = ptBlock + 1;
            ptBlock->y = tmp->y;
            
            if (ptTotal->y > maxTotal)
                maxTotal = ptTotal->y;
            
            ++ptTotal;
            ++ptAllow;
            ++ptBlock;
        }
        ptTotal->y = ptAllow->y = ptBlock->y = 0.0;
        
        NSArray *args = [NSArray arrayWithObjects:
            lineData, // XXX this would be a problem, but placing it in the array adds an extra ref
            [NSNumber numberWithUnsignedInt:(unsigned)maxTotal],
            [NSNumber numberWithUnsignedInt:graphResolution[graphDataType]],
            nil];
        typeof(lastGraphUpdate) savedUpdate = lastGraphUpdate;
        [self updateLineData:args];
        lastGraphUpdate = savedUpdate;
        #ifdef PPDBG
        NSLog(@"real time ping\n");
        #endif
    }
}

- (void)updateStats:(NSTimer*)timer
{
    pp_msg_stats_t s;
    
    if (NO == [[graphView superview] isHidden])
        [self realTimeGraphPing];
    
stats_try_again:
    if (!sp) {
        if (isUnloading || 0 != pp_open_session(&sp))
            return;
    }

    int err = pp_statistics_s(sp, &s);
    if (!err) {
        // Not sure why, but if we don't convert the numbers to 64 bit,
        // [NSNumber description] and whatever NSFormatter uses always return the values as signed 32bit even
        // though they are created with [numberWithUnsignedInt].
        [self setValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_blocked_addrs] forKey:@"iTotalBlocked"];
        [self setValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_in_blocked] forKey:@"iBlock"];
        [self setValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_out_blocked] forKey:@"oBlock"];
        [self setValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_in_allowed] forKey:@"iAllow"];
        [self setValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_out_allowed] forKey:@"oAllow"];

        [self setValue:[NSNumber numberWithUnsignedLongLong:
        (u_int64_t)((u_int64_t)s.pps_in_blocked + (u_int64_t)s.pps_out_blocked)] forKey:@"tBlock"];
        [self setValue:[NSNumber numberWithUnsignedLongLong:
        (u_int64_t)((u_int64_t)s.pps_in_allowed + (u_int64_t)s.pps_out_allowed)] forKey:@"tAllow"];
        
        u_int32_t avgconn, avgblk;
        (void)pp_avgconnstats(&s, (u_int32_t*)&avgconn, (u_int32_t*)&avgblk);
        
        NSNumberFormatter *intFormatter = [[[NSNumberFormatter alloc] init] autorelease];
        [intFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
        [intFormatter setNumberStyle:NSNumberFormatterDecimalStyle];
        [intFormatter setLocalizesFormat:YES];
        
        NSString *conns, *blks;
        conns = [NSString stringWithFormat:@"%@ (%@)",
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_conns_per_sec]],
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)avgconn]]];
        
        blks = [NSString stringWithFormat:@"%@ (%@)",
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_blcks_per_sec]],
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)avgblk]]];
        
        [self setValue:conns forKey:@"connPerSec"];
        [self setValue:blks forKey:@"blockPerSec"];
    } else if (ESHUTDOWN == err) {
        pp_close_session(sp);
        sp = NULL;
        goto stats_try_again;
    }
}

- (void)kextWillUnload:(NSNotification*)note
{
    isUnloading = 1;
    if (sp) {
        (void)pp_close_session(sp);
        sp = NULL;
    }
}

- (void)kextNotUnloading:(NSNotification*)note
{
    isUnloading = 0;
}
#define kextDidUnload kextNotUnloading
#define kextFailedUnload kextNotUnloading
#define kextDidLoad kextNotUnloading

#define AQUA_SPACING 8.0f
- (void)graphWillShow
{
    [self setValue:NSLocalizedString(@"Hide Graph", "") forKey:@"disclosureTitle"];
    
    NSWindow *w = [self window];
    NSView *sv = [graphView superview];
    if ([sv isHidden]) {
        NSRect wframe = [w frame], gframe = [sv frame];
        wframe.size.height += lastSuperHeight; //gframe.size.height;
        wframe.origin.y -= lastSuperHeight; //gframe.size.height;
        [w setFrame:wframe display:YES animate:YES];
        gframe = [graphView frame];
        gframe.origin.y = lastGraphY;
        gframe.size.height = lastGraphHeight;
        [graphView setFrame:gframe];
        [[graphView superview] setHidden:NO];
    }
    [self histFileDidUpdate:nil];
    [[NSDistributedNotificationCenter defaultCenter]
        addObserver:self selector:@selector(histFileDidUpdate:) name:@PG_HIST_FILE_DID_UPDATE object:nil];
}

- (void)graphWillHide
{
    [[NSDistributedNotificationCenter defaultCenter]
        removeObserver:self name:@PG_HIST_FILE_DID_UPDATE object:nil];
    
    if (lineData)
        [self zeroGraph];
    
    NSWindow *w = [self window];
    NSRect wframe = [w frame], gframe = [[graphView superview] frame];
    lastSuperHeight = gframe.size.height; // zero'd when window frame is set - why?
    wframe.size.height -=  gframe.size.height;
    wframe.origin.y += gframe.size.height;
    // the graph subview is screwed up too
    gframe = [graphView frame];
    lastGraphY = gframe.origin.y;
    lastGraphHeight = gframe.size.height;
    [[graphView superview] setHidden:YES];
    [w setFrame:wframe display:YES animate:YES];
    
    [self setValue:NSLocalizedString(@"Show Graph", "") forKey:@"disclosureTitle"];
}

- (void)graphTypeDidChange
{
    graphDataType = [[NSUserDefaults standardUserDefaults] integerForKey:@"SelectedGraphType"];
    if (lineData)
        [self zeroGraph];
    lastGraphUpdate = 0;
    [self histFileDidUpdate:nil];
}

- (void)filteredPortDidChange
{
    filteredPort = [[NSUserDefaults standardUserDefaults] integerForKey:@"FilteredPort"];
    lastGraphUpdate = 0;
    if (graphDataType != kGraphRealTime)
        [self histFileDidUpdate:nil]; 
}

- (void)opacityDidChange
{
    [[self window] setAlphaValue:[[NSUserDefaults standardUserDefaults] floatForKey:@"StatsWindowOpacity"]];
}

- (void)alwaysOnTopDidChange
{
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"StatsAlwaysOnTop"])
        [[self window] setLevel:kCGMainMenuWindowLevel-1];
    else
        [[self window] setLevel:kCGNormalWindowLevel];
}

- (void)observeValueForKeyPath:(NSString *)key ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if ([key isEqualToString:@"values." PG_STATS_HIDDEN]) {
        SEL method;
        if (NO == [[NSUserDefaults standardUserDefaults] boolForKey:PG_STATS_HIDDEN])
            method = @selector(graphWillShow);
        else
            method = @selector(graphWillHide);
        [self performSelector:method withObject:nil afterDelay:0.0];
    } else if ([key isEqualToString:@"values." @"SelectedGraphType"]) {
        [self performSelector:@selector(graphTypeDidChange) withObject:nil afterDelay:0.0];
    } else if ([key isEqualToString:@"values." @"FilteredPort"]) {
        [self performSelector:@selector(filteredPortDidChange) withObject:nil afterDelay:0.0];
    } else if ([key isEqualToString:@"values."@"StatsWindowOpacity"]) {
        [self performSelector:@selector(opacityDidChange) withObject:nil afterDelay:0.0];
    } else if ([key isEqualToString:@"values."@"StatsAlwaysOnTop"]) {
        [self performSelector:@selector(alwaysOnTopDidChange) withObject:nil afterDelay:0.0];
    } else if (graphDataType != kGraphRealTime) {
        lastGraphUpdate = 0;
        [self performSelector:@selector(histFileDidUpdate:) withObject:nil afterDelay:0.0];
    }
}

- (IBAction)showWindow:(id)sender
{
    NSDistributedNotificationCenter *nc = [NSDistributedNotificationCenter defaultCenter];
    [nc addObserver:self selector:@selector(kextWillUnload:) name:PP_KEXT_WILL_UNLOAD object:nil];
    [nc addObserver:self selector:@selector(kextDidUnload:) name:PP_KEXT_DID_UNLOAD object:nil];
    [nc addObserver:self selector:@selector(kextFailedUnload:) name:PP_KEXT_FAILED_UNLOAD object:nil];
    [nc addObserver:self selector:@selector(kextDidLoad:) name:PP_KEXT_DID_LOAD object:nil];
    
    // set initial values, then create timer
    [self updateStats:nil];
    
    statsTimer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self
        selector:@selector(updateStats:) userInfo:nil repeats:YES];
    
    [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self
        forKeyPath:@"values." PG_STATS_HIDDEN options:0 context:nil];
    [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self
        forKeyPath:@"values." @"SelectedGraphType" options:0 context:nil];
    [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self
        forKeyPath:@"values." @"FilteredPort" options:0 context:nil];
    [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self
        forKeyPath:@"values." @"TCPFilter" options:0 context:nil];
    [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self
        forKeyPath:@"values." @"UDPFilter" options:0 context:nil];
    [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self
        forKeyPath:@"values." @"StatsWindowOpacity" options:0 context:nil];
    [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self
        forKeyPath:@"values." @"StatsAlwaysOnTop" options:0 context:nil];
    
    [NSApp activateIgnoringOtherApps:YES];
    [[super window] setHidesOnDeactivate:NO];
    [super showWindow:sender];
    
    [graphView reloadAttributes];
}

#define GRAPH_DATA_TAG 901
- (void)windowDidLoad
{
    [graphView setBorderColor:[NSColor disabledControlTextColor]];
    [graphView setLabel:NSLocalizedString(@"Time", "") forAxis:kSM2DGraph_Axis_X];
    [graphView setLabel:NSLocalizedString(@"Connections", "") forAxis:kSM2DGraph_Axis_Y_Left];
    //[graphView setLabel:NSLocalizedString(@"", "") forAxis:kSM2DGraph_Axis_Y_Right];
    //int marks = [graphView frame].size.width / (PG_HIST_POINTS / TICK_FACTOR);
    //[graphView setNumberOfTickMarks:marks forAxis:kSM2DGraph_Axis_X];
    
    NSMenu *m = [[NSMenu alloc] initWithTitle:@""];
    // XXX must be added in the order of the graph constants (so the indices match)
    [m addItemWithTitle:NSLocalizedString(@"Real Time", "") action:nil keyEquivalent:@""];
    [m addItemWithTitle:NSLocalizedString(@"Last 5 minutes", "") action:nil keyEquivalent:@""];
    [m addItemWithTitle:NSLocalizedString(@"Last 15 minutes", "") action:nil keyEquivalent:@""];
    [m addItemWithTitle:NSLocalizedString(@"Last 30 minutes", "") action:nil keyEquivalent:@""];
    [m addItemWithTitle:NSLocalizedString(@"Last hour", "") action:nil keyEquivalent:@""];
    [m addItemWithTitle:NSLocalizedString(@"Last 12 hours", "") action:nil keyEquivalent:@""];
    [m addItemWithTitle:NSLocalizedString(@"Last Day", "") action:nil keyEquivalent:@""];
    [m addItemWithTitle:NSLocalizedString(@"Last Week", "") action:nil keyEquivalent:@""];
    [m addItemWithTitle:NSLocalizedString(@"Last Month", "") action:nil keyEquivalent:@""];
    [m addItemWithTitle:NSLocalizedString(@"Last Year", "") action:nil keyEquivalent:@""];
    [m addItemWithTitle:NSLocalizedString(@"All", "") action:nil keyEquivalent:@""];
    id popup = [[graphView superview] viewWithTag:GRAPH_DATA_TAG];
    [popup setMenu:m];
    [m release];
    [popup selectItemAtIndex:graphDataType];
    
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"StatsAlwaysOnTop"])
        [[self window] setLevel:kCGMainMenuWindowLevel-1];
    
    [[self window] setAlphaValue:[[NSUserDefaults standardUserDefaults] floatForKey:@"StatsWindowOpacity"]];
    
    [NSThread detachNewThreadSelector:@selector(processHistoryThread:) toTarget:self withObject:nil];
    
    if (NO == [[NSUserDefaults standardUserDefaults] boolForKey:PG_STATS_HIDDEN])
        [self graphWillShow];
    else
        [self graphWillHide];
}

- (void)windowWillClose:(NSNotification*)note
{
    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self
        forKeyPath:@"values." PG_STATS_HIDDEN];
    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self
        forKeyPath:@"values." @"SelectedGraphType"];
    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self
        forKeyPath:@"values." @"FilteredPort"];
    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self
        forKeyPath:@"values." @"TCPFilter"];
    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self
        forKeyPath:@"values." @"UDPFilter"];
    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self
        forKeyPath:@"values." @"StatsWindowOpacity"];
    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self
        forKeyPath:@"values." @"StatsAlwaysOnTop"];
    
    NSDistributedNotificationCenter *nc = [NSDistributedNotificationCenter defaultCenter];
    [nc removeObserver:self name:@PG_HIST_FILE_DID_UPDATE object:nil];
    [nc removeObserver:self name:PP_KEXT_WILL_UNLOAD object:nil];
    [nc removeObserver:self name:PP_KEXT_DID_UNLOAD object:nil];
    [nc removeObserver:self name:PP_KEXT_FAILED_UNLOAD object:nil];
    [nc removeObserver:self name:PP_KEXT_DID_LOAD object:nil];
    
    [statsTimer invalidate];
    statsTimer = nil;
    
    [self kextWillUnload:nil];
    isUnloading = 0;
    
#ifdef notyet
    // kill the thread
    NSArray *args = [NSArray arrayWithObjects:[NSNull null], nil];
    [wlock lock];
    [work addObject:args];
    [wlock unlockWithCondition:1];
    
    // wait for exit
    [wlock lock];
    [wlock unlock];
    
    [self autorelease];
#endif
}

- (id)init
{
    self = [super initWithWindowNibName:@"PGStats"];
    [super setWindowFrameAutosaveName:@"Statistics"];
    
    lineAttrs = [[NSArray alloc] initWithObjects:
        [NSDictionary dictionaryWithObjectsAndKeys:
            [NSColor blackColor], NSForegroundColorAttributeName, nil], // total
        [NSDictionary dictionaryWithObjectsAndKeys:
            [NSColor greenColor], NSForegroundColorAttributeName, nil], // allowed
        [NSDictionary dictionaryWithObjectsAndKeys:
            [NSColor redColor], NSForegroundColorAttributeName, nil], // blocked
        nil];
    
    wlock = [[NSConditionLock alloc] initWithCondition:0];
    work = [[NSMutableArray alloc] init];
    
    u_int64_t mem = 0ULL;
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    size_t len = sizeof(mem);
    (void)sysctl(mib, 2, &mem, &len, NULL, 0);
    if (0 == mem)
        mem = (128 * 1024 * 1024);
    
    maxHistData = ((typeof(maxHistData))(mem / 10)/*10%*/ / sizeof(pg_hist_ent_t) ) * sizeof(pg_hist_ent_t);
    maxHistData += sizeof(pg_hist_ent_t);
    
    maxY = 0.0;
    
    if ([[NSUserDefaults standardUserDefaults] objectForKey:@"SelectedGraphType"]) {
        graphDataType = [[NSUserDefaults standardUserDefaults] integerForKey:@"SelectedGraphType"];
        if (graphDataType >= kGraphTypeCount) {
            graphDataType = kGraphRealTime;
            [[NSUserDefaults standardUserDefaults] setInteger:graphDataType forKey:@"SelectedGraphType"];
        }
    } else {
        graphDataType = kGraphRealTime;
        [[NSUserDefaults standardUserDefaults] setInteger:graphDataType forKey:@"SelectedGraphType"];
    }
    
    if ([[NSUserDefaults standardUserDefaults] objectForKey:@"FilteredPort"]) {
        graphDataType = [[NSUserDefaults standardUserDefaults] integerForKey:@"FilteredPort"];
        if (filteredPort > 65535) {
            filteredPort = 0;
            [[NSUserDefaults standardUserDefaults] setInteger:filteredPort forKey:@"FilteredPort"];
        }
    } else {
        filteredPort = 0;
        [[NSUserDefaults standardUserDefaults] setInteger:filteredPort forKey:@"FilteredPort"];
    }
    
    if (![[NSUserDefaults standardUserDefaults] objectForKey:@"TCPFilter"])
        [[NSUserDefaults standardUserDefaults] setInteger:1 forKey:@"TCPFilter"];
    if (![[NSUserDefaults standardUserDefaults] objectForKey:@"UDPFilter"])
        [[NSUserDefaults standardUserDefaults] setInteger:1 forKey:@"UDPFilter"];
    if (![[NSUserDefaults standardUserDefaults] floatForKey:@"StatsWindowOpacity"])
        [[NSUserDefaults standardUserDefaults] setFloat:1.0 forKey:@"StatsWindowOpacity"];
    
    return (self);
}

- (void)dealloc
{
    [lineData release];
    [lineAttrs release];
    [wlock release];
    [work release];
    [super dealloc];
}

- (IBAction)showPrefs:(id)sender
{
    [NSApp beginSheet:prefsWindow modalForWindow:[super window] modalDelegate:self didEndSelector:nil contextInfo:nil];
}

- (IBAction)closePrefs:(id)sender
{
    [NSApp endSheet:prefsWindow];
    [prefsWindow close];
}

// Graph data provider methods
- (unsigned int)numberOfLinesInTwoDGraphView:(SM2DGraphView *)gv
{
    return (PG_GRAPH_LINES);
}

- (NSDictionary *)twoDGraphView:(SM2DGraphView *)gv attributesForLineIndex:(unsigned int)idx
{
    if (idx < [lineAttrs count])
        return ([lineAttrs objectAtIndex:idx]);
    
    return (nil);
}

- (NSData *)twoDGraphView:(SM2DGraphView *)gv dataObjectForLineIndex:(unsigned int)idx
{
    if (idx < [lineData count])
        return ([lineData objectAtIndex:idx]);
    
    return (nil);
}

- (double)twoDGraphView:(SM2DGraphView *)gv maximumValueForLineIndex:(unsigned int)idx
    forAxis:(SM2DGraphAxisEnum)axis
{
    double m;
    switch (axis) {
        case kSM2DGraph_Axis_Y:
        case kSM2DGraph_Axis_Y_Right:
            m = maxY > 0.0 ? maxY : 1.0;
            break;
        case kSM2DGraph_Axis_X:
            m = maxX;
            break;
    };
    return (m);
}

- (double)twoDGraphView:(SM2DGraphView *)gv minimumValueForLineIndex:(unsigned int)idx
    forAxis:(SM2DGraphAxisEnum)axis
{
    return (0.0);
}

#ifdef notyet
- (void)twoDGraphView:(SM2DGraphView *)gv didClickPoint:(NSPoint)pt
{
    pt = [gv convertPoint:pt fromView:gv toLineIndex:0];
    NSLog(@"clicked point (x=%f, y=%f)\n", pt.x, pt.y);
}
#endif
    
@end
