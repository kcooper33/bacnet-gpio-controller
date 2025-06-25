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
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

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

#ifndef OPTIONS_H
#define OPTIONS_H

// common global configuration options
// the device instance that this will be (0-4194303)
extern int BACnet_Device_Instance;
// APDU timeout (for retries) in seconds 
extern int BACnet_APDU_Timeout;
// the BACnet Vendor ID that will be used (0 == ASHRAE)
// Note: ASHRAE maintains the list of Vendor Ids - see them for one.
extern int BACnet_Vendor_Identifier;
// the ethernet interface that will be used for 802.2
extern char BACnet_Device_Interface[MAX_INTERFACE_CHARS];
// port for BACnet/IP (47808 = 0xBAC0 is the default BACnet/IP port)
extern int BACnet_UDP_Port;
// port to run MS/TP state machine from
extern int BACnet_MSTP_Port;
// This flag enables the BACnet Ethernet 802.2 (enable=1, disable=0)
extern int BACnet_Ethernet_Enable;

void options_interpret_arguments(int argc, char *argv[]);
void options_usage(void);
void options_default(void);

#endif
