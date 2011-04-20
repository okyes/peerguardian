/*
    Copyright 2006 Brian Bergstrand

    This program is free software; you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program;
    if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#import "PGLookupController.h"
#import "pplib.h"
#import "pploader.h"

#import <netdb.h>

@implementation PGLookupController

- (void)awakeFromNib
{
    [progress setUsesThreadedAnimation:NO];
    [super setWindowFrameAutosaveName:@"Name Lookup"];
}

- (id)initWithDelegate:(id)newDelegate
{
    delegate = newDelegate;
    return ([super initWithWindowNibName:@"PGLookup"]);
}

- (void)addAddresses:(NSArray*)addrs block:(BOOL)block
{
    if (addrs && [addrs count] > 0) {
        NSDistantObject<PPLoaderDOServerExtensions> *proxy;
        if (!(proxy = [delegate performSelector:@selector(loadProxy)])) {
            NSBeep();
            return;
        }
        
        NSDictionary *list = [proxy userList:block];
        if (!list) {
            NSBeginInformationalAlertSheet(
                NSLocalizedString(@"PeerGuardian: Add Address Failed", ""),
                @"OK", nil, nil, [self window],
                nil, nil, nil, nil,
                NSLocalizedString(@"The address could not be added, because an appropriate list could not be found."
                    @" Please create a custom list using the List Manager and try again.", ""));
            return;
        }
        
        // add the ranges to the list
        NSURL *url;
        @try {
            url = [NSURL URLWithString:[ListURLs(list) objectAtIndex:0]];
        } @catch (NSException *e) {
            url = nil;
        }
        int err = 0;
        NSString *msg;
        if (url && [url isFileURL]) {
            NSMutableArray *ranges = [self addressSpecificationsFromFile:[url path]];
            if (ranges) {
                [ranges addObjectsFromArray:addrs];
                if ((err = [self commitAddressSpecifications:ranges toFile:[url path]])) {
                    if (EINVAL == err)
                        msg = NSLocalizedString(@"One or more addresses are invalid.", "");
                    else
                        msg = NSLocalizedString(@"An error occured while trying to create or modify the file.", "");
                }
            } else {
                err = EIO;
                msg = NSLocalizedString(@"Failed to read the current list entries.", "");
            }
        } else {
            err = ENOENT;
            msg = NSLocalizedString(@"The list contains an invalid file path.", "");
        }
        
        if (!err)
            [self setValue:[NSNumber numberWithBool:NO] forKey:@"isNameValid"];
        else {
            NSBeginCriticalAlertSheet(
                NSLocalizedString(@"Could Not Save File", ""),
                @"OK", nil, nil, [self window],
                nil, nil, nil, nil,
                [NSString stringWithFormat:@"%@ (%d)", msg, err]);
        }
    }
}

- (IBAction)allowAddresses:(id)sender
{
    [self addAddresses:foundAddrs block:NO];
}

- (IBAction)blockAddresses:(id)sender
{
    [self addAddresses:foundAddrs block:YES];
}

- (void)lookupDidCompleteWithError:(NSError*)error
{
    NSArray *addrs = [[error userInfo] objectForKey:@"addrs"];
    NSString *s = @"";
    if (error && 0 == [error code] && addrs) {
        if ((s = [[error userInfo] objectForKey:@"name"]))
            [name setStringValue:s];
        s = @"";
        NSDictionary *addr;
        NSEnumerator *en = [addrs objectEnumerator];
        while ((addr = [en nextObject]))
            s = [s stringByAppendingFormat:@"%@\n", [addr objectForKey:PP_ADDR_SPEC_START]];
        
        [self setValue:[NSNumber numberWithBool:YES] forKey:@"isNameValid"];
        [foundAddrs release];
        foundAddrs = [addrs retain];
    } else if (!(s = [error localizedFailureReason])) {
        if (!(s = [error localizedDescription]))
            s = NSLocalizedString(@"Unknown error.", "");
    }
    [addresses setString:s];
    
    [name setEnabled:YES];
    [progress stopAnimation:nil];
    [[self window] makeFirstResponder:name];
}

- (void)nameLookupThread:(NSString*)dn
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    const char *s = [dn UTF8String];
    int err;
    
    NSMutableArray *addrs = nil;
    NSString *hostName = nil;
    struct hostent *ent = gethostbyname(s);
    if (ent && AF_INET == ent->h_addrtype) {
        addrs = [NSMutableArray array];
        err = 0;
        
        char ipstr[64];
        NSMutableDictionary *addrSpec;
        int i = 0;
        while ((s = ent->h_addr_list[i])) {
            struct in_addr a = {*((in_addr_t*)s)};
            (void)pp_inet_ntoa(a, ipstr, sizeof(ipstr));
            addrSpec = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                [NSString stringWithCString:ipstr encoding:NSUTF8StringEncoding], PP_ADDR_SPEC_START,
                [NSString stringWithCString:ent->h_name encoding:NSUTF8StringEncoding], PP_ADDR_SPEC_NAME,
                nil];
            [addrs addObject:addrSpec];
            ++i;
        }
        
        if (0 == strcmp(ipstr, [dn UTF8String]) && ent->h_length <= sizeof(ipstr)) {
            // Reverse lookup to get host name
            bcopy(ent->h_addr_list[0], ipstr, ent->h_length);
            if ((ent = gethostbyaddr(ipstr, ent->h_length, ent->h_addrtype))) {
                hostName = [NSString stringWithCString:ent->h_name encoding:NSUTF8StringEncoding];
                if (hostName) {
                    // Update the range names
                    NSEnumerator *en = [addrs objectEnumerator];
                    while((addrSpec = [en nextObject]))
                        [addrSpec setObject:hostName forKey:PP_ADDR_SPEC_NAME];
                }
            }
        }
    } else
        err = errno;
    
    NSDictionary *userInfo;
    if (addrs && [addrs count] > 0)
        userInfo = [NSDictionary dictionaryWithObjectsAndKeys:
            addrs, @"addrs",
            hostName, @"name",
            nil];
    else if (h_errno) {
         userInfo = [NSDictionary dictionaryWithObject:
            NSLocalizedString([NSString stringWithCString:hstrerror(h_errno) encoding:NSASCIIStringEncoding],
                "error string from domain lookup")
            forKey:NSLocalizedFailureReasonErrorKey];
    } else
        userInfo = nil;
    
    NSError *error = [NSError errorWithDomain:NSPOSIXErrorDomain code:err userInfo:userInfo];
    [self performSelectorOnMainThread:@selector(lookupDidCompleteWithError:)
        withObject:error waitUntilDone:NO];
    [pool release];
}

- (IBAction)lookupName:(id)sender
{
    [[self window] makeFirstResponder:nil]; // make sure name editing is commited
    [foundAddrs release];
    foundAddrs = nil;
    [addresses setString:@""];
    [self setValue:[NSNumber numberWithBool:NO] forKey:@"isNameValid"];
    [name setEnabled:NO];
    [progress startAnimation:sender];
    
    [NSThread detachNewThreadSelector:@selector(nameLookupThread:)
        toTarget:self withObject:[name stringValue]];
}

@end
