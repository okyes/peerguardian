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
#include <mach/semaphore.h>
#include "ppmsg.h"

__private_extern__ lck_spin_t     *pp_loglck;
__private_extern__ int32_t         pp_logclients;
__private_extern__ int32_t         pp_logallowed;

__private_extern__ semaphore_t     pp_logsignal;
// the lists have to be under a spinlock, since we will be on the network
// callback when accessing them, which means no blocking.
#define pp_logentries_lock() lck_spin_lock(pp_loglck)
#define pp_logentries_unlock() lck_spin_unlock(pp_loglck)

__private_extern__
pp_log_entry_t* pp_log_get_entry(int lockheld);

#ifdef notyet
__private_extern__
void pp_log_relse_entry(pp_log_entry_t*, int lockheld);
#endif

__private_extern__
void pp_log_processth(void *arg, wait_result_t wait);

#define pp_log_wakeup() semaphore_signal(pp_logsignal)
