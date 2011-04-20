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
#import "PPAddrActionAskController.h"
#import "pplib.h"

@interface NSObject (PPAddrActionAskControllerDelgate)
- (int)addTempAllowAddress:(NSDictionary*)spec;
@end

@implementation PPAddrActionAskController

- (PPAddrActionAskController*)initWithAddressSpecification:(NSDictionary*)spec
    delegate:(id)delegate
{
    self = [super initWithWindowNibName:@"AddrAskAction"];
    [super setWindowFrameAutosaveName:@"Address Action"];
    addrSpec = [spec mutableCopy];
    myDelegate = delegate;
    return (self);
}

- (void)windowDidLoad
{
    [appIcon setImage:[NSImage imageNamed:@"app.icns"]];
    
    NSString *label = [addrSpec objectForKey:PP_ADDR_SPEC_START];
    if ([addrSpec objectForKey:PP_ADDR_SPEC_NAME])
        label = [label stringByAppendingFormat:@" (%@)", [addrSpec objectForKey:PP_ADDR_SPEC_NAME]];
    [labelText setStringValue:label];
    NSWindow *w = [self window];
    [w setTitle:NSLocalizedString(@"PeerGuardian Temporary Address Action", "")];
    [w center];
}

- (IBAction)peformAction:(id)sender
{
    unsigned expireSecs = (unsigned)[[expireTimePopup selectedItem] tag];
    [addrSpec setObject:[NSNumber numberWithUnsignedInt:expireSecs] forKey:PP_ADDR_SPEC_EXPIRE];
    int err = [myDelegate addTempAllowAddress:addrSpec];
    NSWindow *w = [self window];
    if (err) {
        NSString *title = NSLocalizedString(@"PeerGuardian: Add Address Failed", "");
        NSString *msg;
        if (ENOENT == err)
            msg = NSLocalizedString(@"The address could not be added, because an active permanent allow list could not be found. Please create an allow list and try again.", "");
        else {
            msg = NSLocalizedString(@"The address could not be added because an unknown error occurred", "");
            msg = [msg stringByAppendingFormat:@": %d.", err];
        }
        NSBeginAlertSheet(title, @"OK", nil, nil, w, nil, nil, nil, nil, msg);
        return;
    }
    [w performClose:sender];
}

- (BOOL)windowShouldClose:(id)sender
{
    return (YES);
}

- (void)windowWillClose:(NSNotification *)notification
{
    [self autorelease];
}

- (void)dealloc
{
    [addrSpec release];
    [super dealloc];
}

@end
