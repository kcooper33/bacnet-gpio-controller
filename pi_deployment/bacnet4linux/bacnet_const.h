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
#ifndef BACNET_CONST_H
#define BACNET_CONST_H

#define DEFAULT_MTU 1514        /* max size of an Ethernet frame */
#define MAX_APDU 480            /* size of maximum APDU in bytes */
#define MAX_MAC_LEN 6           // length of hardware MAC address in bytes

#define BACNET_MAX_ID 4194303L  // last valid BACnet instance number
#define BACNET_ARRAY_ALL (~0)
#define BACNET_BROADCAST_ID (-1)
#define BACNET_COV_PROCESS_ID (10)      // arbitrary, but common

//CLB Moved here from main.c
#define MAX_INTERFACE_CHARS 255

/* BACnet parameters */
//#define DEVICEINSTANCE 2 /* the device instance that this will be (0-4194303) */
//#define NETNUM 0      /* the BACnet network number that this is connected to (leave as 0 in most cases [0-65534]) */
//#define APDUTIMEOUT 10      /* APDU timeout (for retries) in seconds */
//#define VENDORID 0    /* the BACnet Vendor ID that will be used (0 == ASHRAE) */

/* limits */
#define MAXINVOKEIDS 255        /* the maximum number of invoke IDs that can be in use at once (20-255) 
                                   (this can be used to save memory) */

/* debugging parameters */
// #define DEBUG        /* if defined, additional debug messages will be outputted */
// #define OUTPUTPACKETS      /* if defined, the contents of packets received and sent will be shown */
#define ONLYDEVICEOBJECT        /* if defined, the Device created by BACnet4Linux will only have a Device object */

/* these are just an easy way to identify primitive data types */
#define PRIMATIVE_BINARY     0x10
#define PRIMATIVE_UNSIGNED   0x20
#define PRIMATIVE_REAL       0x40
#define PRIMATIVE_STRING     0x70
#define PRIMATIVE_ENUMERATED 0x90

//BVLL Function definitions
#define Write_Broadcast_Distribution_Table     0x01
#define Read_Broadcast_Distribution_Table      0x02
#define Read_Broadcast_Distribution_Table_ACK  0x03
#define Forwarded_NPDU                         0x04
#define Register_Foreign_Device                0x05
#define Read_Foreign_Device_Table              0x06
#define Read_Foreign_Device_Table_ACK          0x07
#define Delete_Foreign_Device_Table_Entry      0x08
#define Distribute_Broadcast_To_Network        0x09
#define Original_Unicast_NPDU                  0x0A
#define Original_Broadcast_NPDU                0x0B

#endif
