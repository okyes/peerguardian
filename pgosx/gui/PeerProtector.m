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
#define PG_MAKE_TABLE_ID
#import "PeerProtector.h"
#import <unistd.h>
#import <fcntl.h>
#import <sys/stat.h>
#import <sys/sysctl.h>
#import <sys/types.h>
#import <sys/socket.h>
#import <netinet/in.h>
#import <arpa/inet.h>

#import "pplib.h"
#import "pgsb.h"
#import "p2b.h"
#import "pploader.h"

#import "UKFileWatcher.h"
#import "UKKQueue.h"

#import "BBNetUpdate/BBNetUpdateVersionCheckController.h"

#import "PGLookupController.h"
#import "NSWorkspace+PPAdditions.m"

static NSMenu *dockMenu = nil;

//2004-08-27 04:12:10 GMT
static NSDate *LIST_EPOCH = nil;

// Control tags
#define FILE_MENU_TAG 10
#define DISABLE_FILTERS_TAG 100
#define ADD_EDIT_OK 1001
#define ADD_EDIT_CANCEL 1002
#define DOCK_DISABLE_FILTERS_TAG 2001
#define LICENSE_TAG 3001

#define PGProxySupportClass PeerProtector
#import "proxy.m"

@implementation PeerProtector

+ (id)allocWithZone:(NSZone *)zone
{
    NSDictionary *defaults = [NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithBool:YES], @"BBNetUpdateDontAskConnect",
        [NSNumber numberWithInt:1], @"Stats Update Interval",
        [NSNumber numberWithBool:NO], @"Display Blocks Only",
        nil];
    
    [[NSUserDefaults standardUserDefaults] registerDefaults:defaults];
    return ([super allocWithZone:zone]);
}

- (NSURL*)firstURL:(NSDictionary*)list
{
    NSArray *urls = [[list valueForKey:@"URLs"] allKeys];
    BOOL triedController = NO;
    NSURL *url;

first_enum_urls:
    url = nil;
    if (urls && [urls count] > 0) {
        @try {
            url = [NSURL URLWithString:[urls objectAtIndex:0]];
        } @finally {}
    }
    if (!url && NO == triedController) {
        triedController = YES;
        urls = [urlsController content];
        goto first_enum_urls;
    }
    return (url);
}

- (NSString*)selectedURL
{
    [currentlySelectedURL release];
    currentlySelectedURL = nil;
    
    @try {
        id obj = nil;
        NSArray *s = [urlsController selectedObjects];
        if ([s count] > 0)
            obj = [s objectAtIndex:0];
        if (obj && [obj isKindOfClass:[NSString class]])
            return ((currentlySelectedURL = [obj retain]));
    } @finally {}
    
    return (@"");
}

- (void)setSelectedURL:(id)object
{
    if (!object)
        return;
    @try {
        if (currentlySelectedURL) {
            [urlsController removeObject:currentlySelectedURL];
            [currentlySelectedURL release];
            currentlySelectedURL = nil;
        }
        if ([object length] > 0) {
            [urlsController addObject:object];
            if ([urlsController setSelectedObjects:[NSArray arrayWithObject:object]])
                currentlySelectedURL = [object retain];
        }
    } @finally {}
}

- (IBAction)addURL:(id)sender
{
    NSDictionary *list = [[listsController selectedObjects] objectAtIndex:0];
    NSURL *url = [self firstURL:list];
    if (url && [url isFileURL]) {
        // Local file lists are allowed only one URL.
        NSBeep();
        return;
    }
    
    id obj = [[[urlsController objectClass] alloc] init];
    if (obj)
        [urlsController addObject:obj];
    else
        return;
    if ([urlsController setSelectedObjects:[NSArray arrayWithObject:obj]]) {
        [currentlySelectedURL release];
        currentlySelectedURL = [obj retain];
    }
}

- (IBAction)removeURL:(id)sender
{
    if (currentlySelectedURL) {
        [urlsController removeObject:currentlySelectedURL];
        [currentlySelectedURL release];
        currentlySelectedURL = nil;
    }
}

- (void)updateURLs:(NSMutableDictionary*)list
{
    NSArray *listurls = [[list valueForKey:@"URLs"] allKeys];
    NSMutableArray *newurls = [urlsController content];
    if (NO == [listurls isEqualToArray:newurls]) {
        NSMutableDictionary *urlsdict = [[list valueForKey:@"URLs"] mutableCopy];
        NSEnumerator *en = [newurls objectEnumerator];
        NSString *url;
        while ((url = [en nextObject])) {
            if ([url length] > 0 && nil == [urlsdict objectForKey:url])
                [urlsdict setObject:LIST_EPOCH forKey:url];
        }
        en = [listurls objectEnumerator];
        while ((url = [en nextObject])) {
            if (NO == [newurls containsObject:url])
                [urlsdict removeObjectForKey:url];
        }
        [list setValue:urlsdict forKey:@"URLs"];
        [urlsdict release];
    }
}

