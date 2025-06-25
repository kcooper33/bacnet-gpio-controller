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
#ifndef BACNET_OBJECT_H
#define BACNET_OBJECT_H

/* this function finds and returns the object from the device object list */
struct ObjectRef_Struct *object_find(int device_id,
    enum BACnetObjectType type, int instance);

/* this function finds and returns the object from the device object list 
   or, if the object is not found, it creates it. */
struct ObjectRef_Struct *object_new(int device_id,
    enum BACnetObjectType type, int instance);

struct ObjectRef_Struct *object_fetch_by_index(int device_id, int index);
struct ObjectRef_Struct *object_get_by_index(struct BACnet_Device_Info
    *dev_ptr, int index);

int object_count(int device_id);
int object_total_count(void);

#endif
