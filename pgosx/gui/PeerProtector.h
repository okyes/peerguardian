/*
    Copyright 2005-2007 Brian Bergstrand

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
#import "pploader.h"

@interface PeerProtector : NSObject <PPLoaderDOClientExtensions> {
    IBOutlet NSWindow *listWindow;
    IBOutlet NSWindow *logWindow;
    IBOutlet NSWindow *addEditSheet;
    IBOutlet NSWindow *prefsWindow;
    
    IBOutlet NSTableView *listTable;
    IBOutlet NSTextField *listMessage;
    IBOutlet NSArrayController *listsController;
    
    IBOutlet NSView *savePanelAccessoryView;
    IBOutlet NSPopUpButton *savePanelFormatMenu;
    
    IBOutlet NSTextView *logText;
    
    IBOutlet NSArrayController *rangeController;
    IBOutlet NSArrayController *urlsController;
    
    IBOutlet NSWindow *exportProgressWindow;
    IBOutlet NSTextField *exportProgressText;
    IBOutlet NSProgressIndicator *exportProgressBar;

    BOOL ppenabled, autoUpdate;
    NSDictionary *preEditListCopy;
    NSFileHandle *logFile;
    NSTimer *clearMsgTimer;
    NSTimer *statsTimer;
    NSDictionary *blockedLogAttrs;
    NSString *portRulesCache, *currentlySelectedURL;
}

- (IBAction)checkForUpdate:(id)sender;
- (IBAction)donate:(id)sender;

- (IBAction)openLogWindow:(id)sender;
- (IBAction)openListWindow:(id)sender;
- (IBAction)openPrefsWindow:(id)sender;
- (IBAction)openLookupWindow:(id)sender;
- (IBAction)openStats:(id)sender;

- (IBAction)updateLists:(id)sender;
- (IBAction)addList:(id)sender;
- (IBAction)editList:(id)sender;
- (IBAction)removeList:(id)sender;
- (IBAction)exportSelectedLists:(id)sender;

- (IBAction)addRange:(id)sender;
- (IBAction)removeRange:(id)sender;
- (IBAction)addEditDone:(id)sender;
- (IBAction)addEditChooseFile:(id)sender;
- (IBAction)addURL:(id)sender;
- (IBAction)removeURL:(id)sender;

- (IBAction)savePrefs:(id)sender;
- (IBAction)showHelp:(id)sender;
- (IBAction)orderFrontStandardAboutPanel:(id)sender;

- (void)setMsg:(NSString*)string;

@end
