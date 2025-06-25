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
//
// This module handles the BACnet device object cache.
//
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bacnet_const.h"
#include "bacnet_struct.h"
#include "bacnet_object.h"
#include "bacnet_text.h"
#include "keylist.h"
#include "debug.h"

static OS_Keylist Device_List = NULL;   // handle to the list of devices

static void check_device_list(void)
{
    // is the list created yet?
    if (!Device_List)
        Device_List = Keylist_Create();
    // did it get created?
    if (!Device_List)
        error_printf("BACnet_Device: Unable to create list.\n");
}

int device_count(void)
{
    check_device_list();

    return Keylist_Count(Device_List);
}

struct BACnet_Device_Info *device_record(int device_index)
{
    check_device_list();

    return Keylist_Data_Index(Device_List, device_index);
}

struct BACnet_Device_Info *device_get(int device_id)    // device instance number
{
    check_device_list();

    return Keylist_Data(Device_List, device_id);
}

struct BACnet_Device_Info *device_new(int device_id)    // device instance number
{
    struct BACnet_Device_Info *dev_ptr;

    check_device_list();
    // does this device already exist?
    dev_ptr = Keylist_Data(Device_List, device_id);
    if (dev_ptr)
        memset(dev_ptr, 0, sizeof(struct BACnet_Device_Info));
    else {
        dev_ptr = calloc(1, sizeof(struct BACnet_Device_Info));
        if (dev_ptr) {
            dev_ptr->object_list = Keylist_Create();
            Keylist_Data_Add(Device_List, device_id, dev_ptr);
        } else
            error_printf("Device: Unable to allocate device %d buffer\n",
                device_id);
    }

    return dev_ptr;
}


// adds a new device, or returns an existing device
struct BACnet_Device_Info *device_add(int device_id)    // device instance number
{
    struct BACnet_Device_Info *dev_ptr;

    dev_ptr = device_get(device_id);
    if (!dev_ptr) {
        dev_ptr = device_new(device_id);
        if (dev_ptr) {
            dev_ptr->state = 0;
            dev_ptr->device = device_id;
            // at least one device object in the list
            dev_ptr->true_num_objects = 1;
        }
    }
    return dev_ptr;
}


/* returns the device instance for a given npdu */
int device_which_sent(struct BACnet_Device_Address *src)
{
    int device_id = -1;         // return value, -1 if not found
    struct BACnet_Device_Info *dev_ptr;
    int device_count;           // how many are in our device list
    int i;                      // counter 

    check_device_list();
    /* an NPDU is passed in, and an attempt to match it to known devices is done */
    device_count = Keylist_Count(Device_List);
    for (i = 0; i < device_count; i++) {
        dev_ptr = Keylist_Data_Index(Device_List, i);
        /* correct source MAC */
        if (memcmp(src->mac, dev_ptr->src.mac, MAX_MAC_LEN) == 0) {
            /* a local device, since no source was given */
            // FIXME: inconsistent
            if ((src->net == -1) && (dev_ptr->src.local)) {
                device_id = dev_ptr->device;
                break;
            }
            /* a routed device */
            else {
                if ((memcmp(src->adr, dev_ptr->src.adr, MAX_MAC_LEN) == 0)
                    && (src->net == dev_ptr->src.net)) {
                    device_id = dev_ptr->device;
                    break;
                }
            }
        }
    }
    return device_id;
}

void device_init(void)
{
    check_device_list();
}

/* function to remove a known device and free all allocated memory */
// note: since we remove the device out of the list, we can't use
// the device_ and object_ functions here since it is no longer on
// the list.  Just use Keylist functions directly.
void device_record_remove(int device_record)
{
    struct ObjectRef_Struct *obj_ptr;
    struct BACnet_Device_Info *dev_ptr;

    check_device_list();
    dev_ptr = Keylist_Data_Delete_By_Index(Device_List, device_record);
    if (dev_ptr) {
        debug_printf(2, "Device: Removing %d\n", dev_ptr->device);
        if (dev_ptr->device_name)
            free(dev_ptr->device_name);
        if (dev_ptr->object_list) {
            debug_printf(3, "Device: Has an object list %d\n",
                dev_ptr->device);
            // pull first one off list until there are no more.
            while ((obj_ptr =
                    Keylist_Data_Delete_By_Index(dev_ptr->object_list,
                        0))) {
                debug_printf(4, "Device: Removing Device %d %s %d\n",
                    dev_ptr->device, enum_to_text_object(obj_ptr->type),
                    obj_ptr->instance);
                // cleanup any names created
                if (obj_ptr->name) {
                    /* free the memory to store the object name */
                    free(obj_ptr->name);
                    /* guarantee that the same memory isn't freed twice */
                    obj_ptr->name = NULL;
                }
                if ((obj_ptr->type == OBJECT_BINARY_INPUT) ||
                    (obj_ptr->type == OBJECT_BINARY_OUTPUT) ||
                    (obj_ptr->type == OBJECT_BINARY_VALUE) ||
                    (obj_ptr->type == OBJECT_CALENDAR) ||
                    (obj_ptr->type == OBJECT_SCHEDULE)) {
                    if (obj_ptr->units.states.active) {
                        /* free the memory to store the object name */
                        free(obj_ptr->units.states.active);
                        /* guarantee that the same memory isn't freed twice */
                        obj_ptr->units.states.active = NULL;
                    }
                    if (obj_ptr->units.states.inactive) {
                        /* free the memory to store the object name */
                        free(obj_ptr->units.states.inactive);
                        /* guarantee that the same memory isn't freed twice */
                        obj_ptr->units.states.inactive = NULL;
                    }
                }
                free(obj_ptr);
            }
            Keylist_Delete(dev_ptr->object_list);
        } else {
            debug_printf(3, "Device: no object list for Device %d.\n",
                dev_ptr->device);
        }
        free(dev_ptr);
    } else {
        debug_printf(2, "Device: no data to remove for index %d.\n",
            device_record);
    }
}

// used at the end of the run to clean up resources
void device_cleanup(void)
{
    int num_devices, i;

    num_devices = Keylist_Count(Device_List);
    for (i = 0; i < num_devices; i++) {
        /* keep yanking record 0 out of the list, 
           until there are no records left */
        device_record_remove(0);
    }
    Keylist_Delete(Device_List);
    debug_printf(1, "Device: Removed %d devices\n", num_devices);

    return;
}
