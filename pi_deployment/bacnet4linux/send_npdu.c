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
#include <assert.h>
#include "os.h"
#include "bacnet_struct.h"
#include "bacnet_device.h"
#include "bacnet_const.h"
#include "bacnet_api.h"
#include "ethernet.h"
#include "invoke_id.h"
#include "reject.h"
#include "debug.h"
#include "main.h"
#include "pdu.h"
#include "net.h"


//#define SEND_NPDU_LOCAL

/* max-segments-accepted
   B'000'      Unspecified number of segments accepted.
   B'001'      2 segments accepted.
   B'010'      4 segments accepted.
   B'011'      8 segments accepted.
   B'100'      16 segments accepted.
   B'101'      32 segments accepted.
   B'110'      64 segments accepted.
   B'111'      Greater than 64 segments accepted.
*/

/* max-APDU-length-accepted
   B'0000'  Up to MinimumMessageSize (50 octets)
   B'0001'  Up to 128 octets
   B'0010'  Up to 206 octets (fits in a LonTalk frame)
   B'0011'  Up to 480 octets (fits in an ARCNET frame)
   B'0100'  Up to 1024 octets
   B'0101'  Up to 1476 octets (fits in an ISO 8802-3 frame)
   B'0110'  reserved by ASHRAE
   B'0111'  reserved by ASHRAE
   B'1000'  reserved by ASHRAE
   B'1001'  reserved by ASHRAE
   B'1010'  reserved by ASHRAE
   B'1011'  reserved by ASHRAE
   B'1100'  reserved by ASHRAE
   B'1101'  reserved by ASHRAE
   B'1110'  reserved by ASHRAE
   B'1111'  reserved by ASHRAE
*/
static uint8_t get_max_seg_max_apdu(int max_segs, int max_apdu)
{
    uint8_t octet = 0;

    if (max_segs < 2)
        octet = 0;
    else if (max_segs < 4)
        octet = 0x10;
    else if (max_segs < 8)
        octet = 0x20;
    else if (max_segs < 16)
        octet = 0x30;
    else if (max_segs < 32)
        octet = 0x40;
    else if (max_segs < 64)
        octet = 0x50;
    else if (max_segs == 64)
        octet = 0x60;
    else
        octet = 0x70;

    // max_apdu must be 50 octets minimum
    assert(max_apdu >= 50);
    if (max_apdu == 50)
        octet |= 0x00;
    else if (max_apdu <= 128)
        octet |= 0x01;
    //fits in a LonTalk frame
    else if (max_apdu <= 206)
        octet |= 0x02;
    //fits in an ARCNET or MS/TP frame
    else if (max_apdu <= 480)
        octet |= 0x03;
    else if (max_apdu <= 1024)
        octet |= 0x04;
    // fits in an ISO 8802-3 frame
    else if (max_apdu <= 1476)
        octet |= 0x05;

    return octet;
}

