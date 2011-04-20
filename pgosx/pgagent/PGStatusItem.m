/*
    Copyright 2007-2008 Brian Bergstrand

    This program is free software; you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program;
    if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#import <sys/types.h>
#import <unistd.h>

#import "pplib.h"
#import "pploader.h"
#import "PGStatusItem.h"
#import "PGStatsController.h"

static PGStatusItem *shared = nil;

#define LIST_ITEM_TAG 100
#define FILTER_ITEM_TAG 110

#define PGProxySupportClass PGStatusItem
#define PGPROXY_DONT_LAUNCH 1
#import "../gui/proxy.m"

@implementation PGStatusItem

+ (PGStatusItem*)sharedInstance
{
    return (shared ? shared : (shared = [[PGStatusItem alloc] init]));
}

- (id)retain
{
    return (self);
}

- (void)release
{

}

- (unsigned)retainCount
{
    return (UINT_MAX);
}

- (void)setFilterState:(BOOL)enabled
{
    if (enabled != ppenabled) {
        ppenabled = enabled;
        
        NSString *title;
        if (enabled) {
            title = NSLocalizedString(@"Disable Filters", "");
            [statusItem setTitle:[statusItem title]]; // get rid of any attributes
        } else {
            title = NSLocalizedString(@"Enable Filters", "");
            
            NSAttributedString *atitle = [[NSAttributedString alloc]
            initWithString:[statusItem title] attributes:
                [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSFont menuFontOfSize:[NSFont systemFontSize]], NSFontAttributeName,
                    [NSColor redColor], NSForegroundColorAttributeName,
                    nil]];
            [statusItem setAttributedTitle:atitle];
            [atitle release];
        }
        
        NSMenuItem *item = [[statusItem menu] itemWithTag:FILTER_ITEM_TAG];
        [item setTitle:title];
    }
}

- (void)filterStateChange:(NSNotification*)note
{
    BOOL enabled = [[[note userInfo] objectForKey:PP_FILTER_CHANGE_ENABLED_KEY] boolValue];
    [self setFilterState:enabled];
}

- (void)proxyDied
{
    [self performSelector:@selector(updateListMenu) withObject:nil];
}

- (void)proxyAttached
{
    BOOL enabled = [proxy isEnabled];
    [self setFilterState:enabled];
    [self performSelector:@selector(updateListMenu) withObject:nil];
}

- (IBAction)statistics:(id)sender
{
    [[PGStatsController sharedInstance] showWindow:nil];
}

- (IBAction)enableDisableList:(id)sender
{
    if (proxy) {
        id list = [sender representedObject];
        @try {
            [list setObject:[NSNumber numberWithBool:!ListActive(list)] forKey:@"Active"];
            (void)[proxy changeList:list];
        } @catch (NSException *e) {
            NSLog(@"enableDisableList: Exception '%@'\n", e);
        }
    }
}

- (IBAction)donate:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:
        [NSURL URLWithString:@"http://www.bergstrand.org/brian/donate"]];
}

- (IBAction)launchGUI:(id)sender
{
    [[NSWorkspace sharedWorkspace] launchAppWithBundleIdentifier:@"xxx.qnation.PeerProtectorApp"
        options:NSWorkspaceLaunchAsync additionalEventParamDescriptor:nil launchIdentifier:nil];
}

- (IBAction)enableDisableWrapper:(id)sender
{
    [NSApp activateIgnoringOtherApps:YES];
    [self enableDisable:sender];
}

- (void)updateListMenu
{
    if (proxy) {
        @try {
            id oldlists = pglists;
            
            pglists = [[proxy lists] retain];
            
            NSMenuItem *item = [[statusItem menu] itemWithTag:LIST_ITEM_TAG];
            NSMenu *submenu = [[NSMenu alloc] initWithTitle:@""];
            [submenu setAutoenablesItems:NO];
            NSMenuItem *listItem;
            
            NSEnumerator *en = [pglists objectEnumerator];
            NSDictionary *list;
            #ifdef notyet
            NSColor *disabledColor = [NSColor disabledControlTextColor];
            NSFont *menuFont = [NSFont menuFontOfSize:[NSFont systemFontSize]];
            #endif
            while ((list = [en nextObject])) {
                NSString *title = ListName(list);
                listItem = [[NSMenuItem alloc] initWithTitle:title
                    action:@selector(enableDisableList:) keyEquivalent:@""];
                [listItem setTarget:self];
                [listItem setState: ListActive(list) ? NSOnState : NSOffState];
                #ifdef notyet
                if (!ListActive(list)) {
                    NSAttributedString *atitle = [[NSAttributedString alloc]
                        initWithString:[listItem title] attributes:
                            [NSDictionary dictionaryWithObjectsAndKeys:
                                menuFont, NSFontAttributeName,
                                disabledColor, NSForegroundColorAttributeName,
                                nil]];
                    [listItem setAttributedTitle:atitle];
                    [atitle release];
                }
                #endif
                [listItem setRepresentedObject:list];
                [submenu addItem:listItem];
                [listItem release];
            }
            
            [item setSubmenu:submenu];
            [submenu release];
            
            [oldlists release];
        } @catch (NSException *e) {
            NSLog(@"updateListMenu: Exception '%@'\n", e);
        }
    } else {
         NSMenuItem *item = [[statusItem menu] itemWithTag:LIST_ITEM_TAG];
         NSMenu *submenu = [[NSMenu alloc] initWithTitle:@""];
         [item setSubmenu:submenu];
         [submenu release];
         
         [self performSelector:@selector(updateListMenu) withObject:nil afterDelay:5.0];
    }
        
}

// DO Client extensions
- (oneway void)listWillUpdate:(id)list
{
}

- (oneway void)listDidUpdate:(id)list
{
}

- (oneway void)listFailedUpdate:(id)list error:(NSDictionary*)error
{
}

- (oneway void)listDidChange:(id)list
{
    [self performSelector:@selector(updateListMenu) withObject:nil afterDelay:0.0];
}

- (oneway void)listWasRemoved:(id)list
{
    [self performSelector:@selector(updateListMenu) withObject:nil afterDelay:0.0];
}


- (void)applicationWillTerminate:(NSNotification*)note
{
    [self killProxy];
}

- (id)init
{
    self = [super init];
    
    NSDistributedNotificationCenter *nc = [NSDistributedNotificationCenter defaultCenter];
    [nc addObserver:self selector:@selector(filterStateChange:) name:PP_FILTER_CHANGE object:nil];
    [nc addObserver:self selector:@selector(statistics:) name:PP_SHOW_STATS object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationWillTerminate:)
        name:NSApplicationWillTerminateNotification object:NSApp];
    
    // Register status item
    statusItem = [[[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength] retain];
    [statusItem setTitle:@"PG"];
    [statusItem setHighlightMode:YES];
    
    NSMenu *m = [[NSMenu alloc] initWithTitle:@""];
    [m setAutoenablesItems:NO];
    
    NSMenuItem *item;
    item = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"Lists", "") action:nil keyEquivalent:@""];
    [item setTag:LIST_ITEM_TAG];
    [item setEnabled:YES];
    [m addItem:item];
    [item release];
    
    item = [[NSMenuItem alloc] initWithTitle:@"Enable Filters" action:@selector(enableDisableWrapper:) keyEquivalent:@""];
    [item setTarget:self];
    [item setTag:FILTER_ITEM_TAG];
    [m addItem:item];
    [item release];
    
    item = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"Statistics", "")
        action:@selector(statistics:) keyEquivalent:@""];
    [item setTarget:self];
    [m addItem:item];
    [item release];
    
    [m addItem:[NSMenuItem separatorItem]];
    
    item = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"Launch PeerGuardian", "")
        action:@selector(launchGUI:) keyEquivalent:@""];
    [item setTarget:self];
    [m addItem:item];
    [item release];
    
    item = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"Donate", "")
        action:@selector(donate:) keyEquivalent:@""];
    [item setTarget:self];
    [m addItem:item];
    [item release];
    
    item = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"Quit", "")
        action:@selector(terminate:) keyEquivalent:@""];
    [item setTarget:NSApp];
    [m addItem:item];
    [item release];
    
    [statusItem setMenu:m];
    
    [self loadProxy];
    
    [statusItem setEnabled:YES];
    [m release];
}

@end