- (void)clearMsg:(NSTimer*)timer
{
    clearMsgTimer = nil;
    [self setMsg:@""];
}

- (void)setMsg:(NSString*)msg
{
    [listMessage setStringValue:msg];
    if ([msg length]) {
        [clearMsgTimer invalidate];
        clearMsgTimer = [NSTimer scheduledTimerWithTimeInterval:20.0 target:self
            selector:@selector(clearMsg:) userInfo:nil repeats:NO];
    }
}

- (void)fileDataAvailable:(NSNotification*)note
{
    NSString *data = [[NSString alloc] initWithData:
        [[note userInfo] objectForKey:NSFileHandleNotificationDataItem]
        encoding:NSUTF8StringEncoding]; 
    
    NSMutableAttributedString *str = [[NSMutableAttributedString alloc] initWithString:data];
    
    BOOL blocksOnly = [[NSUserDefaults standardUserDefaults] boolForKey:@"Display Blocks Only"];
    
    // Highlight block events/remove non-block events
    NSRange r;
    NSArray *lines = [data componentsSeparatedByString:@"\n"];
    if (lines && [lines count]) {
        NSEnumerator *en = [lines objectEnumerator];
        NSString *line;
        unsigned offset = 0;
        unsigned len;
        while ((line = [en nextObject])) {
            // XXX - Dependency on pplogger internals
            if (NO == blocksOnly) // only highlight if showing everything
                r = [line rangeOfString:@"-Blck-"];
            else
                r.location = NSNotFound;
            if (NSNotFound != r.location) {
                r.location = offset; // start of line
                len = r.length = [line length];
                [str setAttributes:blockedLogAttrs range:r];
                len++; // make sure offset reflects the newline
            } else if ((len = [line length])) {
                len += 1; // + 1 for '\n' puts us at the start of the next line
                if (blocksOnly) {
                    r = [line rangeOfString:@"-Blck-"];
                    if (NSNotFound == r.location) {
                        // not a block, strip it
                        r.location = offset; // start of line
                        len = r.length = len;
                        [str replaceCharactersInRange:r withString:@""];
                        len = 0; // make sure offset reflects the removal
                    }
                }
            }
            
            offset += len;
        }
    }
    [data release];
    
    [[logText textStorage] appendAttributedString:str];
    [str release];

    r = [logText selectedRange];
    // auto-scroll if the user doesn't have anything selected
    if (0 == r.length)
        [logText scrollRangeToVisible:NSMakeRange([[logText string] length] - 1, 0)];

#ifdef cpueater
    [logFile readInBackgroundAndNotify];
#endif
}

#ifndef cpueater
- (void)watcher:(id<UKFileWatcher>)kq receivedNotification:(NSString*)nm forPath:(NSString*)fpath
{
    if ([UKFileWatcherWriteNotification isEqualToString:nm]
        && [fpath isEqualToString:PP_LOGFILE]) {
        NSData *data = nil;
        @try {
            data = [logFile readDataToEndOfFile];
        } @catch (id exception) { }
        if (!data)
            return;
        
        NSNotification *note =
            [NSNotification notificationWithName:NSFileHandleReadCompletionNotification
            object:self
            userInfo:[NSDictionary dictionaryWithObjectsAndKeys:
                data, NSFileHandleNotificationDataItem, nil]];
        [self fileDataAvailable:note];
    }
}
#endif

- (void)awakeFromNib
{
    NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
    // List columns changed in 1.2.3, make sure it takes effect
    if (NO == [ud boolForKey:@"123 Table"]) {
        [ud removeObjectForKey:@"NSTableView Columns List"];
        [ud setBool:YES forKey:@"123 Table"];
    }
    id obj = [NSButtonCell new];
    [obj setButtonType:NSSwitchButton];
    [obj setAllowsMixedState:NO];
    [obj setTitle:@""];
    [obj setEnabled:NO]; // Editing must be done in the Edit window
    [[listTable tableColumnWithIdentifier:@"Active"] setDataCell:obj];
    [[listTable tableColumnWithIdentifier:@"Always Allow"] setDataCell:obj];
    [[listTable tableColumnWithIdentifier:@"Block HTTP"] setDataCell:obj];
    
    [listTable setTarget:self];
    [listTable setDoubleAction:@selector(editList:)];
    
    [listMessage setStringValue:@""];

    [listsController setContent:[NSMutableArray array]];
    
    // see fileDataAvailable:
    blockedLogAttrs = [[NSDictionary alloc] initWithObjectsAndKeys:
            [NSColor redColor], NSForegroundColorAttributeName, nil];
    
#ifdef obsolete
    [[NSNotificationCenter defaultCenter] addObserver:self
        selector:@selector(windowWillClose:) name:NSWindowWillCloseNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
        selector:@selector(windowDidBecomeKey:) name:NSWindowDidBecomeKeyNotification object:nil];
#endif

    [exportProgressBar setUsesThreadedAnimation:NO];
    
    ppenabled = YES;
}

