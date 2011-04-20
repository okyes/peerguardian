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

static NSDistantObject<PPLoaderDOServerExtensions> *proxy = nil;
static unsigned int clientid = 0;

#ifndef PGProxySupportClass
#error Must define PGProxySupportClass
#endif

@interface PGProxySupportClass (PGProxySupportExtensions)

- (void)connectionDied:(NSNotification*)note;
- (void)killProxy;
- (NSDistantObject*)loadProxy;

@end

@interface NSObject (PGProxyDelegate)

- (void)proxyDied;
- (void)proxyAttached;

@end

@implementation PGProxySupportClass (PGProxySupportExtensions)

- (void)connectionDied:(NSNotification*)note
{
    [[NSNotificationCenter defaultCenter] removeObserver:self
        name:NSConnectionDidDieNotification object:[note object]];
    
    @try {
        if (proxy) [proxy release];
    } @catch (NSException *e) {}
    proxy = nil;
    
    if ([self respondsToSelector:@selector(proxyDied)]) {
        @try {
            [self proxyDied];
        } @catch (NSException *e) {}
    }
    
    #ifndef PGPROXY_DONT_LAUNCH
    if (0 == FindPIDByPathOrName(NULL, "pploader.app", ANY_USER)) {
        // not good, the loader may have crashed
        NSString *loader = [[NSBundle mainBundle] pathForResource:@"pploader" ofType:@"app"];
        NSWorkspace *ws = [NSWorkspace sharedWorkspace];
        (void)[ws launchApplication:loader showIcon:NO autolaunch:YES];
    }
    #endif
    [self performSelector:@selector(loadProxy) withObject:nil afterDelay:.50];
}

- (void)killProxy
{
    if (proxy) {
        // Make sure we don't try to start another proxy
        @try {
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:NSConnectionDidDieNotification object:[proxy connectionForProxy]];
            if (-1 != clientid)
                [proxy deregisterClient:clientid];
        } @catch (NSException *exception) {
        }
        [proxy release];
        proxy = nil;
    }
}

- (NSDistantObject*)loadProxy
{
    static BOOL setup = YES;
    
    if (setup) {
        [[NSConnection defaultConnection] setRootObject:
            [NSProtocolChecker protocolCheckerWithTarget:self protocol:
                @protocol(PPLoaderDOClientExtensions)]];
        [[NSConnection defaultConnection] setReplyTimeout:5.0];
        [[NSConnection defaultConnection] setRequestTimeout:5.0];
        setup = NO;
    }
    
    if (!proxy) {    
        proxy = [[NSConnection rootProxyForConnectionWithRegisteredName:
            [NSString stringWithFormat:@"%s+%ul", PP_CTL_NAME, getuid()]
                host:nil] retain];
        if (!proxy) {
            [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(loadProxy) object:nil];
            [self performSelector:@selector(loadProxy) withObject:nil afterDelay:1.0];
            return (nil);
        }
        
        [[NSNotificationCenter defaultCenter] addObserver:self
            selector:@selector(connectionDied:) name:NSConnectionDidDieNotification
            object:[proxy connectionForProxy]];
        
        [proxy setProtocolForProxy:@protocol(PPLoaderDOClientExtensions)];
        [[proxy connectionForProxy] setReplyTimeout:5.0];
        [[proxy connectionForProxy] setRequestTimeout:5.0];
        
        clientid = [proxy registerClient:[[NSConnection defaultConnection] rootObject]];
        
        if ([self respondsToSelector:@selector(proxyAttached)]) {
            @try {
                [self proxyAttached];
            } @catch (NSException *e) {}
        }
    }
    
    return (proxy);
}

- (IBAction)enableDisable:(id)sender
{
    if (ppenabled) {
        int ret = NSRunInformationalAlertPanel(
            NSLocalizedString(@"Disable PeerGuardian?", ""),
            NSLocalizedString(@"Disabling PeerGuardian will allow all incoming and outgoing connections.", ""),
            @"OK", NSLocalizedString(@"Cancel", ""), nil);
        if (NSAlertDefaultReturn == ret) {
            NSString *title, *msg;
            ret = EINVAL;
            if (proxy && 0 == (ret = [proxy disable])) {
                title = NSLocalizedString(@"PeerGuardian Disabled", "");
                msg = NSLocalizedString(@"All connections are now being allowed.", "");
            } else {
                title = NSLocalizedString(@"PeerGuardian Could Not Be Disabled", "");
                NSError *error = [NSError errorWithDomain:NSPOSIXErrorDomain code:ret userInfo:nil];
                msg = [error localizedDescription];
                ppassert(msg != nil);
            }
            (void)NSRunInformationalAlertPanel(title, msg, @"OK", nil, nil);
        }
    } else {
        if (proxy) {
            [proxy enable];
        } else {
            (void)NSRunInformationalAlertPanel(
            NSLocalizedString(@"PeerGuardian Could Not Be Enabled", ""),
            NSLocalizedString(@"A connection to the helper tool is not available.", ""),
            @"OK", nil, nil);
        }
    }
}

@end
