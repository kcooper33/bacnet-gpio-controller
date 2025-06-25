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
//
// This function verifies that all known devices are still online.
//
#include "os.h"
#include "bacnet_api.h"
#include "bacnet_const.h"
#include "bacnet_device.h"
#include "bacnet_object.h"
#include "bacnet_struct.h"
#include "debug.h"
#include "main.h"
#include "options.h"

int check_online_status(void)
{
    time_t t = 0;               /* time_h storage for time */
    time_t duration = 0;        /* time_h storage for time */
    int sent_packet = 0;        /* have we sent a packet this time? */
    struct BACnet_Device_Info *dev_ptr = NULL;
    static int device_index = 0;        /* we only want to send one packet each time this is entered */
    static time_t last_verify_t = 0;

    /* find current time */
    t = time(NULL);
    if (last_verify_t == 0)
        last_verify_t = t;
    /* begin sweep of known devices for verification */
    duration = t - last_verify_t;
    if (duration > (60 * 5)) {  /* keeps this from executing too fast */
        last_verify_t = t;
        sent_packet = 0;

        dev_ptr = device_record(device_index);
        if (dev_ptr && (dev_ptr->device != BACnet_Device_Instance)) {
            duration = t - dev_ptr->last_found;
            if (duration > (60 * 60 * 24)) {
                debug_printf(3,
                    "CS: Removing %d - she's been too quiet.\n",
                    dev_ptr->device);
                device_record_remove(device_index);
            } else if (duration > (60 * 5)) {
                /* no other traffic in a reasonable time */
                debug_printf(3,
                    "CS: Sending Who-Is to %d to check online status...\n",
                    dev_ptr->device);
                /* send Who-Is to this device */
                send_whois(dev_ptr->device);
            } else {
#if 0
                // FIXME: good idea - implement later      
                /* why not check if a devices' ObjectList has changed? */
                read_property(dev_ptr->device,
                    OBJ_DEVICE, dev_ptr->device, object_list, 0);
#endif
            }
            /* we only want to send one packet each time this is entered */
            device_index++;
        } else {
            device_index = 0;
        }
    }
    /* end sweep of known devices */

    return 0;                   /* just to be nice */
}