- (void)openLogFile:(NSNotification*)note
{
    if (logFile) {
        // pplogger has opened the file, it may have been moved and re-created in the proces
        // close our current handle and reopen the file
    #ifndef cpueater
        [[UKKQueue sharedQueue] removePath:PP_LOGFILE];
    #endif
        [logFile closeFile];
        [logFile release];
    }
    
    logFile = [[NSFileHandle fileHandleForReadingAtPath:PP_LOGFILE] retain];
    
    if (!logFile)
        return;
    
    // we only show the entries added since we've launched
    [logFile seekToEndOfFile];
#ifndef cpueater
    [[UKKQueue sharedQueue] addPath:PP_LOGFILE];
#else
    [[NSNotificationCenter defaultCenter] addObserver:self
        selector:@selector(fileDataAvailable:)
        name:NSFileHandleReadCompletionNotification object:logFile];
    // This thing chwes up CPU time like Oprah chews up drumsticks.
    // On a dual 2.5, this uses 120% after a few reads.
    // UKKQueue uses less than .1%. WTF?!
    [logFile readInBackgroundAndNotify];
#endif
}

#define BADGE_SIZE 48.0
#define BADGE_WOFFSET 2.0
#define BADGE_HOFFSET 48.0
- (NSImage*)disabledIcon
{
    NSImage *app = [NSImage imageNamed:@"app.icns"];
    NSImage *badge = [[NSImage imageNamed:@"disabled.tif"] copy];
    [badge setScalesWhenResized:YES];
    [badge setSize:NSMakeSize(BADGE_SIZE, BADGE_SIZE)];
    
    NSSize sz = [app size];
    NSImage *icon = [[NSImage alloc] initWithSize:sz];
    NSPoint p;
    [icon lockFocus];
    
    p.x = p.y = 0.0;
    [app compositeToPoint:p operation:NSCompositeSourceOver];
    p.x = 0.0 + BADGE_WOFFSET;
    p.y = sz.height - BADGE_HOFFSET;
    [badge compositeToPoint:p operation:NSCompositeSourceOver];
    
    [icon unlockFocus];
    
    [badge release];
    
    return ([icon autorelease]);
}

- (BOOL)automaticUpdate
{
    if (proxy)
        return ((autoUpdate = [[proxy objectForKey:@"Update Interval"] boolValue]));
    else
        return (NO);
}

- (void)setAutomaticUpdate:(BOOL)enabled
{
    if (proxy && enabled != autoUpdate) {
        [proxy setObject:[NSNumber numberWithInt:enabled] forKey:@"Update Interval"];
        autoUpdate = enabled;
    }
}

- (BOOL)autoAllowDNS
{
    if (proxy)
        return ((autoUpdate = [[proxy objectForKey:@"Auto-Allow DNS"] boolValue]));
    else
        return (NO);
}

- (void)setAutoAllowDNS:(BOOL)enabled
{
    if (proxy && enabled != autoUpdate) {
        [proxy setObject:[NSNumber numberWithInt:enabled] forKey:@"Auto-Allow DNS"];
        autoUpdate = enabled;
    }
}

- (void)setFilterState:(BOOL)enabled
{
    if (enabled != ppenabled) {
        ppenabled = enabled;
        
        NSString *title;
        if (enabled) {
            [NSApp setApplicationIconImage:[NSImage imageNamed:@"app.icns"]];
            title = NSLocalizedString(@"Disable Filters", "");
        } else {
            [NSApp setApplicationIconImage:[self disabledIcon]];
            title = NSLocalizedString(@"Enable Filters", "");
        }
        NSMenuItem *item = [[[[NSApp mainMenu] itemWithTag:FILE_MENU_TAG] submenu]
            itemWithTag:DISABLE_FILTERS_TAG];
        [item setTitle:title];
        [[dockMenu itemWithTag:DOCK_DISABLE_FILTERS_TAG] setTitle:title];
    }
}

- (void)filterStateChange:(NSNotification*)note
{
    BOOL enabled = [[[note userInfo] objectForKey:PP_FILTER_CHANGE_ENABLED_KEY] boolValue];
    [self setFilterState:enabled];
}

