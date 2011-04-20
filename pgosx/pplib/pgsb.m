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

#import <Foundation/Foundation.h>
#import <sandbox.h>
#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5
#pragma weak sandbox_init
#endif

#import "pgsb.h"

void pgsbinit()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    #if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5
    if (sandbox_init)
    #endif
    {
        const char *sbf = [[[NSBundle mainBundle] pathForResource:@"pg" ofType:@"sb"] fileSystemRepresentation];
        if (sbf) {
            char *sberr = NULL;
            (void)sandbox_init(sbf, SANDBOX_NAMED_EXTERNAL /*XXX: SPI*/, &sberr);
            if (sberr) {
                #ifdef PPDBG
                if (strlen(sberr) > 0)
                    NSLog(@"sandbox error: '%s'\n", sberr);
                #endif
                sandbox_free_error(sberr);
            }
        }
    }
    
    [pool release];
}