/* function that creates an NPDU ready to be sent from the APDU */
static int send_npdu_raw(struct BACnet_NPDU *npdu,
    unsigned char *apdu, int apdu_len)
{
    int eth_rv = 0;
    int ip_rv = 0;
    int invokeID = 0;

    assert(npdu);
    assert(apdu);
    /* load APDU passed to function into the NPDU */
    if (apdu[0] == 0x00)        /* a confirmed request */
        npdu->expecting_reply = 1;
    else                        /* some other service */
        npdu->expecting_reply = 0;
    /* determine control byte */
    npdu->control_byte =
        npdu->network_message * 128 + npdu->dest_present * 32 +
        npdu->src_present * 8 + npdu->expecting_reply * 4;
    /* a reply is expected (Invoke ID is needed) */
    if (npdu->expecting_reply) {
        invokeID = invoke_id();
        /* that's no good */
        if (invokeID < 0) {
            npdu_free(npdu);
            error_printf("send_npdu: invalid invoke id!\n");
            return 0;
        }
        /* max APDU accepted in response */
        apdu[1] = get_max_seg_max_apdu(0, MAX_APDU);
        apdu[2] = invokeID;
        invoke_id_send_npdu(invokeID, npdu, apdu, apdu_len);
        debug_printf(3, "send_npdu: Adding Invoke ID %d for %s\n",
            invokeID, hwaddrtoa(npdu->dest.mac));
    }

    /* send 802.2 packet */
    if (ethernet_valid() &&
        (memcmp(npdu->dest.mac, Ethernet_Empty_MAC,
                sizeof(npdu->dest.mac)) != 0)) {
#ifdef SEND_NPDU_LOCAL
        // if we are sending to ourselves, shortcut the system...
        if (memcmp(npdu->dest.mac, BACnet_Device_MAC_Address,
                sizeof(npdu->dest.mac)) == 0) {
            receive_apdu(apdu, apdu_len, &npdu->dest);
        } else
#endif
        {
            debug_printf(3, "send_npdu: sending b/eth packet to %s\n",
                hwaddrtoa(npdu->dest.mac));
            /* 1 is returned if send succeeded */
            eth_rv = ethernet_send(npdu, apdu, apdu_len);
        }
    }
    if ((bip_sockfd >= 0) && (npdu->dest.ip.s_addr > 0)) {
        // if we are sending to ourselves, shortcut the system...
#ifdef SEND_NPDU_LOCAL
        if (npdu->dest.ip.s_addr == BACnet_Device_IP_Address.s_addr) {
            receive_apdu(apdu, apdu_len, &npdu->dest);
        } else
#endif
        {
            debug_printf(3, "send_npdu: sending bip packet to %s\n",
                inet_ntoa(npdu->dest.ip));
            /* send b/ip packet */
            ip_rv = send_bip(npdu, apdu, apdu_len);
        }
    }

    return (ip_rv || eth_rv);
}

/* function that creates an NPDU ready to be sent from the APDU 
   for devices that are not registered via I-Am */
int send_npdu_address(struct BACnet_Device_Address *dest,
    unsigned char *apdu, int apdu_len)
{
    struct BACnet_NPDU *npdu;
    // does this exceed our APDU limit?
    if (apdu_len > MAX_APDU) {
        error_printf("send_npdu: apdu exceeds maximum of %d bytes\n",
            MAX_APDU);
        return -1;              /* this packet is too large to send */
    }
    // get a packet for sending
    npdu = npdu_alloc();
    if (!npdu) {
        error_printf("send_npdu: unable to allocate memory for packet\n");
        return 0;
    }
    npdu_send_init(npdu);
    npdu->local_broadcast = false;      // for B/IP BVLL
    memmove(npdu->dest.mac, dest->mac, sizeof(npdu->dest.mac));
    memmove(&npdu->dest.ip, &dest->ip, sizeof(npdu->dest.ip));
    debug_printf(3, "send_npdu: dest.mac=%s dest.ip=%s\n",
        hwaddrtoa(npdu->dest.mac), inet_ntoa(npdu->dest.ip));
    if (dest->local) {          /* local destination */
        npdu->dest_present = 0;
        npdu->src_present = 0;
        debug_printf(3, "send_npdu: local destination\n");
    } else {                    /* remote network destination */
        npdu->dest_present = 1;
        npdu->dest.net = dest->net;
        npdu->dest.len = dest->len;     /* length of destination address */
        memmove(npdu->dest.adr, dest->adr, sizeof(npdu->dest.adr));
        // we are not routing yet...
        npdu->src_present = 0;
        debug_printf(3,
            "send_npdu: dest.net=%d dest.len=%d dest.adr=%s\n",
            npdu->dest.net, npdu->dest.len, hwaddrtoa(npdu->dest.adr));
        debug_printf(3,
            "send_npdu: src.net=%d src.len=%d src.adr=%s\n",
            npdu->src.net, npdu->src.len, hwaddrtoa(npdu->src.adr));
    }

    return (send_npdu_raw(npdu, apdu, apdu_len));
}