- (void)applicationWillFinishLaunching:(NSNotification*)note
{
    LIST_EPOCH = [[NSDate dateWithTimeIntervalSince1970:(NSTimeInterval)1093597930] retain];
    
    NSString *loader, *logger, *agent;
    
    loader = [[NSBundle mainBundle] pathForResource:@"pploader" ofType:@"app"];
    logger = [[NSBundle mainBundle] pathForResource:@"pplogger" ofType:@"app"];
    agent = [[NSBundle mainBundle] pathForResource:@"pgagent" ofType:@"app"];
    
    NSWorkspace *ws = [NSWorkspace sharedWorkspace];
    // NSWorkspace/LaunchServices is pretty poor at detecting if background apps are running,
    // so we do it ourself
    if (0 == FindPIDByPathOrName(NULL, "pploader.app", ANY_USER))
        (void)[ws launchApplication:loader showIcon:NO autolaunch:YES];
    
#ifndef cpueater
    [[UKKQueue sharedQueue] setDelegate:self];
#endif
    [self openLogFile:nil];
    NSDistributedNotificationCenter *nc = [NSDistributedNotificationCenter defaultCenter];
    [nc addObserver:self selector:@selector(openLogFile:) name:PP_LOGFILE_CREATED object:nil];
    [nc addObserver:self selector:@selector(filterStateChange:) name:PP_FILTER_CHANGE object:nil];
    
    if (0 == FindPIDByPathOrName(NULL, "pplogger.app", ANY_USER))
        (void)[ws launchApplication:logger showIcon:NO autolaunch:YES];
    
    if (![ws isLoginItem:loader])
        [ws addLoginItem:loader hidden:NO];
    if (![ws isLoginItem:logger])
        [ws addLoginItem:logger hidden:NO];
}

- (void)syncWithLoaderState:(NSTimer*)timer
{
    if ([self loadProxy]) {
        BOOL enabled = [proxy isEnabled];
        [self setFilterState:enabled];
        enabled = [self automaticUpdate];
        [self setValue:[NSNumber numberWithBool:enabled] forKey:@"automaticUpdate"];
        enabled = [self autoAllowDNS];
        [self setAutoAllowDNS:enabled];
    } else
        NSBeep();
}

- (void)applicationDidFinishLaunching:(NSNotification*)note
{
    [self syncWithLoaderState:nil];
    // once more after a small delay, in case the server is busy during launch
    [self performSelector:@selector(syncWithLoaderState:) withObject:nil afterDelay:0.75];
    
    // Start update check
    [[NSNotificationCenter defaultCenter] addObserver:self
        selector:@selector(checkUpdateDidFinish:)
        name:BBNetUpdateDidFinishUpdateCheck
        object:nil];
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"BBNetUpdateDontAskConnect"])
        [BBNetUpdateVersionCheckController checkForNewVersion:@"PeerGuardian" interact:NO];
}

- (void)applicationWillTerminate:(NSNotification*)note
{
    [self killProxy];
}

- (NSMenu *)applicationDockMenu:(NSApplication *)sender
{
    if (!dockMenu) {
        // Build the menu
        dockMenu = [[NSMenu alloc] initWithTitle:@""];

        NSString *title;
        if (ppenabled)
            title = NSLocalizedString(@"Disable Filters", "");
        else
            title = NSLocalizedString(@"Enable Filters", "");

        // Add the items
        [[dockMenu addItemWithTitle:title action:@selector(enableDisable:)
            keyEquivalent:@""] setTarget:self];
        [[dockMenu itemWithTitle:title] setTag:DOCK_DISABLE_FILTERS_TAG];

        //[menu addItem:[NSMenuItem separatorItem]];
    }

    return (dockMenu);
}

- (BOOL)versionCheckInProgress
{
    return ([BBNetUpdateVersionCheckController isCheckInProgress]);
}

// This doesn't do anything, it's just here for bindings support
- (void)setVersionCheckInProgress:(BOOL)inprogress
{

}

- (void)checkUpdateDidFinish:(NSNotification*)note
{
    [self setValue:[NSNumber numberWithBool:NO] forKey:@"versionCheckInProgress"];
}

- (IBAction)checkForUpdate:(id)sender
{
    [self setValue:[NSNumber numberWithBool:YES] forKey:@"versionCheckInProgress"];
    [BBNetUpdateVersionCheckController checkForNewVersion:@"PeerGuardian" interact:YES];
}

- (IBAction)donate:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:
        [NSURL URLWithString:@"http://www.bergstrand.org/brian/donate"]];
}

- (IBAction)quitHelpers:(id)sender
{
    [self killProxy];
    [[NSDistributedNotificationCenter defaultCenter]
        postNotificationName:PP_HELPER_QUIT
        object:nil userInfo:nil];
}

- (IBAction)openLogWindow:(id)sender
{
    [logWindow makeKeyAndOrderFront:sender];
}

- (IBAction)openListWindow:(id)sender
{
    // prime the lists array
    if ([self loadProxy] && 0 == [[listsController content] count]) {
        NSArray *mylists = [proxy lists];
        if ([mylists count] > 0) {
            [listsController setSelectsInsertedObjects:NO];
            [listsController addObjects:mylists];
            [listsController setSelectsInsertedObjects:YES];
        } else {
            dbgbrk();
            NSBeep();
            return;
        }
    }
    
    [listWindow makeKeyAndOrderFront:sender];
}

- (IBAction)updateLists:(id)sender
{
    if (proxy)
        [proxy updateAllLists];
    else
        NSBeep();
}

