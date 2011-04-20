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
#import <Foundation/Foundation.h>

#ifdef PPDBG
#define pptrace(fmt, ...) NSLog(fmt, ## __VA_ARGS__)
#else
#define pptrace(fmt, ...)
#endif

@protocol PPLoaderDOClientExtensions

- (oneway void)listWillUpdate:(id)list;
- (oneway void)listDidUpdate:(id)list;
- (oneway void)listFailedUpdate:(id)list error:(NSDictionary*)error;
// Some properties of the list changed -- ie enabled, allow, etc
- (oneway void)listDidChange:(id)list;
- (oneway void)listWasRemoved:(id)list; 

@end

@protocol PPLoaderDOServerExtensions

// Returns -1 for failure, or a new client id
- (unsigned int)registerClient:(NSDistantObject*)client;
- (void)deregisterClient:(unsigned int)clientID;

- (NSArray*)lists;
// Returns an empty list
- (NSMutableDictionary*)createList;
- (NSMutableDictionary*)userList:(BOOL)blockList;
- (oneway void)updateAllLists;
- (oneway void)updateList:(id)list;
// change/add list
- (BOOL)addList:(id)list;
- (BOOL)changeList:(id)list;
- (oneway void)removeList:(id)list;
- (int)setPortRule:(NSString*)rule;

// get pref setting
- (id)objectForKey:(NSString*)key;
- (oneway void)setObject:(id)obj forKey:(NSString*)key;

- (BOOL)isEnabled;
- (int)disable;
- (oneway void)enable;

@end

#define ListURLs(l) [[l objectForKey:@"URLs"] allKeys]
#define ListActive(l) [[l objectForKey:@"Active"] boolValue]
#define ListName(l) [l objectForKey:@"Description"]

PG_DEFINE_TABLE_ID(PG_TABLE_ID_NULL,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
#define PP_LIST_NEWID PG_TABLE_ID_NULL
#define PP_LOGFILE [@"~/Library/Logs/PeerGuardian.log" stringByExpandingTildeInPath]
#define PP_LOGFILE_CREATED @"xxx.qnation.PPLogFileCreated"

#define PP_KEXT_PATH @"/Library/Extensions/PeerGuardian.kext"
#define PPKTOOL_PATH "/Library/Application Support/PeerGuardian/pgstart"

// Distributed notifications
#define PP_HELPER_QUIT @"xxx.qnation.PP_51554954"
#define PP_KEXT_RELOAD @"xxx.qnation.PP_4b52656c6f6164"
#define PP_KEXT_WILL_LOAD @"xxx.qnation.PPKextWillLoad"
#define PP_KEXT_DID_LOAD @"xxx.qnation.PPKextDidLoad"
#define PP_KEXT_FAILED_LOAD @"xxx.qnation.PPKextFailedLoad"
// This can be sent multiple times
#define PP_KEXT_WILL_UNLOAD @"xxx.qnation.PPKextWillUnload"
#define PP_KEXT_DID_UNLOAD @"xxx.qnation.PPKextDidUnload"
#define PP_KEXT_FAILED_UNLOAD @"xxx.qnation.PPKextFailedUnload"

#define PP_KEXT_FAILED_ERR_KEY @"error"

#define PP_PREF_CHANGED @"xxx.qnation.PPPrefChanged"
#define PP_PREF_KEY @"key"
#define PP_PREF_VALUE @"value"

#define PP_FILTER_CHANGE @"xxx.qnation.PP_464368616e6765"
#define PP_FILTER_CHANGE_ENABLED_KEY @"Enabled"

#define PP_ADDR_ACTION_ASK @"xxx.qnation.PPActionAsk"

#define PP_SHOW_STATS @"xxx.qnation.PPShowStats"

// Preference keys
#define PP_PREF_PORT_RULES @"Port Rules"
