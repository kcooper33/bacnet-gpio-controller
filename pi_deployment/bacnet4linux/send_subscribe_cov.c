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
#include "bacnet_struct.h"
#include "bacnet_enum.h"
#include "bacnet_const.h"
#include "bacnet_api.h"
#include "bacnet_text.h"
#include "bacdcode.h"
#include "pdu.h"
#include "main.h"
#include "debug.h"

/* function to initiate a SubscribeCOV service */
int subscribe_cov(int device, enum BACnetObjectType object, int instance)
{
    unsigned char *apdu;
    int status = -1;            // return value
    int len = 0;
    int apdu_len = 0;

    debug_printf(5, "subscribe_cov: entered\n");

    apdu = pdu_alloc();
    if (apdu) {
        apdu[0] = 0;            /* confirmed service */
        apdu[1] = 0;            /* max segs, max resp */
        apdu[2] = 0;            /* invoke id - filled in by net layer */
        apdu[3] = SERVICE_CONFIRMED_SUBSCRIBE_COV;      // service type
        apdu_len = 4;
        // subscriberProcessIdentifier
        len =
            encode_context_unsigned(&apdu[apdu_len], 0,
            BACNET_COV_PROCESS_ID);
        apdu_len += len;
        // monitoredObjectIdentifier
        len =
            encode_context_object_id(&apdu[apdu_len], 1, object, instance);
        apdu_len += len;
        // issueConfirmedNotifications - no!
        len = encode_context_enumerated(&apdu[apdu_len], 2, 0);
        apdu_len += len;
        // lifetime
        len =
            encode_context_unsigned(&apdu[apdu_len], 3,
            BACnet_COV_Lifetime + 60);
        apdu_len += len;
        if (send_npdu(device, &apdu[0], apdu_len)) {
            debug_printf(2, "subscribe_cov: Sent to %s %d in device %d \n",
                enum_to_text_object(object), instance, device);
            status = 0;
        }
        pdu_free(apdu);
    }

    return status;
}