- (IBAction)addList:(id)sender
{
    if (proxy) {
        NSMutableDictionary *newlist = [proxy createList];
        if (newlist) {
            [listsController addObject:newlist];
            [self editList:sender];
        }
    } else
        NSBeep();
}

- (IBAction)editList:(id)sender
{
    [preEditListCopy release];
    preEditListCopy = nil;
    
    NSDictionary *list = [[listsController selectedObjects] objectAtIndex:0];
    preEditListCopy = [list copy];
    
    NSMutableArray *ranges = nil;
    
    char buf[64];
    pg_table_id_string(PP_LIST_NEWID, buf, sizeof(buf), 1);
    NSString *newUUID = [NSString stringWithUTF8String:buf];
    
    if (NSOrderedSame != [newUUID caseInsensitiveCompare:[list valueForKey:@"UUID"]]) {
        NSURL *url = [self firstURL:list];
        if (url && [url isFileURL]) {
            ranges = [self addressSpecificationsFromFile:[url path]];
            // show error if nil?
        }
    } else
        ranges = [NSMutableArray array];
    
    [rangeController setContent:ranges];
    
    NSArray *urls = [[list valueForKey:@"URLs"] allKeys];
    [urlsController setContent:[NSMutableArray array]];
    if (urls)
        [urlsController addObjects:urls];
    
    [NSApp beginSheet:addEditSheet modalForWindow:listWindow modalDelegate:self
        didEndSelector:nil contextInfo:nil];
}

- (IBAction)removeList:(id)sender
{
    if (proxy) {
        NSArray *lists = [[listsController selectedObjects] copy];
        NSEnumerator *en = [lists objectEnumerator];
        NSDictionary *list;
        while ((list = [en nextObject]))
            [proxy removeList:list];
        [lists release];
    } else
        NSBeep();
}

- (IBAction)addRange:(id)sender
{
    NSDictionary *list = [[listsController selectedObjects] objectAtIndex:0];
    NSURL *url = [self firstURL:list];
    if (!url || ![url isFileURL]) {
        NSRunInformationalAlertPanel(
            NSLocalizedString(@"Not Supported", ""),
            NSLocalizedString(@"Addresses can only be added to local files. Click Add in the List Manager to create a new local file.", ""),
            @"OK", nil, nil);
        return;
    }
    
    [rangeController insert:self];
}

- (IBAction)removeRange:(id)sender
{
    NSArray *ranges = [[rangeController selectedObjects] copy];
    [rangeController removeObjects:ranges];
    [ranges release];
}

// It's a race between [showExportProgressPanel:] and [exportListsDidFinish:] -- this makes
// sure we have the right state.
static BOOL showExportProgressPanel = YES;

- (void)exportListsDidFinish:(NSError*)err
{
    if (err) {
        NSDictionary *args = [err userInfo];
        NSRunAlertPanel(
            NSLocalizedString(@"List Export Failed", ""),
            NSLocalizedString(@"Export to '%@' failed -- %@ (%d)", ""),
            @"OK", nil, nil, [[args objectForKey:@"output"] lastPathComponent],
            [err localizedDescription], [err code]);
    }
    showExportProgressPanel = NO;
    if (exportProgressWindow == [listWindow attachedSheet]) {
        [exportProgressBar stopAnimation:nil];
        [NSApp endSheet:exportProgressWindow];
        [exportProgressWindow close];
        
    }
}

- (void)exportListsThread:(NSDictionary*)args
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSArray *paths = [args objectForKey:@"paths"];
    int formatVersion = [[args objectForKey:@"format"] intValue];
    NSString *outputFile = [args objectForKey:@"output"];
    NSError *errObj;
    
    int err;
    int numPaths = [paths count];
    char **fsPaths = malloc(sizeof(char*) * (numPaths + 1));
    if (!fsPaths) {
        err = ENOMEM;
        goto exportListsThreadExit;
    }
    fsPaths[numPaths] = NULL;
    
    int i;
    for (i = 0; i < numPaths; i++)
        fsPaths[i] = (char*)[[paths objectAtIndex:i] fileSystemRepresentation];
    
    err = p2b_merge([outputFile fileSystemRepresentation], formatVersion, fsPaths);
    
    free(fsPaths);
    
exportListsThreadExit:
    if (0 == err)
        errObj = nil;
    else
        errObj = [NSError errorWithDomain:NSPOSIXErrorDomain code:err userInfo:args];
    [self performSelectorOnMainThread:@selector(exportListsDidFinish:) withObject:errObj waitUntilDone:NO];
    [pool release];
}

- (void)showExportProgressPanel:(id)obj
{
    static BOOL delay = YES;
    if (showExportProgressPanel && exportProgressWindow != [listWindow attachedSheet]) {
        if (delay) {
            delay = NO;
            // Wait until the next run-loop run so the previous sheet can actually detach
            [self performSelector:@selector(showExportProgressPanel:) withObject:nil afterDelay:0.0];
            return;
        }
        [exportProgressBar startAnimation:nil];
        [NSApp beginSheet:exportProgressWindow modalForWindow:listWindow modalDelegate:self
            didEndSelector:nil contextInfo:nil];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidEndSheetNotification
            object:listWindow];
    }
    delay = YES;
}

