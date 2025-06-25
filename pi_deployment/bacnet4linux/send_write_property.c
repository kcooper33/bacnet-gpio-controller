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
#include <assert.h>             // warns programmer about an assumption
#include "os.h"
#include "bacnet_struct.h"
#include "bacnet_enum.h"
#include "bacnet_api.h"
#include "bacnet_text.h"
#include "bacdcode.h"
#include "pdu.h"
#include "debug.h"

/* will initiate a write-property */
int write_property(int device,
    enum BACnetObjectType object,
    int instance,
    enum BACnetPropertyIdentifier property,
    unsigned char valuetype, union ObjectValue value)
{
    unsigned char *apdu;
    int status = -1;            // return value
    int len = 0;
    int apdu_len = 0;

    debug_printf(5, "write_property: entered\n");

    apdu = pdu_alloc();
    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = 0;            /* max segs, max resp */
        apdu[2] = 0;            /* invoke id - filled in by net layer */
        apdu[3] = SERVICE_CONFIRMED_WRITE_PROPERTY;     // service type
        apdu_len = 4;
        // objectIdentifier
        len =
            encode_context_object_id(&apdu[apdu_len], 0, object, instance);
        apdu_len += len;
        len = encode_context_enumerated(&apdu[apdu_len], 1, property);
        apdu_len += len;
        // FIXME: array context specific tag [2] is optional
        // propertyValue
        len = encode_opening_tag(&apdu[apdu_len], 3);
        apdu_len += len;
        if (valuetype == PRIMATIVE_BINARY) {
            // FIXME:   
        } else if (valuetype == PRIMATIVE_UNSIGNED) {
            len = encode_tagged_unsigned(&apdu[apdu_len], value.integer);
            apdu_len += len;
            debug_printf(3,
                "write_property: writing %d to %s %d in device %d\n",
                value.integer,
                enum_to_text_object(object), instance, device);
        } else if (valuetype == PRIMATIVE_REAL) {
            len = encode_tagged_real(&apdu[apdu_len], value.real);
            apdu_len += len;
            debug_printf(3,
                "write_property: writing %.2f to %s %d in device %d\n",
                value.real, enum_to_text_object(object), instance, device);
        } else if (valuetype == PRIMATIVE_STRING) {
            // FIXME:
        } else if (valuetype == PRIMATIVE_ENUMERATED) {
            len = encode_tagged_enumerated(&apdu[apdu_len], value.binary);
            apdu_len += len;
        }
        len = encode_closing_tag(&apdu[apdu_len], 3);
        apdu_len += len;
        if (send_npdu(device, &apdu[0], apdu_len)) {
            debug_printf(2,
                "write_property: Sent to %s of %s %d in device %d\n",
                enum_to_text_property(property),
                enum_to_text_object(object), instance, device);
            status = 0;
        }
        pdu_free(apdu);
    }

    return status;
}

/* end of write property */
