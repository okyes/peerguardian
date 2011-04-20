/*
    Copyright 2006-2008 Brian Bergstrand

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

#import "pplib.h"
#import "pgsb.h"
#import "pploader.h"
#import "PGStatsController.h"
#import "PGStatusItem.h"

@interface PGAgent : NSObject {
}

@end

static PGAgent *pga = nil;

@implementation PGAgent

- (void)quit:(NSNotification*)note
{
    [NSApp terminate:nil];
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    [[NSUserDefaults standardUserDefaults] registerDefaults:
        [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:NO]
        forKey:PG_STATS_HIDDEN]];
    
    
    NSDistributedNotificationCenter *nc = [NSDistributedNotificationCenter defaultCenter];
    [nc addObserver:self selector:@selector(quit:) name:PP_HELPER_QUIT object:nil];
    
    (void)[PGStatusItem sharedInstance];
}

@end

int main (int argc, char *argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    pgsbinit();
    
    pga = [[PGAgent alloc] init];

    [[NSRunLoop currentRunLoop] configureAsServer];
    [[NSApplication sharedApplication] setDelegate:pga];
    
    [pool release];
    
    [NSApp run];
    
    return (0);
}