- (void)exportSelectedSavePanelDidEnd:(NSSavePanel*)panel returnCode:(int)ret contextInfo:(void*)contextInfo
{
    if (NSFileHandlingPanelOKButton == ret) {
        NSString *cacheRoot = [proxy objectForKey:@"CacheRoot"];
        
        NSArray *lists = [listsController selectedObjects];
        NSEnumerator *en = [lists objectEnumerator];
        NSMutableArray *paths = [NSMutableArray arrayWithCapacity:[lists count]];
        NSDictionary *list;
        
        while ((list = [en nextObject])) {
            NSURL *url = [self firstURL:list];
            if (url) {
                NSString *path = nil;
                if (NO == [url isFileURL]) {
                    NSString *cacheFile = [list valueForKey:@"CacheFile"];
                    if (cacheRoot && cacheFile && [cacheRoot length] && [cacheFile length])
                        path = [cacheRoot stringByAppendingPathComponent:cacheFile];
                } else {
                    path = [url path];
                }
                
                if (path && [[NSFileManager defaultManager] fileExistsAtPath:path])
                    [paths addObject:path];
                else
                    NSLog(@"PeerGuardian: cannot find file for list '%@', skipping export\n",
                        [list valueForKey:@"Description"]);
            }
        }
        
        int formatVersion = [[savePanelFormatMenu selectedItem] tag];
        NSString *outputFile = [panel filename];
        NSDictionary *args = [NSDictionary dictionaryWithObjectsAndKeys:
            outputFile, @"output",
            paths, @"paths",
            [NSNumber numberWithInt:formatVersion], @"format",
            nil];
        if ([paths count]) {
            // Delay until the Save sheet ends.
            [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(showExportProgressPanel:)
                name:NSWindowDidEndSheetNotification object:listWindow];
            showExportProgressPanel = YES;
            [exportProgressText setStringValue:
                [NSLocalizedString(@"Exporting to", "") stringByAppendingFormat:
                    @" '%@'", outputFile]];
            [NSThread detachNewThreadSelector:@selector(exportListsThread:)
                toTarget:self withObject:args];
        } else
            [self exportListsDidFinish:[NSError errorWithDomain:NSPOSIXErrorDomain
                code:ENOENT userInfo:args]];
    }
}

- (IBAction)exportSelectedLists:(id)sender
{
    if ([listWindow attachedSheet]) {
        NSBeep();
        return;
    }
    
    NSSavePanel *panel = [NSSavePanel savePanel];
    [panel setCanSelectHiddenExtension:YES];
    [panel setExtensionHidden:NO];
    [panel setAllowedFileTypes:[NSArray arrayWithObjects:@"p2b", @"p2p", nil]];
    [panel setAllowsOtherFileTypes:NO];
    [panel setAccessoryView:savePanelAccessoryView];
    
    [panel beginSheetForDirectory:nil file:NSLocalizedString(@"MyList","") modalForWindow:listWindow
        modalDelegate:self
        didEndSelector:@selector(exportSelectedSavePanelDidEnd:returnCode:contextInfo:)
        contextInfo:nil];
}

- (IBAction)addEditDone:(id)sender
{
    (void)[listsController commitEditing];
    NSMutableDictionary *list = [[listsController selectedObjects] objectAtIndex:0];
    if (proxy && list) {
        char buf[64];
        pg_table_id_string(PP_LIST_NEWID, buf, sizeof(buf), 1);
        NSString *newUUID = [NSString stringWithUTF8String:buf];
        
        if (ADD_EDIT_OK == [sender tag]) {
            BOOL good = YES;
            
            (void)[urlsController commitEditing];
            // this is necessary because the combo box does not actually commit its text field contents with [commitEditing]
            (void)[addEditSheet makeFirstResponder:nil];
            [self updateURLs:list];
            
            (void)[rangeController commitEditing];
            NSURL *url = [self firstURL:list];
            if (url && [url isFileURL]) {
                int err;
                if ((err = [self commitAddressSpecifications:[rangeController content] toFile:[url path]])) {
                    NSString *msg;
                    if (EINVAL == err)
                        msg = NSLocalizedString(@"Please check that all entries have a description and properly formatted IP address. (%d)", "");
                    else
                        msg = NSLocalizedString(@"An error occured while trying to create or modify the file. (%d)", "");
                    NSRunCriticalAlertPanel(
                        NSLocalizedString(@"Could Not Save File", ""),
                        [NSString stringWithFormat:msg, err],
                        @"OK", nil, nil);
                    return;
                }
            }
            
            if (NSOrderedSame == [newUUID caseInsensitiveCompare:[list valueForKey:@"UUID"]])
                good = [proxy addList:list];
            else if (preEditListCopy && ![preEditListCopy isEqualTo:list]) {
                // This list has actually changed, let the loader know
                good = [proxy changeList:list];
            }
            
            if (!good) {
                NSRunCriticalAlertPanel(
                    NSLocalizedString(@"Could Not Save List", ""),
                    NSLocalizedString(@"Please check the entered values, in particular the URL must be valid.", ""),
                    @"OK", nil, nil);
                return;
            }
            
        } else if (NSOrderedSame == [newUUID caseInsensitiveCompare:[list valueForKey:@"UUID"]]) {
            [listsController removeObject:list];
        }
    } else
        NSBeep();
    
    [preEditListCopy release];
    preEditListCopy = nil;
    
    [NSApp endSheet:addEditSheet];
    [addEditSheet close];
    
    [rangeController setContent:nil];
}

