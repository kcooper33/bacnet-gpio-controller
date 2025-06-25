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
// Functions to handle finding an object in the list, and putting it back. 
// Used to update values, names, etc. 
//
#include <string.h>
#include <stdlib.h>

#include "bacnet_struct.h"
#include "bacnet_object.h"
#include "bacnet_device.h"
#include "bacnet_text.h"
#include "keylist.h"
#include "key.h"
#include "debug.h"

/* this function finds and returns the object from the device object list */
struct ObjectRef_Struct *object_fetch_by_index(int device_id, int index)
{
    struct BACnet_Device_Info *dev_ptr = NULL;
    struct ObjectRef_Struct *obj_ptr = NULL;    // return value 

    debug_printf(5, "Object: Entered 'object_fetch_by_index'\n");

    dev_ptr = device_get(device_id);
    if (dev_ptr && dev_ptr->object_list)
        obj_ptr = Keylist_Data_Index(dev_ptr->object_list, index);

    return obj_ptr;
}

/* this function finds and returns the object from the device object list */
struct ObjectRef_Struct *object_get_by_index(struct BACnet_Device_Info
    *dev_ptr, int index)
{
    struct ObjectRef_Struct *obj_ptr = NULL;    // return value 

    debug_printf(5, "Object: Entered 'object_fetch_by_index'\n");
    if (dev_ptr && dev_ptr->object_list)
        obj_ptr = Keylist_Data_Index(dev_ptr->object_list, index);

    return obj_ptr;
}

/* this function finds and returns the object from the device object list */
struct ObjectRef_Struct *object_find(int device_id,
    enum BACnetObjectType type, int instance)
{
    struct BACnet_Device_Info *dev_ptr = NULL;
    struct ObjectRef_Struct *obj_ptr = NULL;    // return value 
    KEY key = 0;

    debug_printf(5, "Object: Entered 'object_find'\n");

    dev_ptr = device_get(device_id);
    if (dev_ptr && dev_ptr->object_list) {
        key = KEY_ENCODE(type, instance);
        obj_ptr = Keylist_Data(dev_ptr->object_list, key);
    }

    return obj_ptr;
}

/* this function finds and returns the object from the device object list 
   or, if the object is not found, it creates it. */
struct ObjectRef_Struct *object_new(int device_id,
    enum BACnetObjectType type, int instance)
{
    struct BACnet_Device_Info *dev_ptr = NULL;
    struct ObjectRef_Struct *obj_ptr = NULL;    // return value 
    KEY key = 0;

    debug_printf(5, "Object: Entered 'object_new' for device %d\n",
        device_id);

    dev_ptr = device_get(device_id);
    if (dev_ptr && dev_ptr->object_list) {
        debug_printf(3, "Object: Found Device %d in list.\n", device_id);
        // try to find existing key first since we unique data
        key = KEY_ENCODE(type, instance);
        obj_ptr = Keylist_Data(dev_ptr->object_list, key);
        if (!obj_ptr) {
            debug_printf(3,
                "Object: %s %d is not in ObjectList in Device %d.\n",
                enum_to_text_object(type), instance, dev_ptr->device);
            obj_ptr = calloc(1, sizeof(struct ObjectRef_Struct));
            if (obj_ptr) {
                // FIXME: type and instance are redundant if we are using
                // the key (made from type and instance) as the storage index
                obj_ptr->type = type;
                obj_ptr->instance = instance;
                if ((type == OBJECT_BINARY_INPUT) ||
                    (type == OBJECT_BINARY_OUTPUT) ||
                    (type == OBJECT_BINARY_VALUE) ||
                    (type == OBJECT_CALENDAR)
                    || (type == OBJECT_SCHEDULE)) {
                    obj_ptr->units.states.active = strdup("Active");
                    obj_ptr->units.states.inactive = strdup("Inactive");
                }
                (void) Keylist_Data_Add(dev_ptr->object_list, key,
                    obj_ptr);
                debug_printf(2,
                    "Object: Added %s %d to ObjectList in Device %d.\n",
                    enum_to_text_object(obj_ptr->type), obj_ptr->instance,
                    dev_ptr->device);
            } else {
                debug_printf(1,
                    "Object: Failed to add %s %d to ObjectList in Device %d.\n",
                    enum_to_text_object(type), instance, dev_ptr->device);
            }
        } else {
            debug_printf(3,
                "Object: %s %d already exists in ObjectList in Device %d.\n",
                enum_to_text_object(obj_ptr->type), obj_ptr->instance,
                dev_ptr->device);
        }
    }

