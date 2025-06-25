/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2004 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to 
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330 
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/

#include "os.h"
#include "bacnet_const.h"
#include "bacnet_struct.h"
#include "main.h"
#include "net.h"
#include "ethernet.h"
#include "debug.h"

struct BACnet_NPDU *npdu_alloc(void)
{
    return calloc(1, sizeof(struct BACnet_NPDU));
}

void npdu_send_init(struct BACnet_NPDU *npdu)
{
    if (npdu) {
        /* fill in the NPDU structure with default values */
        npdu->hop_count = 255;  /* was -1 */
        npdu->message_type = -1;        /* this is used for network messages, but currently, we don't send any */
        npdu->vendor_id = -1;   /* this is used for proprietary network messages */
        npdu->net_priority = 0; /* default to lowest priority */
        npdu->network_message = 0;      /* default to APDU present */
        npdu->dest_present = 0;
        npdu->src_present = 0;
        npdu->dest.net = 0;
        npdu->dest.len = 0;
        npdu->src.net = 0;
        npdu->src.len = 0;
        npdu->version = 0x01;

        memmove(npdu->src.mac, Ethernet_MAC_Address,
            sizeof(npdu->src.mac));
        npdu->src.ip.s_addr = BACnet_Device_IP_Address.s_addr;
        debug_printf(3, "send_npdu: src.mac=%s src.ip=%s\n",
            hwaddrtoa(npdu->src.mac), inet_ntoa(npdu->src.ip));
    }
}


void npdu_free(struct BACnet_NPDU *npdu)
{
    if (npdu)
        free(npdu);
}


unsigned char *pdu_alloc(void)
{
    return calloc(1, MAX_APDU);
}

void pdu_free(unsigned char *pdu)
{
    if (pdu)
        free(pdu);
}