- (IBAction)addEditChooseFile:(id)sender
{
    NSSavePanel *panel = [NSSavePanel savePanel];
    [panel setCanSelectHiddenExtension:YES];
    [panel setExtensionHidden:NO];
#if 0
    [panel setRequiredFileType:@"p2b"];
    [panel setMessage:
        NSLocalizedString(@"You can import an existing file by selecting it in the browser (even though it will appear not selectable) and then confirming the replacement. All ranges will be preserved.", "")];
#endif
    [panel setAllowedFileTypes:[NSArray arrayWithObjects:@"p2b", @"p2p", @"txt", nil]];
    int ret = [panel runModalForDirectory:nil file:NSLocalizedString(@"MyList","")];
    if (NSFileHandlingPanelOKButton == ret) {
        NSString *path = [panel filename];
        if ([[NSFileManager defaultManager] fileExistsAtPath:path]) {
            // load the ranges if possible
            NSMutableArray *ranges = [self addressSpecificationsFromFile:path];
            if (ranges)
                [rangeController setContent:ranges];
            else {
                ret = NSRunCriticalAlertPanel(
                    NSLocalizedString(@"Invalid File Format", ""),
                    NSLocalizedString(@"The file '%@' does not appear to be in a recognized file format. Replace and overwrite the contents of '%@'?", ""),
                    NSLocalizedString(@"Replace", ""), NSLocalizedString(@"Cancel", ""), nil,
                    [path lastPathComponent], [path lastPathComponent], nil);
                if (NSAlertDefaultReturn != ret)
                    return;
            }
        }
    
        // We always save in p2b format, so make sure the extension is correct
        NSString *oldpath = path;
        path = [[path stringByDeletingPathExtension] stringByAppendingPathExtension:@"p2b"];
        if (![oldpath isEqualToString:path] && [[NSFileManager defaultManager] fileExistsAtPath:path]) {
            NSString *newfile = [path lastPathComponent];
            ret = NSRunCriticalAlertPanel(
                    NSLocalizedString(@"Replace File?", ""),
                    NSLocalizedString(@"The file '%@' needs to be saved as '%@' which already exists. Replace and overwrite the contents of '%@'?", ""),
                    NSLocalizedString(@"Replace", ""), NSLocalizedString(@"Cancel", ""), nil,
                    [oldpath lastPathComponent], newfile, newfile, nil);
            if (NSAlertDefaultReturn != ret)
                    return;
        }
        // Everthing expects URL as a string
        path = [[NSURL fileURLWithPath:path] absoluteString];
        // Mulitple values are not supported for file based lists
        [urlsController setContent:[NSMutableArray array]];
        [self setSelectedURL:path];
    }
}

- (void)updateListController
{
    if (proxy) {
        NSMutableArray *mylists = nil;
        @try {
            if (nil == [listWindow attachedSheet]) {
                mylists = [[proxy lists] mutableCopy];
                [listsController setContent:mylists];
            } else {
                [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(delayedUpdateListController:)
                    name:NSWindowDidEndSheetNotification object:listWindow];
                return;
            }
        } @catch (NSException *e) {
            NSLog(@"[PeerGuardian updateListController] - Exception '%@'\n", e);
        }
        [mylists release];
        [self setMsg:
            NSLocalizedString(@"One or more lists have been updated.", "")];
    }
}

- (void)delayedUpdateListController:(NSNotification*)note
{
    [self performSelector:@selector(updateListController) withObject:nil afterDelay:0.0];
}

- (IBAction)openPrefsWindow:(id)sender
{
    [self setValue:nil forKey:@"portRules"];
    [prefsWindow makeKeyAndOrderFront:sender];
}

- (IBAction)openLookupWindow:(id)sender
{
    static id win = nil;
    if (!win)
        win = [[PGLookupController alloc] initWithDelegate:self];
    
    [win showWindow:sender];
}

