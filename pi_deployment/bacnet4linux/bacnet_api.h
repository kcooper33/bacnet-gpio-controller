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
#ifndef BACNET_API_H
#define BACNET_API_H

#include <stdint.h>
#include "bacnet_enum.h"
#include "bacnet_struct.h"

/* BACnet services */
int read_property(int device, enum BACnetObjectType object,
    int instance, enum BACnetPropertyIdentifier property,
    int PropertyArrayIndex);

int subscribe_cov(int device, enum BACnetObjectType type, int instance);

int write_property(int device, enum BACnetObjectType object,
    int instance, enum BACnetPropertyIdentifier property,
    unsigned char valuetype, union ObjectValue value);

void send_whois(int instance);
void send_iam(int instance, unsigned short vendor_id);
void time_synch(void);

/* specialty functions */
int check_online_status(void);

/* query newly found devices for device object properties. */
int query_new_device(void);

/* returns valid BACnet properties for an object type */
enum BACnetPropertyIdentifier *getobjectprops(enum BACnetObjectType
    object);

/* receiving functions */
void receive_bip(void);
void receive_http(void);
int receive_npdu(struct BACnet_NPDU *npdu);
int receive_apdu(uint8_t * apdu, int apdu_len,
    struct BACnet_Device_Address *src);
void receive_IAm(uint8_t * apdu, int apdu_len,
    struct BACnet_Device_Address *src);
int receive_readproperty(uint8_t * service_request, int service_len,
    struct BACnet_Device_Address *src, int src_max_apdu,
    uint8_t invoke_id);
int receive_readpropertyACK(uint8_t * service_request, int service_len,
    struct BACnet_Device_Address *src);
int receive_COV(uint8_t * apdu, int apdu_len,
    struct BACnet_Device_Address *src);

/* generic send */
int send_npdu(int dest_device, unsigned char *apdu, int apdu_len);
int send_npdu_address(struct BACnet_Device_Address *dest,
    unsigned char *apdu, int apdu_len);

//B/IP send function
int send_bip(struct BACnet_NPDU *npdu, uint8_t * apdu, int apdu_len);


#endif
