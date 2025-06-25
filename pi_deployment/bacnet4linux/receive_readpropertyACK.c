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
// handle values returned from other devices that we wanted.
//
#include "os.h"
#include "bacnet_struct.h"
#include "bacnet_enum.h"
#include "bacnet_const.h"
#include "bacnet_api.h"
#include "bacnet_device.h"
#include "bacnet_object.h"
#include "bacnet_text.h"
#include "bacdcode.h"
#include "invoke_id.h"
#include "debug.h"

int receive_readpropertyACK(uint8_t * service_request, int service_len,
    struct BACnet_Device_Address *src)
{
    int object = 0, obj2 = 0;   /* temporary object references */
    int property = 0;
    int instance = 0, inst2 = 2;        /* temporary object instances */
    int who_sent = 0;           /* what device sent us this packet? */
    uint32_t array_index = 0;
    int offset = 0;
    int len = 0;
    char temp_string[256] = "";
    struct BACnet_Device_Info *dev_ptr = NULL;  // for device info
    struct ObjectRef_Struct *obj_ptr = NULL;    // temporary objectref
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    float real_value = 0.0;
    uint32_t enum_value = 0;
    uint32_t unsigned_value = 0;

    debug_printf(5, "read-property-ack: Entered\n");
    /* which BACnet address? */
    who_sent = device_which_sent(src);
    dev_ptr = device_get(who_sent);
    // check for valid device to store data into
    if (who_sent == -1) {
        debug_printf(2,
            "read-property-ack: I don't know who sent this pdu\n");
        return -1;
    }
    if (dev_ptr == NULL) {
        debug_printf(2,
            "read-property-ack: device %d is not in my list.\n", who_sent);
        return -1;
    }
    // Tag 0: Object ID         
    offset = 0;
    if (!decode_is_context_tag(&service_request[offset], 0))
        return -1;
    offset++;
    offset +=
        decode_object_id(&service_request[offset], &object, &instance);
    debug_printf(4,
        "read-property-ack: Decoded %s(%d) %d from Device %d.\n",
        enum_to_text_object(object), object, instance, who_sent);
    // Tag 1: Property ID
    offset += decode_tag_number_and_value(&service_request[offset],
        &tag_number, &len_value_type);
    if (tag_number != 1)
        return -1;
    offset += decode_enumerated(&service_request[offset], len_value_type,
        &property);
    debug_printf(4, "read-property-ack: Decoded %s(%d) property.\n",
        enum_to_text_property(property), property);
    // Tag 2: Optional Array Index
    len = decode_tag_number_and_value(&service_request[offset],
        &tag_number, &len_value_type);
    if (tag_number == 2) {
        offset += len;
        offset += decode_unsigned(&service_request[offset], len_value_type,
            &array_index);
        debug_printf(2,
            "read-property-ack: Device %d sent %s %d : %s[%u].\n",
            who_sent, enum_to_text_object(object), instance,
            enum_to_text_property(property), array_index);
    } else {
        debug_printf(2,
            "read-property-ack: : Device %d sent %s %d : %s.\n", who_sent,
            enum_to_text_object(object), instance,
            enum_to_text_property(property));
        array_index = BACNET_ARRAY_ALL;
    }
    // Tag 3: opening context tag */
    if (decode_is_opening_tag_number(&service_request[offset], 3)) {
        offset++;               // tag number of 3 is not extended so only one octet
        // decode the application tag number
        offset += decode_tag_number_and_value(&service_request[offset],
            &tag_number, &len_value_type);
        switch (tag_number) {
        case BACNET_APPLICATION_TAG_NULL:
            debug_printf(2, "RP[Null]: Device %d %s %d %s.\n",
                who_sent, enum_to_text_object(object),
                instance, enum_to_text_property(property));
            break;
        case BACNET_APPLICATION_TAG_BOOLEAN:
            debug_printf(2, "RP[Boolean]: Device %d %s %d %s.\n",
                who_sent, enum_to_text_object(object),
                instance, enum_to_text_property(property));
            break;
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            offset += decode_unsigned(&service_request[offset],
                len_value_type, &unsigned_value);
            debug_printf(2, "RP[Unsigned]: Device %d %s %d %s=%lu\n",
                who_sent, enum_to_text_object(object),
                instance, enum_to_text_property(property), unsigned_value);
            if ((property == PROP_OBJECT_LIST) &&
                (object == OBJECT_DEVICE)) {
                if (array_index == 0) {
                    debug_printf(2,
                        "RP: Device %d ObjectList[0] (Array Size) is: %lu\n",
                        who_sent, unsigned_value);
                    /* record true number of objects - must be at least 1
                       since the device must have a device object */
                    if (unsigned_value)
                        dev_ptr->true_num_objects = unsigned_value;
                }
            }
            break;
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            debug_printf(2, "RP[Signed]: Device %d %s %d %s\n",
                who_sent, enum_to_text_object(object),
                instance, enum_to_text_property(property));
            break;
        case BACNET_APPLICATION_TAG_REAL:
            offset += decode_real(&service_request[offset], &real_value);
            debug_printf(2, "RP[float]: Device %d %s %d %s=%f\n",
                who_sent, enum_to_text_object(object),
                instance, enum_to_text_property(property), real_value);
            if (property == PROP_PRESENT_VALUE) {
                obj_ptr = object_find(who_sent, object, instance);
                if (obj_ptr) {
                    obj_ptr->value.real = real_value;
                    debug_printf(3, "RP[float]: Device %d %s %d %s=%f\n",
                        who_sent, enum_to_text_object(obj_ptr->type),
                        obj_ptr->instance, enum_to_text_property(property),
                        obj_ptr->value.real);
                }
            }
            break;
        case BACNET_APPLICATION_TAG_DOUBLE:
            break;
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            break;
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            // FIXME: string could be longer than we allocated!!!!
            offset += decode_character_string(&service_request[offset],
                len_value_type, temp_string, sizeof(temp_string));
            debug_printf(2, "RP[string]: Device %d %s %d %s=%s\n",
                who_sent, enum_to_text_object(object),
                instance, enum_to_text_property(property), temp_string);
            // load the string into object storage
            if (property == PROP_OBJECT_NAME) {
                if (object == OBJECT_DEVICE) {
                    if (dev_ptr->device_name)
                        free(dev_ptr->device_name);
                    dev_ptr->device_name = strdup(temp_string);
                }
                obj_ptr = object_find(who_sent, object, instance);
                if (obj_ptr) {
                    if (obj_ptr->name)
                        free(obj_ptr->name);
                    obj_ptr->name = strdup(temp_string);
                }
            } else if (property == PROP_ACTIVE_TEXT) {
                obj_ptr = object_find(who_sent, object, instance);
                if (obj_ptr) {
                    if (obj_ptr->units.states.active)
                        free(obj_ptr->units.states.active);
                    obj_ptr->units.states.active = strdup(temp_string);
                }
            } else if (property == PROP_INACTIVE_TEXT) {
                obj_ptr = object_find(who_sent, object, instance);
                if (obj_ptr) {
                    if (obj_ptr->units.states.inactive)
                        free(obj_ptr->units.states.inactive);
                    obj_ptr->units.states.inactive = strdup(temp_string);
                }
            }
            /* some other string property */
            else {
                // add properties as needed.
            }
            break;
        case BACNET_APPLICATION_TAG_BIT_STRING:
            break;
        case BACNET_APPLICATION_TAG_ENUMERATED:
            offset += decode_enumerated(&service_request[offset],
                len_value_type, &enum_value);
            debug_printf(2, "RP[enum]: Device %d %s %d %s=%lu\n",
                who_sent, enum_to_text_object(object),
                instance, enum_to_text_property(property), enum_value);
            if (property == PROP_UNITS) {
                debug_printf(2, "RP[enum units]%s\n",
                    enum_to_text_units(enum_value));
                /* find and change the object */
                obj_ptr = object_find(who_sent, object, instance);
                if (obj_ptr) {
                    obj_ptr->units.units = enum_value;
                    debug_printf(3, "RP[enum]: Device %d %s %d %s=%s\n",
                        who_sent, enum_to_text_object(obj_ptr->type),
                        obj_ptr->instance, enum_to_text_property(property),
                        enum_to_text_units(obj_ptr->units.units));
                }
            } else if (property == PROP_PRESENT_VALUE) {
                /* find and change the object */
                obj_ptr = object_find(who_sent, object, instance);
                if (obj_ptr) {
                    obj_ptr->value.binary = enum_value;
                    debug_printf(3, "RP[enum]: Device %d %s %d %s=%s\n",
                        who_sent, enum_to_text_object(obj_ptr->type),
                        obj_ptr->instance, enum_to_text_property(property),
                        obj_ptr->value.binary ? "ACTIVE" : "INACTIVE");
                }
            } else
                debug_printf(2, "RP[enum] %lu\n", enum_value);
            break;
        case BACNET_APPLICATION_TAG_DATE:
            break;
        case BACNET_APPLICATION_TAG_TIME:
            break;
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            if ((property == PROP_OBJECT_LIST) &&
                (object == OBJECT_DEVICE) &&
                (array_index != 0) && (array_index != BACNET_ARRAY_ALL)) {
                /* object type and instance is next */
                offset += decode_object_id(&service_request[offset],
                    &obj2, &inst2);
                debug_printf(2,
                    "RP: Device %d sent ObjectList[%lu] %s %d.\n",
                    who_sent, array_index, enum_to_text_object(obj2),
                    inst2);
                /* it's a known BACnet standard object */
                if (obj2 < OBJECT_RESERVED_0) {
                    obj_ptr = object_new(who_sent, obj2, inst2);
                    if (obj_ptr) {
                        debug_printf(2,
                            "RP: Device %d ObjectList added %s %d.\n",
                            who_sent, enum_to_text_object(obj2), inst2);
                    } else
                        debug_printf(2,
                            "RP: Device %d ObjectList unable to add %s %d.\n",
                            who_sent, enum_to_text_object(obj2), inst2);
                }
            }
            break;
        default:
            break;
        }
    }

    /* end of object list handling */
    debug_printf(4, "RRPA: returning\n");
    return 0;                   /* indicating success */
}