- (IBAction)openStats:(id)sender
{
    if (0 == FindPIDByPathOrName(NULL, "pgagent.app", ANY_USER)) {
        NSString *agent = [[NSBundle mainBundle] pathForResource:@"pgagent" ofType:@"app"];
        NSWorkspace *ws = [NSWorkspace sharedWorkspace];
        (void)[ws launchApplication:agent showIcon:NO autolaunch:YES];
        usleep(320000);
        
        if (![[NSUserDefaults standardUserDefaults] boolForKey:@"PGAgent Added to Login"]) {
            if (![ws isLoginItem:agent])
                [ws addLoginItem:agent hidden:NO];
            [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"PGAgent Added to Login"];
        }
    }
    
    [[NSDistributedNotificationCenter defaultCenter] postNotificationName:PP_SHOW_STATS object:nil];
}

- (IBAction)savePrefs:(id)sender
{
    [prefsWindow endEditingFor:nil];
    
    if (!portRulesCache || 0 == [portRulesCache length]) {
        [prefsWindow performSelector:@selector(performClose:) withObject:self afterDelay:0.0];
        return;
    }
    NSString *msg;
    if (proxy) {
        int err = [proxy setPortRule:portRulesCache];
        if (!err) {
            [self setValue:nil forKey:@"portRules"];
            [prefsWindow performSelector:@selector(performClose:) withObject:self afterDelay:0.0];
            return;
        } else {
            msg = [NSString stringWithFormat:@"%@ (%d)",
                NSLocalizedString(@"The port rule may be incorrectly formatted. Please verify your entries.", ""), err];
        }
    } else
        msg = NSLocalizedString(@"Internal exception. Try again, or quit and restart.", "");
    
    NSBeginAlertSheet(NSLocalizedString(@"Preferences Could Not Be Saved", ""),
        @"OK", nil, nil, prefsWindow,
        nil, nil, nil, nil,
        msg);
}

- (IBAction)showHelp:(id)sender
{
    NSString *path;
    
    if (LICENSE_TAG != [sender tag])
        path = [[NSBundle mainBundle] pathForResource:@"PeerGuardianUserGuide" ofType:@"pdf"];
    else
        path = [[NSBundle mainBundle] pathForResource:@"LICENSE" ofType:@"html"];
    if (path)
        [[NSWorkspace sharedWorkspace] openFile:path];
    else
        [NSApp showHelp:sender];
}

- (IBAction)orderFrontStandardAboutPanel:(id)sender
{
    static NSString *packageVersion = nil;
    if (!packageVersion) {
        NSBundle *kextBundle = [NSBundle bundleWithPath:PP_KEXT_PATH];
        NSString *path = [kextBundle pathForResource:@"pgversion" ofType:nil];
        if (path) {
            packageVersion = [[NSString alloc] initWithContentsOfFile:path
                encoding:NSASCIIStringEncoding error:nil];
        } else
           packageVersion = @"";
    }
    NSDictionary *options = [NSDictionary dictionaryWithObject:packageVersion forKey:@"ApplicationVersion"];
    [NSApp orderFrontStandardAboutPanelWithOptions:options];
}

// accessors for Prefs Window binding
- (NSString*)portRules
{
    if (portRulesCache)
        return (portRulesCache);
    if (proxy) {
        portRulesCache = [proxy objectForKey:PP_PREF_PORT_RULES];
        if (portRulesCache)
            return ([portRulesCache retain]);
    }
    return (@"");
}

- (void)setPortRules:(NSString*)rule
{
    [portRulesCache release];
    portRulesCache = [rule retain];
}

// DOClientExtensions
- (oneway void)listWillUpdate:(id)list
{
    NSString *msg = [NSString stringWithFormat:
        NSLocalizedString(@"Checking %@ for an update...", "list update"), [list objectForKey:@"Description"]];
    [self setMsg:msg];
}

- (oneway void)listDidUpdate:(id)list
{
    NSString *msg = [NSString stringWithFormat:
        NSLocalizedString(@"%@ updated", "list update"), [list objectForKey:@"Description"]];
    [self setMsg:msg];
    [self performSelector:@selector(updateListController) withObject:nil afterDelay:0.0];
}

- (oneway void)listFailedUpdate:(id)list error:(NSDictionary*)error
{
    NSString *errString;
    @try {
        if (ERANGE != [[error objectForKey:@"code"] intValue])
            errString = [error objectForKey:NSLocalizedDescriptionKey];
        else
            errString = NSLocalizedString(@"List is empty.", "");
    } @catch (NSException *exception) {
        errString = [error description];
    }
    
    NSString *msg = [NSString stringWithFormat:
        NSLocalizedString(@"%@ failed to update: '%@'", "list update failed with error"),
            [(NSDictionary*)list objectForKey:@"Description"], errString];
    [self setMsg:msg];
}

- (oneway void)listDidChange:(id)list
{
    [self performSelector:@selector(updateListController) withObject:nil afterDelay:0.0];
}

- (oneway void)listWasRemoved:(id)list
{
    [self performSelector:@selector(updateListController) withObject:nil afterDelay:0.0];
}

@end

int main(int argc, char *argv[]) {
    pgsbinit();
    return (NSApplicationMain(argc, (const char**)argv));
}