    return obj_ptr;
}

int object_count(int device_id)
{
    struct BACnet_Device_Info *dev_ptr = NULL;
    int count = 0;              // return value

    debug_printf(5, "Object: Entered 'object_count'\n");
    dev_ptr = device_get(device_id);
    if (dev_ptr && dev_ptr->object_list)
        count = Keylist_Count(dev_ptr->object_list);

    return count;
}

int object_total_count(void)
{
    struct BACnet_Device_Info *dev_ptr = NULL;  // used for device
    int count = 0;              // return value
    int num_devices = 0;        // total number of device objects
    int i = 0;                  // counter

    num_devices = device_count();
    for (i = 0; i < num_devices; i++) {
        dev_ptr = device_record(i);
        if (!dev_ptr)
            continue;
        if (!dev_ptr->object_list)
            continue;
        count += Keylist_Count(dev_ptr->object_list);
    }

    return count;
}

#ifdef TEST
#include <assert.h>
#include <string.h>

#include "ctest.h"

// test the encode and decode macros
void testObjectList(Test * pTest)
{
    struct BACnet_Device_Info *dev_ptr = NULL;  // for device info
    struct ObjectRef_Struct *obj_ptr = NULL;    // temporary objectref
    int device_id = 42;
    float real_number = 123.456;
    enum BACnetEngineeringUnits units = UNITS_PER_MINUTE;
    enum BACnetObjectType type = OBJECT_ANALOG_OUTPUT;
    int object_id = 999;
    int object_id2 = 23456;

    device_init();
    dev_ptr = device_add(device_id);
    ct_test(pTest, dev_ptr != NULL);
    if (dev_ptr) {
        obj_ptr = object_new(device_id, type, object_id);
        ct_test(pTest, obj_ptr != NULL);
        if (obj_ptr) {
            obj_ptr->units.units = units;
            obj_ptr->value.real = real_number;
            ct_test(pTest, obj_ptr->type == type);
            ct_test(pTest, obj_ptr->instance == object_id);
        }
        obj_ptr = object_new(device_id, type, object_id);
        ct_test(pTest, obj_ptr != NULL);
        if (obj_ptr) {
            obj_ptr->units.units = units;
            obj_ptr->value.real = real_number;
            ct_test(pTest, obj_ptr->type == type);
            ct_test(pTest, obj_ptr->instance == object_id);
        }
        obj_ptr = object_new(device_id, type, object_id2);
        ct_test(pTest, obj_ptr != NULL);
        if (obj_ptr) {
            obj_ptr->units.units = units;
            obj_ptr->value.real = real_number;
            ct_test(pTest, obj_ptr->type == type);
            ct_test(pTest, obj_ptr->instance == object_id2);
        }
        obj_ptr = object_find(device_id, type, object_id);
        ct_test(pTest, obj_ptr != NULL);
        if (obj_ptr) {
            ct_test(pTest, obj_ptr->units.units == units);
            ct_test(pTest, obj_ptr->value.real == real_number);
            ct_test(pTest, obj_ptr->type == type);
            ct_test(pTest, obj_ptr->instance == object_id);
        }
        obj_ptr = object_find(device_id, type, object_id2);
        ct_test(pTest, obj_ptr != NULL);
        if (obj_ptr) {
            ct_test(pTest, obj_ptr->units.units == units);
            ct_test(pTest, obj_ptr->value.real == real_number);
            ct_test(pTest, obj_ptr->type == type);
            ct_test(pTest, obj_ptr->instance == object_id2);
        }
        obj_ptr = object_new(device_id, type, object_id);
        ct_test(pTest, obj_ptr != NULL);
        if (obj_ptr) {
            obj_ptr->units.units = units;
            obj_ptr->value.real = real_number;
            ct_test(pTest, obj_ptr->type == type);
            ct_test(pTest, obj_ptr->instance == object_id);
        }
        obj_ptr = object_find(device_id, type, object_id);
        ct_test(pTest, obj_ptr != NULL);
        if (obj_ptr) {
            ct_test(pTest, obj_ptr->units.units == units);
            ct_test(pTest, obj_ptr->value.real == real_number);
            ct_test(pTest, obj_ptr->type == type);
            ct_test(pTest, obj_ptr->instance == object_id);
        }
    }
    device_cleanup();

    return;
}

#ifdef TEST_OBJECT_LIST
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("object list", NULL);

    /* individual tests */
    rc = ct_addTestFunction(pTest, testObjectList);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);

    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_KEYLIST */
#endif                          /* TEST */
