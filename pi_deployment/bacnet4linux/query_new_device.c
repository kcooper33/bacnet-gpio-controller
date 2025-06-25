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
// Query newly found devices for device object properties.
// Subscribe COV to objects where it makes sense
//
#include "os.h"
#include "bacnet_struct.h"
#include "bacnet_enum.h"
#include "bacnet_const.h"
#include "bacnet_api.h"
#include "bacnet_device.h"
#include "bacnet_object.h"
#include "bacnet_text.h"
#include "invoke_id.h"
#include "main.h"
#include "options.h"
#include "debug.h"

// get the present value by means other than COV - poll the present-value
static time_t device_request_present_value(struct BACnet_Device_Info
    *dev_ptr)
{
    struct ObjectRef_Struct *obj_ptr;   /* temporary objectref */
    time_t t;                   /* time_h storage for time */
    time_t delta_time = 1;      /* time_h storage for time */


    t = time(NULL);             /* find current time */
    obj_ptr = object_get_by_index(dev_ptr, dev_ptr->object_index);
    if (obj_ptr) {
        dev_ptr->object_index++;
        /* time to request a new present-value */
        delta_time = t - obj_ptr->last_subscribe_COV;
        if (delta_time > BACnet_COV_Lifetime) {
            if ((obj_ptr->type == OBJECT_ANALOG_VALUE)
                || (obj_ptr->type == OBJECT_ANALOG_INPUT)
                || (obj_ptr->type == OBJECT_ANALOG_OUTPUT)
                || (obj_ptr->type == OBJECT_BINARY_VALUE)
                || (obj_ptr->type == OBJECT_BINARY_INPUT)
                || (obj_ptr->type == OBJECT_BINARY_OUTPUT)) {
                debug_printf(3,
                    "QND: Requesting Device %d present-value for %s %d\n",
                    dev_ptr->device,
                    enum_to_text_object(obj_ptr->type), obj_ptr->instance);
                read_property(dev_ptr->device,
                    obj_ptr->type, obj_ptr->instance,
                    PROP_PRESENT_VALUE, -1 /* array index */ );
                /* set the time */
                obj_ptr->last_subscribe_COV = t;
            }
        }
    } else {
        // start over, and wait for timeout
        dev_ptr->object_index = 0;
    }

    return delta_time;
}

/* Subscribe COV to appropriate objects */
static time_t device_subscribe_cov(struct BACnet_Device_Info *dev_ptr)
{
    struct ObjectRef_Struct *obj_ptr;   /* temporary objectref */
    time_t delta_time = 1;      /* time_h storage for time */
    time_t t;                   /* time_h storage for time */

    t = time(NULL);             /* find current time */
    obj_ptr = object_get_by_index(dev_ptr, dev_ptr->object_index);
    if (obj_ptr) {
        dev_ptr->object_index++;
        /* time to re-subscribe (or never subscribed) */
        delta_time = t - obj_ptr->last_subscribe_COV;
        if (delta_time > BACnet_COV_Lifetime) {
            if ((obj_ptr->type == OBJECT_ANALOG_VALUE)
                || (obj_ptr->type == OBJECT_ANALOG_INPUT)
                || (obj_ptr->type == OBJECT_ANALOG_OUTPUT)
                || (obj_ptr->type == OBJECT_BINARY_VALUE)
                || (obj_ptr->type == OBJECT_BINARY_INPUT)
                || (obj_ptr->type == OBJECT_BINARY_OUTPUT)) {
                debug_printf(3,
                    "QND: Requesting Device %d subscribe for %s %d\n",
                    dev_ptr->device,
                    enum_to_text_object(obj_ptr->type), obj_ptr->instance);
                subscribe_cov(dev_ptr->device,
                    obj_ptr->type, obj_ptr->instance);
                /* set the time */
                obj_ptr->last_subscribe_COV = t;
            }
        }
    } else {
        // start over, and wait for timeout
        dev_ptr->object_index = 0;
    }

    return delta_time;
}

