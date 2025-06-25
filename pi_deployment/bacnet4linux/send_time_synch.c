/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (c) 2000-2002 by Greg Holloway, hollowaygm@telus.net

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public License
 as published by the Free Software Foundation; either version 2.1
 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public 
 License along with this program; if not, write to 
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330 
 Boston, MA  02111-1307, USA.

 (See the included file COPYING)

 Modified by Steve Karg <skarg@users.sourceforge.net> 15 June 2003
 -------------------------------------------
####COPYRIGHTEND####*/
#include "os.h"
#include "bacnet_struct.h"
#include "bacnet_enum.h"
#include "bacnet_const.h"
#include "bacnet_api.h"
#include "bacnet_text.h"
#include "debug.h"

void time_synch(void)
{
    unsigned char apdu[15];
    time_t t;                   /* time holding variable */
    struct tm t_struct;
    struct tm *t_ptr;
    static time_t last_sync = 0;

    t_ptr = &t_struct;
    t = time(NULL);             /* get current time */
    t_ptr = localtime(&t);

    debug_printf(5, "time_synch: entered\n");

    if (t == last_sync)
        return;                 /* don't send back-to-back syncs */

    apdu[0] = 0x10;             /* unconfirmed service */
    apdu[1] = 0x06;             /* time synch request */
    apdu[2] = 0xA4;             /* application tag for date (length == 4) */
    apdu[3] = t_ptr->tm_year;
    apdu[4] = t_ptr->tm_mon + 1;
    apdu[5] = t_ptr->tm_mday;
    apdu[6] = 0xFF;             /* day of week is unspecified */
    apdu[7] = 0xB4;             /* application tag for time (length == 4) */
    apdu[8] = t_ptr->tm_hour;
    apdu[9] = t_ptr->tm_min;
    apdu[10] = t_ptr->tm_sec;
    apdu[11] = 0x00;            /* set hundredths to 0 */

    if (send_npdu(-1, &apdu[0], 12)) {  /* broadcast to all */
        debug_printf(2, "time_synch: Sent Global Time Synchronization\n");
        last_sync = t;          /* update the last time a sync was done */
    }
}

/* end of time synch */

/* end of send_time_synch.c */
