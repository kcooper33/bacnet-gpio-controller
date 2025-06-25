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
#include <stdio.h>
#include "bacnet_struct.h"
#include "debug.h"
#include "net.h"

/* this function will display the contents of a packet (for debugging) */
void display_packet(struct BACnet_NPDU *npdu, uint8_t * apdu, int apdu_len)
{
    int i;

    debug_printf(5, "display_packet: entered\n");

    printf("NPDU:\n");
    printf("Source ethernet MAC:      %s\n", hwaddrtoa(npdu->src.mac));
    printf("Destination ethernet MAC: %s\n", hwaddrtoa(npdu->dest.mac));
    printf("Control byte:             0x%02X\n", npdu->control_byte);
    printf("Network message:          0x%02X\n", npdu->network_message);
    printf("Destination present:      0x%02X\n", npdu->dest_present);
    printf("Source present:           0x%02X\n", npdu->src_present);
    printf("Expecting reply:          0x%02X\n", npdu->expecting_reply);
    printf("Net priority:             0x%02X\n", npdu->net_priority);
    printf("Destination network:      %dd\n", npdu->dest.net);
    printf("Destination length:       %dd\n", npdu->dest.len);
    printf("Destination MAC address:  %s\n", hwaddrtoa(npdu->dest.adr));
    printf("Source network:           %dd\n", npdu->src.net);
    printf("Source length:            %dd\n", npdu->src.len);
    printf("Source MAC address:       %s\n", hwaddrtoa(npdu->src.adr));
    printf("Hop count:                %dd\n", npdu->hop_count);
    printf("Message type:             0x%X\n", npdu->message_type);
    printf("Vendor ID:                %dd\n", npdu->vendor_id);
    printf("APDU length:              %dd\n", apdu_len);

    printf("APDU: ");
    if (apdu) {
        for (i = 0; i < apdu_len; i++) {        /* show apdu */
            if (!(i % 16))
                printf("\n%04X: ", i);  /* show offset */
            printf("%02X ", apdu[i]);
        }
        printf("\n");
    }
}
