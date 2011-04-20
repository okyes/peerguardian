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
#import <Security/Security.h>
#import <SecurityFoundation/SFAuthorization.h>

#import "UKFileWatcher.h"
#import "UKKQueue.h"

#import "PPKTool.h"
#import "ppmsg.h"
#import "pploader.h"

NSTimer *writeTimer = nil;
@implementation PPKTool : NSObject

- (void)kextDidLoad:(NSNotification*)note
{
    pptrace(@"PPKTool kextDidLoad:\n");
}

- (void)kextDidUnload:(NSNotification*)note
{
    pptrace(@"PPKTool kextDidUnload:\n");
    if (wantsLoad) {
        [self performSelector:@selector(load) withObject:nil afterDelay:.50];
        wantsLoad = NO;
    }
}

- (void)taskDidTerminate:(NSNotification*)note
{
    pptrace(@"taskDidTerminate: %d\n", [[note object] processIdentifier]);
    [[NSNotificationCenter defaultCenter] removeObserver:self
        name:NSTaskDidTerminateNotification object:[note object]];
    [[note object] autorelease];
}

- (void)runAuthorizedToolWithArgs:(NSArray *)args
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    SFAuthorization *authRef = [SFAuthorization authorization];
    
    AuthorizationItem rights[] = { 
        { kAuthorizationRightExecute, strlen(PPKTOOL_PATH), PPKTOOL_PATH, 0} };
    AuthorizationRights rightSet = { 1, rights };
    
    AuthorizationFlags flags = kAuthorizationFlagDefaults | kAuthorizationFlagPreAuthorize
         | kAuthorizationFlagInteractionAllowed | kAuthorizationFlagExtendRights;
    
    AuthorizationItem prompt = { kAuthorizationEnvironmentPrompt, 0, NULL, 0 };
    
    NSString *promptString = NSLocalizedString(@"Load/Unload PeerGuardian Kernel Extension", "");
    NSString *promptString2 = NSLocalizedString(@"After the extension loads, relaunch your P2P applications, so their connections will be filtered.", "");
    promptString = [NSString stringWithFormat:@"%@\n\n%@", promptString, promptString2];
    const char *promptUTF8String = [promptString UTF8String];
    prompt.value = (void*)promptUTF8String;
    prompt.valueLength = strlen(promptUTF8String);
    
    AuthorizationEnvironment envSet = { 1, &prompt };
    
    //request rights
    OSStatus status = [authRef permitWithRights:&rightSet flags:flags
        environment:&envSet authorizedRights:NULL];
    if (status) {
        NSLog(@"Failed to get admin execute rights: %u\n", status);
        goto authtool_exit;// return (EACCES);
    }
    
    int err = 0;
    char **cargs = calloc(sizeof(char*), ([args count] + 1));
    if (cargs) {
        NSEnumerator *en = [args objectEnumerator];
        NSString *arg;
        int i = 0;
        while ((arg = [en nextObject])) {
            cargs[i] = (char*)[[arg description] cStringUsingEncoding:NSASCIIStringEncoding];
            if (!cargs[i]) {
                err = EINVAL;
                break;
            }
        }
        if (!err) {
            status = AuthorizationExecuteWithPrivileges([authRef authorizationRef],
                PPKTOOL_PATH, kAuthorizationFlagDefaults, cargs, NULL);
            if (status) {
                err = EACCES;
                NSLog(@"AuthorizationExecuteWithPrivileges failed: %u\n", status);
            }
            wait(NULL); // Kill the Zombies!
        }
        free(cargs);
    } else
        err = ENOMEM;
    
authtool_exit:
    [pool release];
    //return (err);
}

- (int)runToolWithArgs:(NSArray *)args authorize:(BOOL)auth wait:(BOOL)wait
{
    if (auth) {
        [NSThread detachNewThreadSelector:@selector(runAuthorizedToolWithArgs:) toTarget:self withObject:args];
        return (0);
    }
    
    NSTask *t = [[NSTask alloc] init];
    [t setLaunchPath:[NSString stringWithCString:PPKTOOL_PATH encoding:NSASCIIStringEncoding]];
    [t setArguments:args];
    [t setStandardOutput:[NSFileHandle fileHandleWithNullDevice]];
    
    int err = 0;
    @try {
        if (!wait) {
            [[NSNotificationCenter defaultCenter] addObserver:self 
                selector:@selector(taskDidTerminate:) 
                name:NSTaskDidTerminateNotification 
                object:t];
        }
        [t launch];
        if (wait) {
            [t waitUntilExit];
            err = [t terminationStatus];
        }
    } @catch (NSException *exception) {
        err = EINVAL;
        NSLog(@"%@ failed launch with: %@ - '%@'\n", [[t launchPath] lastPathComponent],
            [exception name], [exception reason]);
        if (!wait) {
            [[NSNotificationCenter defaultCenter] removeObserver:self
                name:NSTaskDidTerminateNotification object:t];
            wait = 1;
        }
    }
    if (wait)
        [t release];
    return (err);
}

- (BOOL)isLoaded
{
    if (0 == [self runToolWithArgs:[NSArray arrayWithObject:@"-i"] authorize:NO wait:YES])
        return (YES);
    
    return (NO);
}

- (void)load
{
    pptrace(@"PPKTool load\n");
    (void)[self runToolWithArgs:[NSArray arrayWithObject:@"-l"] authorize:YES wait:NO];
}

- (void)unload
{
    pptrace(@"PPKTool unload\n");
    (void)[self runToolWithArgs:[NSArray arrayWithObject:@"-u"] authorize:YES wait:NO];
}

- (id)init
{
    NSDistributedNotificationCenter *nc = [NSDistributedNotificationCenter defaultCenter];
    [nc addObserver:self selector:@selector(kextDidLoad:) name:PP_KEXT_DID_LOAD object:nil];
    [nc addObserver:self selector:@selector(kextDidUnload:) name:PP_KEXT_DID_UNLOAD object:nil];
    
    return (self);
}

- (void)dealloc
{
    [super dealloc];
}

@end
