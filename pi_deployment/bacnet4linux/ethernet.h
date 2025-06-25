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

 Modified by Steve Karg <skarg@users.sourceforge.net> 22 Dec 2004
 -------------------------------------------
####COPYRIGHTEND####*/
#ifndef ETHERNET_H
#define ETHERNET_H

#include "os.h"
#include "bacnet_const.h"
#include "bacnet_struct.h"

// commonly used comparison address for ethernet
extern uint8_t Ethernet_Broadcast[MAX_MAC_LEN];
// commonly used empty address for ethernet quick compare
extern uint8_t Ethernet_Empty_MAC[MAX_MAC_LEN];
// my local device data - MAC address
extern uint8_t Ethernet_MAC_Address[MAX_MAC_LEN];

int ethernet_receive(int eth802_sockfd);
int ethernet_send(struct BACnet_NPDU *npdu, uint8_t * apdu, int apdu_len);
int ethernet_bind(struct sockaddr *eth_addr, char *interface_name);
bool ethernet_valid(void);
int ethernet_socket(void);
bool ethernet_init(char *interface_name);
bool ethernet_cleanup(void);

#endif
