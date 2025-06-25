/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (c) 2004 Steve Karg <skarg@users.sourceforge.net> 

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
#ifndef MAIN_H
#define MAIN_H

#include "bacnet_const.h"

/* TCP socket file handles */
extern int http_sockfd;
extern int bip_sockfd;          /* BACnet/IP file handle */
extern int mstp_sockfd;         /* MS/TP loopback file handle */

// global configuration options
// use COV for getting the Preset_Value from each object
extern int BACnet_COV_Support;
// COV Lifetime indicates how often we renew our subscription (seconds)
extern int BACnet_COV_Lifetime;
// number of concurrent queries (invoke ids)
extern int BACnet_Invoke_Ids;
// port to run HTTP server on (80 is the normal WWW port)
extern int BACnet_HTTP_Port;
// A time master sends a global (or local) time sync to others     
// This value will send the time sync every x seconds when     
// x seconds is evenly divisible. If x seconds is zero, this
// is disabled     
extern int BACnet_Time_Sync_Seconds;
// initial polling delay is how long to wait to begin 
// querying new devices (in seconds, 0=disable query)
extern int BACnet_Initial_Query_Delay;
// my local device data - MAC address
extern struct in_addr BACnet_Device_IP_Address;
// stores the local IP broadcast address which varies depending on subnet
extern struct in_addr Local_IP_Broadcast_Address;

//  filenames for save/restore operations
extern char *readFile;
extern char *writeFile;
extern int exclusiveDevice;

// linked list of devices to accept I-Am's from
struct deviceRange {
    int min;
    int max;
    struct deviceRange *next;
};

extern struct deviceRange *deviceRangeList;


#endif
