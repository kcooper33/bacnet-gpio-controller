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
// APDU receiving and simple processing function
//
#include "os.h"
#include "bacnet_struct.h"
#include "bacnet_const.h"
#include "bacnet_device.h"
#include "bacnet_error.h"
#include "bacnet_enum.h"
#include "bacnet_text.h"
#include "bacnet_api.h"
#include "invoke_id.h"
#include "debug.h"
#include "options.h"
#include "pdu.h"
#include "reject.h"

// Function declarations
int receive_writeproperty(uint8_t * service_request, int service_len,
    struct BACnet_Device_Address *src, int src_max_apdu, uint8_t invoke_id);

static int convert_max_pdu(uint8_t octet)
{
    int message_size = 0;       // return value

    // mask the max-segmenets-accepted
    switch (octet & 0x0F) {
        // minimum-message-size
    case 0:
        message_size = 50;
        break;
    case 1:
        message_size = 128;
        break;
        // lon-talk frame
    case 2:
        message_size = 206;
        break;
        // ARCNET or MS/TP frame
    case 3:
        message_size = 480;
        break;
    case 4:
        message_size = 1024;
        break;
        // ISO 8802-3 frame
    case 5:
        message_size = 1476;
        break;
    default:
        break;
    }

    return message_size;
}

int receive_apdu(uint8_t * apdu, int apdu_len,
    struct BACnet_Device_Address *src)
{
    int i;
    int service_choice = -1;    // return value
    int segmented_message;
    int src_max_apdu = 0;
    int more_follows;
    int PDU_type;
    int PDU_field;
    int seq_number;
    int window_size;
    int segmented_accepted;
    int who_sent = 0;           /* which device made the request */
    time_t t = 0;               /* seconds since epoch time */
    uint8_t invoke_id = 0;      /* temporary Invoke ID */
    int srv_req_start = 0;
    struct BACnet_Device_Info *dev_ptr = NULL;
    int status = 0;

    i = 0;

    t = time(NULL);
    /* which device sent this? (-1 == unknown) */
    who_sent = device_which_sent(src);
    dev_ptr = device_get(who_sent);
    if (who_sent >= 0) {
        debug_printf(3,
            "receive-apdu: Updating last_found time for Device %d\n",
            who_sent);
        if (dev_ptr)
            dev_ptr->last_found = t;
    }
    //Parse the PDU Field
    PDU_field = apdu[0];        /* what PDU_field? */
    PDU_type = PDU_field >> 4;
    segmented_message = (PDU_field & 0x08) ? 1 : 0;
    more_follows = (PDU_field & 0x04) ? 1 : 0;

    debug_printf(3, "receive-apdu: PDU Field: 0x%2X\n", PDU_field);