static bool query_object_property(int device_instance,
    enum BACnetObjectType object_type,
    int object_instance, enum BACnetPropertyIdentifier property)
{
    bool no_property_status = true;     // return value

    if (property != PROP_NO_PROPERTY) {
        no_property_status = false;
        debug_printf(3,
            "query: %s %d %s from Device %d\n",
            enum_to_text_object(object_type),
            object_instance,
            enum_to_text_property(property), device_instance);
        /* only ask for index size of array properties */
        if (property == PROP_OBJECT_LIST) {
            debug_printf(3,
                "query: Asking for index size of %s\n",
                enum_to_text_property(property));
            read_property(device_instance,
                object_type, object_instance, property, 0
                /* array index 0=array size */
                );
        } else {
            read_property(device_instance,
                object_type, object_instance, property, -1
                /* array index -1=no array */
                );
        }
    }

    return no_property_status;
}

/* query the ObjectList properties for values and text */
static void query_object_list_properties(struct BACnet_Device_Info
    *dev_ptr)
{
    struct ObjectRef_Struct *obj_ptr;   /* temporary objectref */
    enum BACnetPropertyIdentifier *property_list_ptr;
    enum BACnetPropertyIdentifier property;     /* pointer to an array of properties */

    if (dev_ptr) {
        obj_ptr = object_get_by_index(dev_ptr, dev_ptr->object_index);
        if (obj_ptr) {
            property_list_ptr = getobjectprops(obj_ptr->type);
            if (property_list_ptr) {
                property = *(property_list_ptr + dev_ptr->prop_count);
                if (property != PROP_NO_PROPERTY) {
                    query_object_property(dev_ptr->device,
                        obj_ptr->type, obj_ptr->instance, property);
                    dev_ptr->prop_count++;
                } else {
                    // last property, go to next object
                    dev_ptr->prop_count = 0;
                    dev_ptr->object_index++;
                }
            } else {
                // internal error?
                dev_ptr->state = DEVICE_STATE_ERROR;
            }
        } else {
            //if (dev_ptr->array_index >= dev_ptr->true_num_objects) {
            // that was the last object in the list of objects
            if (BACnet_COV_Support)
                dev_ptr->state = DEVICE_STATE_SUBSCRIBE_COV;
            else
                dev_ptr->state = DEVICE_STATE_REQUEST_PRESENT_VALUE;
            // housekeeping
            dev_ptr->object_index = 0;
            dev_ptr->prop_count = 0;
        }
    }

    return;
}

/* query the ObjectList - adds objects to our object list */
static void query_device_object_list(struct BACnet_Device_Info *dev_ptr)
{
    if (!dev_ptr)
        return;
    if (dev_ptr->true_num_objects) {
        /* find the true number of objects in the device */
        debug_printf(3,
            "query: Requesting Device %d ObjectList[%d of %d]\n",
            dev_ptr->device, dev_ptr->object_index,
            dev_ptr->true_num_objects);
        read_property(dev_ptr->device, OBJECT_DEVICE,
            dev_ptr->device, PROP_OBJECT_LIST, dev_ptr->object_index);
        dev_ptr->object_index++;
        // finished with all the properties?
        if (dev_ptr->object_index >= dev_ptr->true_num_objects) {
            dev_ptr->object_index = 0;
            dev_ptr->state = DEVICE_STATE_QUERY_OBJECT_LIST_PROPERTIES;
        }
    }

    return;
}

/* query a Device object's name and properties if it is unknown */
static void query_device_properties(struct BACnet_Device_Info *dev_ptr)
{
    enum BACnetPropertyIdentifier *property_list_ptr;   /*array of properties */
    enum BACnetPropertyIdentifier property;

    debug_printf(3, "query: Device %d properties\n", dev_ptr->device);
    /* properties of a device object */
    property_list_ptr = getobjectprops(OBJECT_DEVICE);
    if (property_list_ptr) {
        property = *(property_list_ptr + dev_ptr->prop_count);
        if (property != PROP_NO_PROPERTY) {
            query_object_property(dev_ptr->device,
                OBJECT_DEVICE, dev_ptr->device, property);
            dev_ptr->prop_count++;
        } else {
            dev_ptr->prop_count = 0;
            dev_ptr->object_index = 0;
            dev_ptr->state = DEVICE_STATE_QUERY_OBJECT_LIST;
        }
    } else {
        // internal error?
        dev_ptr->state = DEVICE_STATE_ERROR;
    }

    return;
}

