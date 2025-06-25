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
#ifndef BACNET_DEVICE_H
#define BACNET_DEVICE_H

#include "bacnet_struct.h"

/* device record functions */
int device_count(void);
void device_record_remove(int device_record);
void device_init(void);
struct BACnet_Device_Info *device_record(int device_index);
int device_which_sent(struct BACnet_Device_Address *src_address);
struct BACnet_Device_Info *device_get(int device_id);   // device instance number
// creates a new device, or inits an existing device
struct BACnet_Device_Info *device_new(int device_id);   // device instance number
// adds a new device, or returns an existing device
struct BACnet_Device_Info *device_add(int device_id);   // device instance number
void device_cleanup(void);

#endif
