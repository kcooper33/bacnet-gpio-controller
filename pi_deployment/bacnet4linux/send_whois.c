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
#include "debug.h"

/* quick function to send a Who-Is */
void send_whois(int instance)
{
    unsigned char apdu[20];     /* this APDU is known to be less than 20 bytes */
    int apdu_len = 2;           /* unless otherwise determined */

    debug_printf(5, "send_whois: entered\n");

    /* broadcast Who-Is request */
    apdu[0] = 0x10;             /* unconfirmed service */
    apdu[1] = 0x08;             /* who-is */

    if (instance > -1) {        /* a device instance was specified */
        if ((instance >= 0) && (instance <= 255)) {
            apdu[2] = 0x09;     /* context tag 0 of unsigned of length 1 */
            apdu[3] = instance;
            apdu[4] = 0x19;     /* context tag 1 of unsigned of length 1 */
            apdu[5] = instance;
            apdu_len = 6;
        }
        if ((instance >= 256) && (instance <= 65535)) {
            apdu[2] = 0x0A;     /* context tag 0 of unsigned of length 2 */
            apdu[3] = (int) (instance / 256);
            apdu[4] = instance % 256;
            apdu[5] = 0x1A;     /* context tag 1 of unsigned of length 2 */
            apdu[6] = (int) (instance / 256);
            apdu[7] = instance % 256;
            apdu_len = 8;
        }
        if ((instance >= 65536) && (instance <= 4194303)) {     /* BACnet limit */
            apdu[2] = 0x0B;     /* context tag 0 of unsigned of length 3 */
            apdu[3] = (int) (instance / 65536);
            apdu[4] = (int) ((instance % 65536) / 256); /* remainder div 256 */
            apdu[5] = instance % 256;
            apdu[6] = 0x1B;     /* context tag 1 of unsigned of length 3 */
            apdu[7] = (int) (instance / 65536);
            apdu[8] = (int) ((instance % 65536) / 256); /* remainder div 256 */
            apdu[9] = instance % 256;
            apdu_len = 10;
        }
        debug_printf(2, "send_whois: sending to device %d\n", instance);
    } else                      /* no device was specified */
        debug_printf(2, "send_whois: sending to all devices\n");

    /* send who-is broadcast */
    send_npdu(-1, &apdu[0], apdu_len);
}

/* end of who-is */

/* end of send_whois.c */