/* New Devices are queried in steps... */
/* 1. following receive of I-Am... */
/*       ...the object list in the device object is asked for */
/* 2. following receive of ObjectList array size... */
/*      ...the ObjectList array is asked for one at a time */
/* 3. following receive of complete ObjectList... */
/*      ...the object properties are asked for */
/* 4. following receive of object properties... */
/*      ...appropriate objects are periodically */
/*      Subscribed to COV ... */
/*      or        */
/*      requested present-value ... */
static time_t query_device(struct BACnet_Device_Info *dev_ptr)
{
    time_t busy_time = 1;       // 0 indicates busy

    if (!dev_ptr)
        return busy_time;
    switch (dev_ptr->state) {
    case DEVICE_STATE_INIT:
        dev_ptr->state = DEVICE_STATE_QUERY_DEVICE_PROPERTIES;
        dev_ptr->object_index = 0;
        dev_ptr->prop_count = 0;
        dev_ptr->true_num_objects = 0;
        break;
    case DEVICE_STATE_QUERY_DEVICE_PROPERTIES:
        query_device_properties(dev_ptr);
        break;
    case DEVICE_STATE_QUERY_OBJECT_LIST:
        query_device_object_list(dev_ptr);
        break;
    case DEVICE_STATE_QUERY_OBJECT_LIST_PROPERTIES:
        query_object_list_properties(dev_ptr);
        break;
    case DEVICE_STATE_SUBSCRIBE_COV:
        busy_time = device_subscribe_cov(dev_ptr);
        break;
    case DEVICE_STATE_REQUEST_PRESENT_VALUE:
        busy_time = device_request_present_value(dev_ptr);
        break;
    default:
        break;
    }

    return busy_time;
}

// peek at all the devices see if any will need prompt service
bool query_busy_status(void)
{
    bool relax = true;
    int i = 0;                  // counter
    int max_devices = 0;
    struct BACnet_Device_Info *dev_ptr = NULL;
    struct ObjectRef_Struct *obj_ptr = NULL;    /* temporary objectref */
    time_t delta_time = 1;      /* time_h storage for time */
    time_t t;                   /* time_h storage for time */

    max_devices = device_count();

    for (i = 0; i < max_devices; i++) {
        // check to see if it is at least 
        // polling present value or updating COV
        // so that we can relax
        dev_ptr = device_record(i);
        if (dev_ptr) {
            switch (dev_ptr->state) {
            case DEVICE_STATE_INIT:
            case DEVICE_STATE_QUERY_DEVICE_PROPERTIES:
            case DEVICE_STATE_QUERY_OBJECT_LIST:
            case DEVICE_STATE_QUERY_OBJECT_LIST_PROPERTIES:
                relax = false;
                break;
            case DEVICE_STATE_SUBSCRIBE_COV:
            case DEVICE_STATE_REQUEST_PRESENT_VALUE:
                obj_ptr =
                    object_get_by_index(dev_ptr, dev_ptr->object_index);
                if (obj_ptr) {
                    t = time(NULL);
                    delta_time = t - obj_ptr->last_subscribe_COV;
                    if (delta_time >= BACnet_COV_Lifetime)
                        relax = false;
                }
                break;
            default:
                break;
            }
        }
        // no use continuing if we are already relaxed
        if (!relax)
            break;
    }

    return relax;
}

int query_new_device(void)
{
    static int device_index = 0;        // work only one device per call
    struct BACnet_Device_Info *dev_ptr = NULL;
    bool relax = false;

    if (device_count() > 0) {   /* some devices to check */
        /* all Invoke IDs are available */
        if (invoke_id_in_use() < BACnet_Invoke_Ids) {
            dev_ptr = device_record(device_index);
            if (dev_ptr) {
                // don't query myself 
                // maybe later when I get all my device props working
                if (dev_ptr->device == BACnet_Device_Instance)
                    dev_ptr->state = DEVICE_STATE_IDLE;
                // query the device
                query_device(dev_ptr);
                // get ready for next time 
                device_index++;
            } else {
                // start over
                device_index = 0;
            }
            relax = query_busy_status();
        }
    }

    return (relax);
}
