/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (c) 2004 by Steve Karg <skarg@users.sourceforge.net>

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
 -------------------------------------------
####COPYRIGHTEND####*/
#include "bacnet_enum.h"
#include "bacnet_text.h"
#include "bacnet_api.h"
#include "bacdcode.h"
#include "pdu.h"
#include "debug.h"
#include "reject.h"

void send_reject(int device_instance, uint8_t invoke_id,
    uint8_t reject_reason)
{
    char *apdu;                 // for sending out an APDU

    if (device_instance != -1) {
        apdu = pdu_alloc();
        if (apdu) {
            apdu[0] = PDU_TYPE_REJECT;
            apdu[1] = invoke_id;
            apdu[2] = reject_reason;
            debug_printf(3,
                "reject: Sending %s to %d...\n",
                enum_to_text_reject_reason(reject_reason),
                device_instance);
            send_npdu(device_instance, &apdu[0], 3);
            pdu_free(apdu);
        }
    }

    return;
}

void send_reject_address(struct BACnet_Device_Address *dest,
    uint8_t invoke_id, uint8_t reject_reason)
{
    char *apdu;                 // for sending out an APDU

    if (dest) {
        apdu = pdu_alloc();
        if (apdu) {
            apdu[0] = PDU_TYPE_REJECT;
            apdu[1] = invoke_id;
            apdu[2] = reject_reason;
            debug_printf(3,
                "reject: Sending %s...\n",
                enum_to_text_reject_reason(reject_reason));
            send_npdu_address(dest, &apdu[0], 3);
            pdu_free(apdu);
        }
    }

    return;
}

void send_abort(int device_instance, uint8_t invoke_id,
    uint8_t abort_reason)
{
    char *apdu;                 // for sending out an APDU

    if (device_instance != -1) {
        apdu = pdu_alloc();
        if (apdu) {
            apdu[0] = PDU_TYPE_ABORT;
            apdu[1] = invoke_id;        /* invoke id from request */
            apdu[2] = abort_reason;
            debug_printf(3,
                "abort: Sending %s to %d...\n",
                enum_to_text_abort_reason(abort_reason), device_instance);
            send_npdu(device_instance, &apdu[0], 3);
            pdu_free(apdu);
        }
    }

    return;
}

void send_abort_address(struct BACnet_Device_Address *dest,
    uint8_t invoke_id, uint8_t abort_reason)
{
    char *apdu;                 // for sending out an APDU

    if (dest) {
        apdu = pdu_alloc();
        if (apdu) {
            apdu[0] = PDU_TYPE_ABORT;
            apdu[1] = invoke_id;        /* invoke id from request */
            apdu[2] = abort_reason;
            debug_printf(3,
                "abort: Sending %s...\n",
                enum_to_text_abort_reason(abort_reason));
            send_npdu_address(dest, &apdu[0], 3);
            pdu_free(apdu);
        }
    }

    return;
}

void send_error(int device_instance, uint8_t invoke_id,
    uint8_t service, int error_class, int error_code)
{
    char *apdu;                 // for sending out an APDU
    int apdu_len = 0;

    if (device_instance != -1) {
        apdu = pdu_alloc();
        if (apdu) {
            /* no match for property send back error */
            apdu[0] = PDU_TYPE_ERROR;   /* Error service */
            apdu[1] = invoke_id;        /* invoke id from request */
            apdu[2] = service;
            apdu_len = 3;
            // service parameters
            apdu_len += encode_tagged_enumerated(&apdu[apdu_len],
                error_class);
            apdu_len += encode_tagged_enumerated(&apdu[apdu_len],
                error_code);
            debug_printf(3,
                "abort: Sending %s %s to %d...\n",
                enum_to_text_error_class(error_class),
                enum_to_text_error_code(error_code), device_instance);
            send_npdu(device_instance, &apdu[0], apdu_len);
            pdu_free(apdu);
        }
    }

    return;
}

void send_error_address(struct BACnet_Device_Address *dest,
    uint8_t invoke_id, uint8_t service, int error_class, int error_code)
{
    char *apdu;                 // for sending out an APDU
    int apdu_len = 0;

    if (dest) {
        apdu = pdu_alloc();
        if (apdu) {
            /* no match for property send back error */
            apdu[0] = PDU_TYPE_ERROR;   /* Error service */
            apdu[1] = invoke_id;        /* invoke id from request */
            apdu[2] = service;
            apdu_len = 3;
            // service parameters
            apdu_len += encode_tagged_enumerated(&apdu[apdu_len],
                error_class);
            apdu_len += encode_tagged_enumerated(&apdu[apdu_len],
                error_code);
            debug_printf(3,
                "abort: Sending %s %s...\n",
                enum_to_text_error_class(error_class),
                enum_to_text_error_code(error_code));
            send_npdu_address(dest, &apdu[0], apdu_len);
            pdu_free(apdu);
        }
    }

    return;
}
