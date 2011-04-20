/*
    Copyright 2008 Brian Bergstrand

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
#import <Security/Security.h>
#import <SecurityFoundation/SFAuthorization.h>

#import "ppmsg.h"
#import "pgsb.h"
#import "pploader.h"

#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5
#import "../gui/LoginItemsAE/LoginItemsAE.c"
#endif

@interface PGUninstall : NSObject {
    NSAttributedString *toolOutput;
}

@end

#define SetOutput(os) \
[self setValue:[[[NSAttributedString alloc] initWithString:(os)] autorelease] forKey:@"toolOutput"]

@implementation PGUninstall

- (BOOL)removePGLoginItems
{
    OSStatus err;
    LSSharedFileListRef list;
    if (NULL != LSSharedFileListCreate) {
        if (!(list = LSSharedFileListCreate(kCFAllocatorDefault, kLSSharedFileListSessionLoginItems, NULL)))
            return (NO);
    } else
        list = NULL;
    if (list) {
        UInt32 seed;
        CFArrayRef items = LSSharedFileListCopySnapshot(list, &seed);
        if (items) {
            LSSharedFileListItemRef item;
            CFIndex count = CFArrayGetCount(items);
            NSURL *url;
            NSString *itemPath;
            for (CFIndex i = 0; i < count; ++i) {
                item = (LSSharedFileListItemRef)CFArrayGetValueAtIndex(items, i);
                if (0 == (err = LSSharedFileListItemResolve(item, kLSSharedFileListNoUserInteraction|kLSSharedFileListDoNotMountVolumes,
                    (CFURLRef*)&url, NULL))) {
                    itemPath = [url path];
                    if (NSNotFound != [itemPath rangeOfString:@"PeerGuardian.app" options:NSCaseInsensitiveSearch].location) {
                        err = LSSharedFileListItemRemove(list, item);
                     }
                }
            }
            CFRelease(items);
        }
        CFRelease(list);
        
        return (YES);
    }

    #if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5
    NSArray *items = nil;
    // XXX: this fails completely if there are any orphan items in the list
    if (0 != (err = LIAECopyLoginItems((CFArrayRef*)&items)) || !items) {
        NSBeep();
        NSLog(@"Failed to copy login items (%d)\n", err);
        return (NO);
    }
    
    [items autorelease];
    
    id	obj;
    NSString *path;
    NSEnumerator *itemEnumerator = [items objectEnumerator];
    while ((obj = [itemEnumerator nextObject])) {
        path = [[obj objectForKey:(NSString*)kLIAEURL] path];
        if (NSNotFound != [path rangeOfString:@"PeerGuardian.app" options:NSCaseInsensitiveSearch].location) {
            err = LIAERemove([items indexOfObject:obj]);
        }
    }
    return (YES);
    #endif
}

- (void)quitProcess:(ProcessSerialNumber*)psn
{
    NSAppleEventDescriptor *event, *targetAddress;

    pid_t pid = -1;
    (void)GetProcessPID(psn, &pid);
    
    targetAddress = [NSAppleEventDescriptor descriptorWithDescriptorType:typeKernelProcessID
        bytes:&pid length:sizeof(pid)];
    event = [[NSAppleEventDescriptor alloc] initWithEventClass:kCoreEventClass
        eventID:kAEQuitApplication
        targetDescriptor:targetAddress
        returnID:kAutoGenerateReturnID
        transactionID:kAnyTransactionID];
    
    AEDesc reply;
    if (event && 0 == AESendMessage([event aeDesc], &reply, kAENoReply, 0)) {
        usleep(320000);
    }
    [event release];
    
    // if the send fails...
    (void)kill(pid, SIGTERM);
}

- (void)quitPGApps
{
    ProcessSerialNumber psn = { kNoProcess, kNoProcess };
    while (GetNextProcess(&psn) == noErr) {
        CFDictionaryRef infoDict = ProcessInformationCopyDictionary(&psn, kProcessDictionaryIncludeAllInformationMask);
        if (!infoDict)
            continue;
        
        NSString *bundleid = (NSString*)CFDictionaryGetValue(infoDict, kCFBundleIdentifierKey);
        if (bundleid && [@"xxx.qnation.PeerProtectorApp" isEqualToString:bundleid]) {
            [self quitProcess:&psn];
            break;
        }
        CFRelease(infoDict);
    }

    [[NSDistributedNotificationCenter defaultCenter]
        postNotificationName:PP_HELPER_QUIT
        object:nil userInfo:nil];
    
    usleep(300000);
}

- (BOOL)runUninstaller
{
    NSMutableString *output = [[[toolOutput string] mutableCopy] autorelease];
    
    const char *toolPath = [[[NSBundle mainBundle] pathForResource:@"pguninstall" ofType:@"sh"] fileSystemRepresentation];
    if (!toolPath) {
        [output appendString:@"Failed to find tool path!\n"];
        SetOutput(output);
        NSBeep();
        return (NO);
    }
	
	AuthorizationItem right = {kAuthorizationRightExecute, strlen(toolPath), (char*)toolPath, 0};
	AuthorizationRights rightSet = {1, &right };
	AuthorizationFlags flags = kAuthorizationFlagDefaults | kAuthorizationFlagPreAuthorize 
		| kAuthorizationFlagInteractionAllowed | kAuthorizationFlagExtendRights;
	
    AuthorizationItem prompt = {kAuthorizationEnvironmentPrompt, 0, NULL, 0};
    prompt.value = (void*)[NSLocalizedString(@"Uninstall Peer Guardian\n", "") UTF8String];
    prompt.valueLength = strlen((char*)prompt.value);
    
    AuthorizationEnvironment envSet = {1, &prompt};
    AuthorizationRef authorizationRef = NULL;
	OSStatus status = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults, &authorizationRef);
	if (errAuthorizationSuccess != status) {
        [output appendFormat:@"Failed to create authref: %d.\n", status];
        SetOutput(output);
		return (NO);
	}
	if (0 != (status = AuthorizationCopyRights(authorizationRef, &rightSet, &envSet, flags, NULL))) {
        [output appendFormat:@"Failed to obtain admin execute rights: %d\n", status];
        SetOutput(output);
        return (NO);
    }
    
    FILE *pipe = NULL;
    status = AuthorizationExecuteWithPrivileges(authorizationRef, toolPath, kAuthorizationFlagDefaults, NULL, &pipe);
    AuthorizationFree(authorizationRef, kAuthorizationFlagDestroyRights);
    if (0 != status) {
        [output appendFormat:@"AuthorizationExecuteWithPrivileges failed: %d\n", status];
        SetOutput(output);
        return (NO);
    }
    
    char buf[4096];
    while(NULL != fgets(buf, sizeof(buf), pipe)) {
        // XXX: ouput is actually ASCII, but ASCII is a subset of UTF8
        NSString *s = [NSString stringWithUTF8String:buf];
        if (s) {
            [output appendString:s];
            SetOutput(output);
        }
    }
    
    fclose(pipe);
    wait(NULL); // Kill the Zombies!
    return (YES);
}

- (IBAction)pgUninstall:(id)sender
{
    SetOutput(@"");
    [sender setEnabled:NO];
    
    [self quitPGApps];
    
    NSMutableString *output = [NSMutableString string];
    // this must be done before removing the files on disk or we'll leave orphan items in the login list
    [output appendString:@"Removing login items..."];
    if (YES == [self removePGLoginItems])
        [output appendString:@"\n"];
    else
        [output appendString:@" failed!\n"];
    SetOutput(output);
    
    if (YES == [self runUninstaller]) {
        [sender setEnabled:YES];
    }
}

- (id)init
{
    self = [super init];
    toolOutput = [[NSAttributedString alloc] initWithString:
        NSLocalizedString(@"This will remove all PeerGuardian compononents, including preference files.", "")];
    return (self);
}

@end

int main (int argc, const char *argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    // This breaks AuthorizationExecuteWithPrivileges for some reason; even just loading an "(allow default)" rule.
    #ifdef breaks_AuthServices
    pgsbinit();
    #endif
    
    PGUninstall *pgu = [[PGUninstall alloc] init];

    [[NSApplication sharedApplication] setDelegate:pgu];
    
    [pool release];
    
    return (NSApplicationMain(argc, argv));
}
