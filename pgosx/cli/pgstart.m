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
#import <Foundation/Foundation.h>
#import <Security/Security.h>
#import <unistd.h>
#import <fcntl.h>
#import <mach/mach.h>

#define PG_MAKE_TABLE_ID
#import "pplib.h"
#import "pploader.h"

static const char whatid[] __attribute__ ((used)) =
"@(#)$LastChangedRevision: 565 $ Built: " __DATE__ ", " __TIME__;

#include <sys/syslog.h>
#ifdef PPDBG
#undef pptrace
#define pptrace(fmt, ...) syslog(LOG_MAKEPRI(LOG_DAEMON,LOG_NOTICE), (fmt), ## __VA_ARGS__)
#endif

enum {
    op_bad = -1,
    op_isloaded = 0,
    op_load,
    op_reload, // this is used by the post-install script
    op_unload,
};

@interface Dummy : NSObject {
}
- (void)dummy:(NSTimer*)timer;
@end 
@implementation Dummy 
- (void)dummy:(NSTimer*)timer {}
@end

#define KINFO_PATH "/var/run/.pgkextinfo"
#define KEXT_BUNDLE_ID [NSString stringWithCString:PP_CTL_NAME encoding:NSASCIIStringEncoding]

static unsigned pglastkextver(int fd)
{
    unsigned ver = 0;
    if (fd || (fd = open(KINFO_PATH, O_RDONLY)) > 0) {
        (void)read(fd, &ver, sizeof(ver));
        close(fd);
    }
    return (ver);
}

static int pgsetkextver(unsigned ver, int fd)
{
    int err = 0;
    if (fd || (fd = open(KINFO_PATH, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR)) > 0) {
        if (-1 == pwrite(fd, &ver, sizeof(ver), 0))
            err = errno;
        close(fd);
    } else if (-1 == fd)
        err = errno;
    return (err);
}

static unsigned pgkextver(void)
{
    unsigned ver = 0;
    CFBundleRef b = CFBundleCreate(kCFAllocatorDefault, (CFURLRef)[NSURL fileURLWithPath:PP_KEXT_PATH]);
    if (b) {
        ver = CFBundleGetVersionNumber(b);
        CFRelease(b);
    }
    return (ver);
}

static int runtask(NSString *cmd, id output, NSArray *args)
{
    if (!cmd || !args)
        return (EINVAL);
    
    NSTask *t = [[NSTask alloc] init];
    [t setLaunchPath:cmd];
    [t setArguments:args];
    [t setStandardOutput:output ? output : [NSFileHandle fileHandleWithNullDevice]];
    int err;
    @try {
        [t launch];
        [t waitUntilExit];
        err = [t terminationStatus];
    } @catch (NSException *exception) {
        err = EINVAL;
    }
    
    [t release];
    
    return (err);
}

static void Notify(NSString *event, int err)
{
    if (event) {
        NSDictionary *d = nil;
        if (err)
            d = [NSDictionary dictionaryWithObject:[NSNumber numberWithInt:err]
                forKey:PP_KEXT_FAILED_ERR_KEY];
        [[NSDistributedNotificationCenter defaultCenter]
            postNotificationName:event
            object:nil userInfo:d
            options:NSNotificationPostToAllSessions|NSNotificationDeliverImmediately];
        (void)[[NSRunLoop currentRunLoop] limitDateForMode:NSDefaultRunLoopMode];
    }
}

static int iskextloaded(void)
{
    kern_return_t err;
    kmod_info_array_t kmods;
    mach_msg_type_number_t kmodsSize = 0;
    mach_port_t host = mach_host_self();
    err = kmod_get_info(host, (void*)&kmods, &kmodsSize);
    (void)mach_port_deallocate(mach_task_self(), host);
    if (0 == err) {
        kmod_info_t *kmp = (typeof(kmp))kmods;
        do {
            if (0 == strcasecmp(kmp->name, PP_CTL_NAME))
                break;
            
            if (kmp->next)
                ++kmp;
            else {
                kmp = NULL;
                break;
            }
        } while(1);
        
        vm_deallocate(mach_task_self(), (vm_address_t)kmods, kmodsSize);
        return (kmp ? 0 : ENOENT);
    } else {
        // try to open a session
        pp_session_t s;
        if (0 == pp_open_session(&s)) {
            pp_close_session(s);
            return (0);
        }
    }
    return (ENOENT);
}