/* function that creates an NPDU ready to be sent from the APDU */
int send_npdu(int dest_device, unsigned char *apdu, int apdu_len)
{
    int rv = 0;                 // return value
    struct BACnet_NPDU *npdu;
    struct BACnet_Device_Info *dev_ptr = NULL;
    // does this exceed our APDU limit?
    if (apdu_len > MAX_APDU) {
        error_printf("send_npdu: apdu exceeds maximum of %d bytes\n",
            MAX_APDU);
        return -1;              /* this packet is too large to send */
    }

    npdu = npdu_alloc();
    if (!npdu) {
        error_printf("send_npdu: unable to allocate memory for packet\n");
        return 0;
    }
    npdu_send_init(npdu);
    if (dest_device == BACNET_BROADCAST_ID) {
        npdu->local_broadcast = true;   // for B/IP BVLL
        memset(npdu->dest.mac, 0xFF, sizeof(npdu->dest.mac));
        memmove(&npdu->dest.ip, &Local_IP_Broadcast_Address,
            sizeof(npdu->dest.ip));
        npdu->dest_present = 1;
        npdu->src_present = 0;
        npdu->dest.net = 0xFFFF;        /* broadcast network 65535 */
        npdu->dest.len = 0x00;
        debug_printf(3,
            "send_npdu: dest.net=%d dest.len=%d\n"
            "send_npdu: dest.mac=%s dest.ip=%s\n",
            npdu->dest.net, npdu->dest.len,
            hwaddrtoa(npdu->dest.mac), inet_ntoa(npdu->dest.ip));
    } else {                    /* a destination device was specified */
        npdu->local_broadcast = false;  // for B/IP BVLL
        dev_ptr = device_get(dest_device);
        if (dev_ptr) {
            if (apdu_len > dev_ptr->max_apdu) {
                error_printf("send_npdu: apdu too big [%d bytes]"
                    " for destination [%d bytes]\n", apdu_len,
                    dev_ptr->max_apdu);
                npdu_free(npdu);
                send_abort(dest_device, (uint8_t) invoke_id,
                    ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
                return -1;      /* this packet is too large to send */
            }
            memmove(npdu->dest.mac, dev_ptr->src.mac,
                sizeof(npdu->dest.mac));
            memmove(&npdu->dest.ip, &dev_ptr->src.ip,
                sizeof(npdu->dest.ip));
            debug_printf(3, "send_npdu: dest.mac=%s dest.ip=%s\n",
                hwaddrtoa(npdu->dest.mac), inet_ntoa(npdu->dest.ip));
            if (dev_ptr->src.local) {   /* local destination */
                npdu->dest_present = 0;
                npdu->src_present = 0;
                debug_printf(3, "send_npdu: local destination\n");
            } else {            /* remote network destination */

                npdu->dest_present = 1;
                npdu->dest.net = dev_ptr->src.net;
                npdu->dest.len = dev_ptr->src.len;      /* length of destination address */
                memmove(npdu->dest.adr, dev_ptr->src.adr,
                    sizeof(npdu->dest.adr));
                // we are not routing yet...
                npdu->src_present = 0;
                //npdu->src.net = BACnet_Network_Number;
                //npdu->src.len = 6; /* length of local MAC address */
                //memmove(npdu->src.adr, npdu->src.mac, sizeof(npdu->src.adr));
                debug_printf(3,
                    "send_npdu: dest.net=%d dest.len=%d dest.adr=%s\n",
                    npdu->dest.net, npdu->dest.len,
                    hwaddrtoa(npdu->dest.adr));
                debug_printf(3,
                    "send_npdu: src.net=%d src.len=%d src.adr=%s\n",
                    npdu->src.net, npdu->src.len,
                    hwaddrtoa(npdu->src.adr));
            }

        }
        /* the specified device is unknown */
        else {

            error_printf("send_npdu: I don't know address of device %d.\n",
                dest_device);
            npdu_free(npdu);
            return 0;           /* that's no good */
        }
    }
    rv = send_npdu_raw(npdu, apdu, apdu_len);
    npdu_free(npdu);
    return rv;
}
