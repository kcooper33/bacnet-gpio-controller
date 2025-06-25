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
#include "debug.h"

/* quick function to return an I-Am */
void send_iam(int instance, unsigned short vendor_id)
{
    uint8_t *apdu;
    int len = 0;
    int apdu_len = 0;

    debug_printf(5, "send_iam: entered\n");

    apdu = pdu_alloc();
    if (apdu) {
        /* prepare a I-Am response */
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_I_AM;     // service choice
        apdu_len = 2;
        /* service request */
        len =
            encode_tagged_object_id(&apdu[apdu_len], OBJECT_DEVICE,
            instance);
        apdu_len += len;
        len = encode_tagged_unsigned(&apdu[apdu_len], MAX_APDU);
        apdu_len += len;
        len = encode_tagged_enumerated(&apdu[apdu_len], SEGMENTATION_NONE);
        apdu_len += len;
        len = encode_tagged_unsigned(&apdu[apdu_len], vendor_id);
        apdu_len += len;
        debug_printf(2, "send_iam: Sending I-Am Device %d...\n", instance);
        /* send I-Am broadcast */
        send_npdu(BACNET_BROADCAST_ID, &apdu[0], apdu_len);
        pdu_free(apdu);
    }
}