    switch (PDU_field & 0xF0) {
    case PDU_TYPE_CONFIRMED_SERVICE_REQUEST:
        debug_printf(3,
            "receive-apdu:    %04X .... = PDU Type:  BACnet_Confirmed_Request_PDU\n",
            PDU_type);
        debug_printf(3,
            "receive-apdu:    .... %d... = segmented_message is %d\n",
            segmented_message, segmented_message);
        debug_printf(3,
            "receive-apdu:    .... .%d.. = more_follows is %d\n",
            more_follows, more_follows);
        segmented_accepted = PDU_field & 0x02;
        debug_printf(3,
            "receive-apdu:    .... ..%d. = segmented_accepted is %d\n",
            segmented_accepted, segmented_accepted);
        src_max_apdu = convert_max_pdu(apdu[1]);
        invoke_id = apdu[2];
        debug_printf(3, "receive-apdu:    %d = invoke_id\n",
            (int) invoke_id);
        if (segmented_message) {
            seq_number = apdu[3];
            window_size = apdu[4];
            service_choice = apdu[5];
            srv_req_start = 6;
        } else {
            service_choice = apdu[3];
            srv_req_start = 4;
        }
        debug_printf(3,
            "receive-apdu: %s\n",
            enum_to_text_service_confirmed(service_choice));
        switch (service_choice) {
        case SERVICE_CONFIRMED_READ_PROPERTY:
            /* handle read-property (giving another device a value we have) in another function */
            receive_readproperty(&apdu[srv_req_start],
                apdu_len - srv_req_start, src, src_max_apdu, invoke_id);
            break;
        case SERVICE_CONFIRMED_WRITE_PROPERTY:
            /* handle write-property (setting values in our device) */
            receive_writeproperty(&apdu[srv_req_start],
                apdu_len - srv_req_start, src, src_max_apdu, invoke_id);
            break;
        default:
            debug_printf(4,
                "receive-apdu:    %d = service_choice (unsupported)\n",
                service_choice);
            send_reject_address(src, invoke_id,
                REJECT_REASON_UNRECOGNIZED_SERVICE);
            break;
        }
        break;
    case PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST:
        debug_printf(3,
            "receive-apdu:    %04X .... = PDU Type:  BACnet_Unconfirmed_Request_PDU \n",
            PDU_type);
        service_choice = apdu[1];
        debug_printf(3, "receive-apdu: %s\n",
            enum_to_text_service_unconfirmed(service_choice));
        switch (service_choice) {
        case SERVICE_UNCONFIRMED_I_AM:
            receive_IAm(apdu, apdu_len, src);
            break;
        case SERVICE_UNCONFIRMED_WHO_IS:
            //Note that we need to support ranged Who-Is here, too.
            send_iam(BACnet_Device_Instance, BACnet_Vendor_Identifier);
            break;
        case SERVICE_UNCONFIRMED_COV_NOTIFICATION:
            receive_COV(apdu, apdu_len, src);
            break;
        case SERVICE_UNCONFIRMED_I_HAVE:
        case SERVICE_UNCONFIRMED_EVENT_NOTIFICATION:
        case SERVICE_UNCONFIRMED_PRIVATE_TRANSFER:
        case SERVICE_UNCONFIRMED_TEXT_MESSAGE:
        case SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION:
        case SERVICE_UNCONFIRMED_WHO_HAS:
        case SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION:
        default:
            break;
        }
        break;
    case PDU_TYPE_SIMPLE_ACK:
        debug_printf(3,
            "receive-apdu:    %04X .... = PDU Type:  BACnet_SimpleACK_PDU\n",
            PDU_type);
        invoke_id = apdu[1];
        service_choice = apdu[2];
        debug_printf(3, "receive-apdu:    %d = invoke_id\n",
            (int) invoke_id);
        debug_printf(3,
            "receive-apdu: %s\n",
            enum_to_text_service_confirmed(service_choice));
        /* return this invoke ID to the pool (no further action is needed) */
        invoke_id_reset(invoke_id);
        break;
    case PDU_TYPE_COMPLEX_ACK:
        debug_printf(3,
            "receive-apdu:    %04X .... = PDU Type:  BACnet_ComplexACK_PDU\n",
            PDU_type);
        invoke_id = apdu[1];    /* the original Invoke ID */
        debug_printf(3, "receive-apdu:    %d = invoke_id\n",
            (int) invoke_id);
        invoke_id_reset(invoke_id);     /* return this invoke ID to the pool (no further action is needed) */

        if (segmented_message) {
            seq_number = apdu[2];
            window_size = apdu[3];
            service_choice = apdu[4];
            srv_req_start = 5;
        } else {
            service_choice = apdu[2];
            srv_req_start = 3;
        }
        debug_printf(3,
            "receive-apdu: %s\n",
            enum_to_text_service_confirmed(service_choice));
        if (service_choice == SERVICE_CONFIRMED_READ_PROPERTY) {
            /* handle read-property ACKs (values we wanted) 
               in another function */
            status = receive_readpropertyACK(&apdu[srv_req_start],
                apdu_len - srv_req_start, src);
            if (status == -1)
                debug_printf(3,
                    "receive-apdu: Received read-property-ACK that "
                    "couldn't be handled!\n");
        }
        break;
    case PDU_TYPE_SEGMENT_ACK:
        debug_printf(3,
            "receive-apdu:    %04X .... = PDU Type:  BACnet_SegmentACK_PDU \n",
            PDU_type);
        invoke_id = apdu[1];
        seq_number = apdu[2];
        window_size = apdu[3];
        debug_printf(4, "receive-apdu:    %d = invoke_id\n",
            (int) invoke_id);
        debug_printf(4, "receive-apdu:    %d = sequence number\n",
            seq_number);
        debug_printf(4, "receive-apdu:    %d = actual window size\n",
            window_size);
        /* return this invoke ID to the pool (no further action is needed) */
        invoke_id_reset(invoke_id);
        break;
    case PDU_TYPE_ERROR:
        // Error-PDU
        // pdu type [0] Unsigned (0..15) -- 6 for this PDU type
        // reserved [1] Unsigned (0..15)-- must be 0
        // original-invoke_id [2] Unsigned 0-255
        // error-choice [3] BACnetConfirmedServiceChoice
        // error [4] BACnet-Error
        invoke_id = apdu[1];    /* the original Invoke ID */
        invoke_id_reset(invoke_id);     /* return this invoke ID to the pool (no further action is needed) */
        debug_printf(3,
            "receive-apdu:    %04X .... = PDU Type:  BACnet_Error_PDU\n",
            PDU_type);
        break;
    case PDU_TYPE_REJECT:
        // Reject-PDU
        // pdu type [0] Unsigned (0..15) -- 6 for this PDU type
        // reserved [1] Unsigned (0..15)-- must be 0
        // original-invoke_id [2] Unsigned 0-255
        // reject reason [3] enumeration
        invoke_id = apdu[1];    /* the original Invoke ID */
        invoke_id_reset(invoke_id);     /* return this invoke ID to the pool (no further action is needed) */
        debug_printf(3, "receive-apdu: PDU_TYPE_REJECT: %s\n",
            enum_to_text_reject_reason(apdu[2]));
        break;
    case PDU_TYPE_ABORT:
        // Abort-PDU
        // pdu type [0] Unsigned (0..15) -- 6 for this PDU type
        // reserved [1] Unsigned (0..7) -- must be 0
        // server [2] BOOLEAN
        // original-invoke_id [3] Unsigned 0-255
        // abort reason [4] enumeration
        invoke_id = apdu[1];    /* the original Invoke ID */
        invoke_id_reset(invoke_id);     /* return this invoke ID to the pool (no further action is needed) */
        debug_printf(3, "receive-apdu: PDU_TYPE_ABORT: %s\n",
            enum_to_text_abort_reason(apdu[2]));
        break;
    }

    return service_choice;
}
