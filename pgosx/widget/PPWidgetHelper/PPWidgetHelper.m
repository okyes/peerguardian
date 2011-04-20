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
#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

#define PG_MAKE_TABLE_ID
#import "pplib.h"
#import "pploader.h"
#import "PPWidgetHelper.h"

@implementation PPWidgetHelper

+(NSString*)webScriptNameForSelector:(SEL)sel
{
    NSString *methodName = nil;
    if (sel = @selector(statistics))
        methodName = @"statistics";
    return (methodName);
}

// Control access to selectors
+(BOOL)isSelectorExcludedFromWebScript:(SEL)sel
{
    BOOL excluded = NO;
    if (sel != @selector(statistics))
        excluded = YES;
    return (excluded);
}

// Prevent direct key access
+(BOOL)isKeyExcludedFromWebScript:(const char*)key {
    return (YES);
}

// The meat...
- (NSArray*)statistics
{
    pp_msg_stats_t s;
    
    NSNumberFormatter *intFormatter = [[[NSNumberFormatter alloc] init] autorelease];
    [intFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
    [intFormatter setNumberStyle:NSNumberFormatterDecimalStyle];
    [intFormatter setLocalizesFormat:YES];

stats_try_again:
    if (!sp) {
        if (0 != pp_open_session(&sp))
            return (nil);
    }

    int err = pp_statistics_s(sp, &s);
    if (!err) {
        // Not sure why, but if we don't convert the numbers to 64 bit,
        // [NSNumber description] and whatever NSFormatter use always return the values as signed 32bit even
        // though they are created with [numberWithUnisgnedInt].
        #if 0
        // There is no JS bridge for dictionaries (as of 10.4.3)
        NSDictionary *d = [NSDictionary dictionaryWithObjectsAndKeys:
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_blocked_addrs]],
                @"totalAddresses",
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_in_blocked]],
                @"incomingBlocks",
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_out_blocked]],
                @"outgoingBlocks",
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_in_allowed]],
                @"incomingAllows",
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_out_allowed]],
                @"outgoingAllows",
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:
                (u_int64_t)((u_int64_t)s.pps_in_blocked + (u_int64_t)s.pps_out_blocked)]],
                @"totalBlocks",
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:
                (u_int64_t)((u_int64_t)s.pps_in_allowed + (u_int64_t)s.pps_out_allowed)]],
                @"totalAllows",
            nil];
        #endif
        
        u_int32_t connPerSec, blockPerSec;
        (void)pp_avgconnstats(&s, &connPerSec, &blockPerSec);
        
        NSString *conns, *blks;
        conns = [NSString stringWithFormat:@"%@ (%@)",
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_conns_per_sec]],
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)connPerSec]]];
        
        blks = [NSString stringWithFormat:@"%@ (%@)",
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_blcks_per_sec]],
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)blockPerSec]]];
        
        NSArray *d = [NSArray arrayWithObjects:
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_blocked_addrs]],
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_in_blocked]],
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_out_blocked]],
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_in_allowed]],
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:(u_int64_t)s.pps_out_allowed]],
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:
                (u_int64_t)((u_int64_t)s.pps_in_blocked + (u_int64_t)s.pps_out_blocked)]],
            [intFormatter stringForObjectValue:[NSNumber numberWithUnsignedLongLong:
                (u_int64_t)((u_int64_t)s.pps_in_allowed + (u_int64_t)s.pps_out_allowed)]],
            conns,
            blks,
            nil];
        //NSLog(@"%s: %@\n", __FUNCTION__, d);
        return (d);
    } else if (ESHUTDOWN == err) {
        (void)pp_close_session(sp);
        sp = NULL;
        goto stats_try_again;
    }
    return (nil);
}

-(void)windowScriptObjectAvailable:(WebScriptObject*)wso
{
    [wso setValue:self forKey:@"PPWidgetHelper"];
}

- (void)kextWillUnload:(NSNotification*)note
{
    if (sp) {
        (void)pp_close_session(sp);
        sp = NULL;
    }
}

-(id)initWithWebView:(WebView*)webView
{
    [NSNumberFormatter setDefaultFormatterBehavior:NSNumberFormatterBehavior10_4];
    
    self = [super init];
    NSDistributedNotificationCenter *nc = [NSDistributedNotificationCenter defaultCenter];
    [nc addObserver:self selector:@selector(kextWillUnload:) name:PP_KEXT_WILL_UNLOAD object:nil];
    
    return (self);
}

-(void)dealloc
{
    if (sp)
        pp_close_session(sp);
    [super dealloc];
}

@end
