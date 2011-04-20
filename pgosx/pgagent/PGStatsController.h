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
#import <Cocoa/Cocoa.h>

// Total, Allowed, Blocked
#define PG_GRAPH_LINES 3
#define PG_STATS_HIDDEN @"StatsGraphHidden"

@interface PGStatsController : NSWindowController {
    IBOutlet id graphView;
    IBOutlet id prefsWindow;
    
    // Graph data
    NSArray *lineData;
    NSArray *lineAttrs;
    double maxY, maxX;
    
    // Misc
    NSTimer *statsTimer;
    unsigned maxHistData;
    float lastGraphHeight, lastGraphY, lastSuperHeight;
    NSMutableArray *work;
    NSConditionLock *wlock;
    
    // Bindings
    NSString *disclosureTitle;
    unsigned graphDataType, filteredPort;
    unsigned long long iBlock, oBlock, iAllow, oAllow, iTotalBlocked,
    tBlock, tAllow;
    NSString *connPerSec, *blockPerSec;
}

+ (PGStatsController*)sharedInstance;

- (IBAction)showPrefs:(id)sender;
- (IBAction)closePrefs:(id)sender;

@end