static int loadkext(void)
{
    // launchd respawns us because of our fork/execs
    // And kextload does returns 0 when the kext is already loaded.
    if (0 == iskextloaded())
        return (0);
    
    Notify(PP_KEXT_WILL_LOAD, 0);
    
    NSString *event;
    int err = runtask(@"/sbin/kextload", nil, [NSArray arrayWithObject:PP_KEXT_PATH]);
    if (0 == err) {
        event = PP_KEXT_DID_LOAD;
        
        // write version for use by reload
        unsigned ver = pgkextver();
        if (ver) {
            (void)pgsetkextver(ver, 0);
        }
    } else
        event = PP_KEXT_FAILED_LOAD;
    
    Notify(event, err);
    return (err);
}

static int unloadkext(void)
{
    NSArray *args = [NSArray arrayWithObjects:@"-b", KEXT_BUNDLE_ID, nil];
    pp_msg_stats_t s;
    do {
        pptrace("posting will unload");
        Notify(PP_KEXT_WILL_UNLOAD, 0);
        // Wait for clients to disappear
        (void)[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:2.0]];
        if (0 != pp_statistics(&s))
            break;
        pptrace("userclients: %u", s.pps_userclients);
        if (0 == (s.pps_userclients - 1 /*account for ourself*/))
            break;
    } while (1);
    
    NSString *event;
    int err = runtask(@"/sbin/kextunload", nil, args);
    if (0 == err) {
        event = PP_KEXT_DID_UNLOAD;
        (void)pgsetkextver(0, 0);
    } else
        event = PP_KEXT_FAILED_UNLOAD;
    
    Notify(event, err);
    return (err);
}

int main(int argc, char *argv[])
{
    char *launch = (strrchr(argv[0], '/') + 1);
    if (!launch)
        launch = argv[0];

    openlog(launch, 0, LOG_DAEMON);
    
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    pptrace("argc: %d", argc);
    
    int err = 0, opt, op = op_bad;
    while (-1 != (opt = getopt(argc, argv, "ilru"))) {
        switch (opt) {
            case 'i':
                op = op_isloaded;
            break;
            case 'l':
                op = op_load;
            break;
            case 'r':
                op = op_reload;
            break;
            case 'u':
                op = op_unload;
            break;
            case '?':
                pptrace("invalid argument");
                exit(EINVAL);
            break;
        }
    }
    
    if (op_isloaded != op && getuid() != 0) {
    #ifdef notyet
        // Check for security token only when passed a security ref through a pipe.
        // Currently we are launched via AuthorizationExecuteWithPrivileges()
        // which just runs us as root.
        AuthorizationRef authToken;
        if (0 != AuthorizationCopyPrivilegedReference(&authToken, kAuthorizationFlagDefaults)) {
            err = EACCES;
            pptrace("AuthorizationCopyPrivilegedReference failed");
        }
    #endif
        if (!err && geteuid() != 0) {
            err = EACCES;
            pptrace("geteuid failed");
        }
        if (!err && -1 == setuid(0)) {
            err = errno;
            pptrace("setuid failed: %d", err);
        }
        
        if (err) {
            if (op_load == op || op_unload == op)
                Notify(op_load == op ? PP_KEXT_FAILED_LOAD : PP_KEXT_FAILED_UNLOAD, err);
            goto main_exit;
        }
    }
    
    // 10.4.2 kernel bug causes panic during deregistration of IPV6 socket filters
    if (op_unload == op) {
         long ver = 0; 
         Gestalt (gestaltSystemVersion, &ver);
         if (ver < 0x00001043) {
            err = EPERM;
            pptrace("invalid system version");
            Notify(PP_KEXT_FAILED_UNLOAD, err);
            goto main_exit;
        }
    }
    
    // This is just to emulate sleep() with the run loop -- actually using sleep causes problems
    // with delivering notifications.
    Dummy *dummy  = [[Dummy alloc] init];
    (void)[NSTimer scheduledTimerWithTimeInterval:4200000000.0 target:dummy selector:@selector(dummy)
        userInfo:nil repeats:NO];
    
    pptrace("processing op: %d", op);
    switch (op) {
        case op_load:
           err = loadkext();
        break;
        case op_isloaded:
            err = iskextloaded();
        break;
        case op_reload: {
            unsigned lastver = pglastkextver(0);
            if (0 == lastver || pgkextver() > lastver) {
                [[NSDistributedNotificationCenter defaultCenter]
                    postNotificationName:PP_HELPER_QUIT object:nil userInfo:nil options:NSNotificationPostToAllSessions];
                (void)[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:2.0]];    
                (void)unloadkext();
                (void)[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.0]];
                err = loadkext();
            } else
                err = 0; // no need for a reload
        } break;
        case op_unload:
            err = unloadkext();
        break;
        default:
            err = EINVAL;
        break;
    }
    
main_exit:
    [pool release];
    
    pptrace("main returning %d for op: %d", err, op);
    closelog();
    return (err);
}
