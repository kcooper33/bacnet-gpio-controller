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
// Receive unconfirmed COV notifications.
//
#include <stdio.h>

#include "bacnet_struct.h"
#include "bacnet_enum.h"
#include "bacnet_const.h"
#include "bacnet_api.h"
#include "bacnet_text.h"
#include "bacnet_device.h"
#include "bacnet_object.h"
#include "bacdcode.h"
#include "debug.h"

int receive_COV(uint8_t * apdu, int apdu_len,
    struct BACnet_Device_Address *src)
{
    int object;
    int instance;
    int offset = 0;
    int time_remaining = 0;
    int who_sent;
    float bacnet_real;
    struct ObjectRef_Struct *obj_ptr = NULL;
    struct BACnet_Device_Info *dev_ptr = NULL;

    debug_printf(5, "FN: Entered '%s'\n", __FILE__);

    who_sent = device_which_sent(src);
    dev_ptr = device_get(who_sent);
    if ((who_sent == -1) || (dev_ptr == NULL))
        return -1;

    /* no objects are yet known */
    if (object_count(who_sent) == 0)
        return -1;
    /* All COV requests are sent with a Common Process ID */
    if (apdu[3] == BACNET_COV_PROCESS_ID) {
        // FIXME: convert to use common decode tag functions
        /* what object and instance? */
        decode_object_id(&apdu[10], &object, &instance);
        obj_ptr = object_find(who_sent, object, instance);
        if (!obj_ptr)
            return -1;

        debug_printf(3, "RCOV: Object %d %s %d (%s) \n\tnew value is: ",
            who_sent, enum_to_text_object(object),
            instance, obj_ptr->name);

        /* time remaining one byte < 256 */
        if (apdu[14] == 0x39) {
            offset = 0;
            time_remaining = apdu[15];
        }
        /* time remaining two bytes > 255 < 65536 */
        if (apdu[14] == 0x3A) {
            offset = 1;
            time_remaining = apdu[15] * 256 + apdu[16];
        }
        /* Present Value */
        if (apdu[18 + offset] == PROP_PRESENT_VALUE) {
            /* handle analog values */
            /* real of length 4 bytes is next */
            if (apdu[20 + offset] == 0x44) {
                decode_real(&apdu[21 + offset], &bacnet_real);
                printf("%.2f ", bacnet_real);
                obj_ptr->value.real = bacnet_real;
            }
            /* handle binary values */
            /* enumerated of length 1 bytes is next */
            if (apdu[20 + offset] == 0x91) {
                obj_ptr->value.binary = apdu[21 + offset];
                if (obj_ptr->value.binary == BINARY_INACTIVE)
                    debug_printf(3, "%s ", obj_ptr->units.states.inactive);
                else
                    debug_printf(3, "%s ", obj_ptr->units.states.active);
            }
        }
        debug_printf(3, "\n\t%ds remaining in subscription\n",
            time_remaining);
    }
    return 0;                   /* just to be nice */
}

/* end of receive_COV.c */
