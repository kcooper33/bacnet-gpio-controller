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
// Handle Write-Property requests that come from other devices
// This allows external BACnet clients to control GPIO outputs
//
#include "os.h"
#include <unistd.h>
#include "bacnet_struct.h"
#include "bacnet_enum.h"
#include "bacnet_const.h"
#include "bacnet_api.h"
#include "bacnet_device.h"
#include "bacnet_text.h"
#include "bacdcode.h"
#include "reject.h"
#include "debug.h"
#include "pdu.h"
#include "options.h"
#include "version.h"
#include "gpio_objects.h"

// Use simpler approach - store in relinquish_defaults only and fix read function
// extern struct gpio_priority_array gpio_priorities[5];
#include "bacnet_object.h"

// Forward declaration
static void gpio_write_pin(uint32_t instance, float value);

// from main.c
extern int BACnet_Device_Instance;

// Priority array storage for GPIO output objects
static union ObjectValue priority_arrays[3][16]; // For instances 4018, 4026, 2021
static bool priority_null[3][16] = {{true}}; // Track which priorities are NULL (initialize all to true)
union ObjectValue relinquish_defaults[5]; // Default values when all priorities are NULL (make global)
static bool arrays_initialized = false;

// Map instance to priority array index
static int get_priority_index(uint32_t instance) {
    switch (instance) {
        case 4018: return 0; // Test LED
        case 4026: return 1; // Main Relay
        case 2021: return 2; // Fan Control
        default: return -1;
    }
}

// Calculate effective present-value from priority array
static union ObjectValue calculate_effective_value(int object_type, uint32_t instance) {
    union ObjectValue effective_value;
    int pri_idx = get_priority_index(instance);
    
    if (pri_idx < 0) {
        // Not a GPIO object, return default
        if (object_type == OBJECT_BINARY_OUTPUT) {
            effective_value.enumerated = 0;
        } else {
            effective_value.real = 0.0f;
        }
        return effective_value;
    }
    
    // Find highest priority (lowest number) that's not NULL
    for (int i = 0; i < 16; i++) {
        if (!priority_null[pri_idx][i]) {
            debug_printf(3, "WRP: Effective value from priority %d\n", i + 1);
            return priority_arrays[pri_idx][i];
        }
    }
    
    // All priorities are NULL, use relinquish-default
    debug_printf(3, "WRP: Using relinquish-default (all priorities NULL)\n");
    return relinquish_defaults[pri_idx];
}

// Initialize priority arrays - call this once at startup
static void initialize_priority_arrays(void) {
    if (arrays_initialized) return;
    
    // Initialize all priorities to NULL
    for (int obj = 0; obj < 3; obj++) {
        for (int pri = 0; pri < 16; pri++) {
            priority_null[obj][pri] = true;
            priority_arrays[obj][pri].enumerated = 0; // Default value
        }
    }
    
    // Initialize relinquish defaults
    relinquish_defaults[0].enumerated = 0; // BO4018 default INACTIVE
    relinquish_defaults[1].enumerated = 0; // BO4026 default INACTIVE  
    relinquish_defaults[2].real = 0.0;     // AO2021 default 0%
    
    arrays_initialized = true;
    debug_printf(2, "WRP: Priority arrays and relinquish defaults initialized\n");
}

// External function to get priority value (called from read property handler)
bool get_priority_value(uint32_t instance, int priority, union ObjectValue *value) {
    initialize_priority_arrays();
    
    int pri_idx = get_priority_index(instance);
    if (pri_idx < 0 || priority < 1 || priority > 16) {
        return true; // Invalid - return as NULL
    }
    
    if (priority_null[pri_idx][priority - 1]) {
        return true; // NULL value
    }
    
    *value = priority_arrays[pri_idx][priority - 1];
    return false; // Not NULL
}

// Integrated write property function for all objects including GPIO
int write_object_property_value(int object_type, uint32_t instance, 
    uint32_t property, uint8_t tag, void *value, uint8_t priority)
{
    struct ObjectRef_Struct *obj_ptr;
    
    debug_printf(2, "WRP: Integrated write for object type %d instance %u property %d priority %u\n",
        object_type, instance, property, priority);
    
    // Find the object using the existing object system
    obj_ptr = object_find(BACnet_Device_Instance, object_type, instance);
    if (!obj_ptr) {
        debug_printf(2, "WRP: Object not found - device %d, type %d instance %u\n", 
            BACnet_Device_Instance, object_type, instance);
        return -2; // Object not found
    }
    
    debug_printf(2, "WRP: Found object %s, writing property %d at priority %u\n", 
        obj_ptr->name ? obj_ptr->name : "unnamed", property, priority);
    
    // Initialize priority arrays on first use
    initialize_priority_arrays();
    
    // Handle present-value property with priority array logic
    if (property == PROP_PRESENT_VALUE) {
        // Check if this is a GPIO output object (supports priority arrays)
        if ((object_type == OBJECT_BINARY_OUTPUT && (instance == 4018 || instance == 4026)) ||
            (object_type == OBJECT_ANALOG_OUTPUT && instance == 2021)) {
            
            // Priority-based write for GPIO output objects
            int pri_idx = get_priority_index(instance);
            if (pri_idx < 0) {
                debug_printf(1, "WRP: Invalid GPIO instance %u\n", instance);
                return -3;
            }
            
            // Validate priority (1-16)
            if (priority < 1 || priority > 16) {
                debug_printf(1, "WRP: Invalid priority %u (must be 1-16)\n", priority);
                return -3;
            }
            
            // Store value in priority array or handle NULL write
            if (tag == BACNET_APPLICATION_TAG_NULL) {
                // NULL write - relinquish this priority
                priority_null[pri_idx][priority - 1] = true;
                debug_printf(1, "WRP: Relinquished priority %u for %s %u\n", 
                    priority, (object_type == OBJECT_BINARY_OUTPUT) ? "Binary Output" : "Analog Output", instance);
                    
            } else if (object_type == OBJECT_BINARY_OUTPUT) {
                if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                    uint32_t enum_value = *(uint32_t*)value;
                    priority_arrays[pri_idx][priority - 1].enumerated = (enum_value != 0) ? 1 : 0;
                    priority_null[pri_idx][priority - 1] = false;
                    
                    debug_printf(1, "WRP: Set priority %u to %s for Binary Output %u\n", 
                        priority, priority_arrays[pri_idx][priority - 1].enumerated ? "ACTIVE" : "INACTIVE", instance);
                } else {
                    debug_printf(1, "WRP: Invalid data type %d for Binary Output\n", tag);
                    return -3;
                }
            } else if (object_type == OBJECT_ANALOG_OUTPUT) {
                if (tag == BACNET_APPLICATION_TAG_REAL) {
                    float real_value = *(float*)value;
                    priority_arrays[pri_idx][priority - 1].real = real_value;
                    priority_null[pri_idx][priority - 1] = false;
                    
                    debug_printf(1, "WRP: Set priority %u to %.2f for Analog Output %u\n", 
                        priority, real_value, instance);
                } else {
                    debug_printf(1, "WRP: Invalid data type %d for Analog Output\n", tag);
                    return -3;
                }
            }
            
            // Calculate effective present-value from priority array
            union ObjectValue effective = calculate_effective_value(object_type, instance);
            obj_ptr->value = effective;
            
            // Update the actual GPIO pin through GPIO objects handler
            if (object_type == OBJECT_BINARY_OUTPUT) {
                debug_printf(1, "WRP: Effective present-value for Binary Output %u: %s\n", 
                    instance, effective.enumerated ? "ACTIVE" : "INACTIVE");
                // Call GPIO objects write to trigger actual GPIO control
                gpio_objects_write_property(object_type, instance, PROP_PRESENT_VALUE,
                    BACNET_APPLICATION_TAG_ENUMERATED, &effective.enumerated, priority);
            } else {
                debug_printf(1, "WRP: Effective present-value for Analog Output %u: %.2f\n", 
                    instance, effective.real);
                // Call GPIO objects write to trigger actual GPIO control
                gpio_objects_write_property(object_type, instance, PROP_PRESENT_VALUE,
                    BACNET_APPLICATION_TAG_REAL, &effective.real, priority);
            }
            
            return 0; // Success
            
        } else {
            // Standard object write (no priority array)
            if (object_type == OBJECT_BINARY_OUTPUT) {
                if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                    uint32_t enum_value = *(uint32_t*)value;
                    obj_ptr->value.enumerated = (enum_value != 0) ? 1 : 0;
                    debug_printf(2, "WRP: Set Binary Output %u to %s\n", 
                        instance, obj_ptr->value.enumerated ? "ACTIVE" : "INACTIVE");
                    return 0;
                } else {
                    debug_printf(1, "WRP: Invalid data type %d for Binary Output\n", tag);
                    return -3;
                }
            } else if (object_type == OBJECT_ANALOG_OUTPUT) {
                if (tag == BACNET_APPLICATION_TAG_REAL) {
                    float real_value = *(float*)value;
                    obj_ptr->value.real = real_value;
                    debug_printf(2, "WRP: Set Analog Output %u to %.2f\n", instance, real_value);
                    return 0;
                } else {
                    debug_printf(1, "WRP: Invalid data type %d for Analog Output\n", tag);
                    return -3;
                }
            } else {
                debug_printf(1, "WRP: Object type %d is not writable\n", object_type);
                return -3;
            }
        }
        
    } else if (property == PROP_RELINQUISH_DEFAULT) {
        // Handle relinquish-default property writes for GPIO output objects  
        if ((object_type == OBJECT_BINARY_OUTPUT && (instance == 4018 || instance == 4026)) ||
            (object_type == OBJECT_ANALOG_OUTPUT && instance == 2021)) {
            
            // Set relinquish default and recalculate effective value
            if (object_type == OBJECT_BINARY_OUTPUT) {
                if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                    uint32_t enum_value = *(uint32_t*)value;
                    
                    // Store in relinquish_defaults array - will fix read function separately
                    if (instance == 4018) {
                        relinquish_defaults[0].enumerated = (enum_value != 0) ? 1 : 0;
                        debug_printf(1, "WRP: Set relinquish-default for Binary Output 4018 to %s (value=%u)\n", 
                            (enum_value != 0) ? "ACTIVE" : "INACTIVE", relinquish_defaults[0].enumerated);
                    } else if (instance == 4026) {
                        relinquish_defaults[1].enumerated = (enum_value != 0) ? 1 : 0;
                        debug_printf(1, "WRP: Set relinquish-default for Binary Output 4026 to %s (value=%u)\n", 
                            (enum_value != 0) ? "ACTIVE" : "INACTIVE", relinquish_defaults[1].enumerated);
                    }
                    
                    // Recalculate effective value and update GPIO
                    union ObjectValue effective = calculate_effective_value(object_type, instance);
                    obj_ptr->value = effective;
                    gpio_write_pin(instance, effective.enumerated);
                    
                    return 0;
                } else {
                    debug_printf(1, "WRP: Invalid tag %d for Binary Output relinquish-default\n", tag);
                    return -3;
                }
            } else if (object_type == OBJECT_ANALOG_OUTPUT) {
                if (tag == BACNET_APPLICATION_TAG_REAL) {
                    float real_value = *(float*)value;
                    
                    // Store in relinquish_defaults array
                    relinquish_defaults[2].real = real_value;
                    
                    debug_printf(1, "WRP: Set relinquish-default for Analog Output %u to %.2f (value=%.2f)\n", 
                        instance, real_value, relinquish_defaults[2].real);
                    
                    // Recalculate effective value and update GPIO
                    union ObjectValue effective = calculate_effective_value(object_type, instance);
                    obj_ptr->value = effective;
                    gpio_write_pin(instance, effective.real);
                    
                    return 0;
                } else {
                    debug_printf(1, "WRP: Invalid tag %d for Analog Output relinquish-default\n", tag);
                    return -3;
                }
            }
        } else {
            debug_printf(1, "WRP: Relinquish-default not supported for object type %d instance %u\n", 
                object_type, instance);
            return -3;
        }
        
    } else {
        debug_printf(2, "WRP: Property %u is not writable\n", property);
        return -3; // Property not writable
    }
    
    return -1; // Default error return
}

// Helper function to write to actual GPIO pin
static void gpio_write_pin(uint32_t instance, float value)
{
    // Map BACnet instance to GPIO pin number
    int gpio_pin = -1;
    
    switch (instance) {
        case 4018: gpio_pin = 18; break;  // Test LED
        case 4026: gpio_pin = 26; break;  // Main Relay
        case 2021: gpio_pin = 21; break;  // Fan Control (PWM)
        default:
            debug_printf(1, "WRP: Unknown instance %u for write\n", instance);
            return;
    }
    
    debug_printf(2, "WRP: Setting GPIO pin %d to %.2f\n", gpio_pin, value);
    
    // For binary outputs: value > 0.5 = HIGH (3.3V), <= 0.5 = LOW (0V)
    int digital_value = (value > 0.5) ? 1 : 0;
    
    debug_printf(1, "WRP: *** ATTEMPTING GPIO CONTROL - Pin %d, Value %d ***\n", gpio_pin, digital_value);
    
    // Use multi-method Python GPIO control for Pi 5 compatibility
    char command[256];

    int status;
    
    // Method 1: Try gpiozero with cleanup (preferred for Pi 5)
    snprintf(command, sizeof(command), 
        "python3 -c \"from gpiozero import Device, LED; Device.pin_factory.reset(); led=LED(%d); led.%s(); print('SUCCESS')\" 2>/dev/null",
        gpio_pin, digital_value ? "on" : "off");
    
    debug_printf(1, "WRP: Trying gpiozero method with cleanup for GPIO %d\n", gpio_pin);
    status = system(command);
    if (status == 0) {
        debug_printf(1, "WRP: *** SUCCESS - GPIO %d controlled via gpiozero ***\n", gpio_pin);
        return;
    }
    debug_printf(1, "WRP: gpiozero failed (status %d)\n", status);
    
    // Method 2: Try RPi.GPIO with pin-specific cleanup
    snprintf(command, sizeof(command), 
        "python3 -c \"import RPi.GPIO as GPIO; GPIO.setmode(GPIO.BCM); GPIO.cleanup(%d); GPIO.setup(%d, GPIO.OUT); GPIO.output(%d, %d); print('SUCCESS')\" 2>/dev/null",
        gpio_pin, gpio_pin, gpio_pin, digital_value);
    
    debug_printf(1, "WRP: Trying RPi.GPIO method with pin cleanup for GPIO %d\n", gpio_pin);
    status = system(command);
    if (status == 0) {
        debug_printf(1, "WRP: *** SUCCESS - GPIO %d controlled via RPi.GPIO ***\n", gpio_pin);
        return;
    }
    debug_printf(1, "WRP: RPi.GPIO failed (status %d)\n", status);
    
    debug_printf(1, "WRP: *** FAILED - All GPIO control methods unsuccessful for pin %d ***\n", gpio_pin);
}

// Simple ACK response for successful writes
static void send_simple_ack(struct BACnet_Device_Address *dest, 
    uint8_t invoke_id, uint8_t service_choice)
{
    unsigned char *apdu;
    
    apdu = pdu_alloc();
    if (apdu) {
        apdu[0] = PDU_TYPE_SIMPLE_ACK;
        apdu[1] = invoke_id;
        apdu[2] = service_choice;
        
        send_npdu_address(dest, &apdu[0], 3);
        pdu_free(apdu);
        
        debug_printf(2, "WRP: Sent SimpleACK for invoke_id %d\n", invoke_id);
    }
}

// Error response for failed writes
static void send_error_response(struct BACnet_Device_Address *dest,
    uint8_t invoke_id, uint8_t service_choice, 
    enum BACnetErrorClass error_class, enum BACnetErrorCode error_code)
{
    unsigned char *apdu;
    int len = 0;
    int apdu_len = 0;
    
    apdu = pdu_alloc();
    if (apdu) {
        apdu[0] = PDU_TYPE_ERROR;
        apdu[1] = invoke_id;
        apdu[2] = service_choice;
        apdu_len = 3;
        
        // Error class and code
        len = encode_tagged_enumerated(&apdu[apdu_len], error_class);
        apdu_len += len;
        len = encode_tagged_enumerated(&apdu[apdu_len], error_code);
        apdu_len += len;
        
        send_npdu_address(dest, &apdu[0], apdu_len);
        pdu_free(apdu);
        
        debug_printf(2, "WRP: Sent Error response - class %d, code %d\n", 
            error_class, error_code);
    }
}

int receive_writeproperty(uint8_t * service_request,
    int service_len,
    struct BACnet_Device_Address *src, int src_max_apdu, uint8_t invoke_id)
{
    int len = 0;
    int offset = 0;
    int object_type = 0;
    uint32_t instance = 0;
    uint32_t property = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    int status = 0;
    uint8_t priority = 16; // Default priority (lowest)
    
    debug_printf(2, "WRP: Received WriteProperty request, invoke_id=%d\n", invoke_id);
    
    // Decode object identifier [0] - context tag
    if (decode_is_context_tag(&service_request[offset], 0)) {
        len = decode_tag_number_and_value(&service_request[offset], &tag_number, &len_value_type);
        offset += len;
        len = decode_object_id(&service_request[offset], &object_type, &instance);
        offset += len;
        debug_printf(3, "WRP: Object type %d instance %u\n", object_type, instance);
    } else {
        debug_printf(1, "WRP: Missing object identifier\n");
        send_error_response(src, invoke_id, SERVICE_CONFIRMED_WRITE_PROPERTY,
            ERROR_CLASS_SERVICES, ERROR_CODE_MISSING_REQUIRED_PARAMETER);
        return -1;
    }
    
    // Decode property identifier [1] - context tag
    if (decode_is_context_tag(&service_request[offset], 1)) {
        len = decode_tag_number_and_value(&service_request[offset], &tag_number, &len_value_type);
        offset += len;
        len = decode_enumerated(&service_request[offset], len_value_type, (int*)&property);
        offset += len;
        debug_printf(3, "WRP: Property %u\n", property);
    } else {
        debug_printf(1, "WRP: Missing property identifier\n");
        send_error_response(src, invoke_id, SERVICE_CONFIRMED_WRITE_PROPERTY,
            ERROR_CLASS_SERVICES, ERROR_CODE_MISSING_REQUIRED_PARAMETER);
        return -1;
    }
    
    // Optional array index [2] - skip if present
    if (decode_is_context_tag(&service_request[offset], 2)) {
        uint32_t array_index = 0;
        len = decode_tag_number_and_value(&service_request[offset], &tag_number, &len_value_type);
        offset += len;
        len = decode_unsigned(&service_request[offset], len_value_type, &array_index);
        offset += len;
        debug_printf(3, "WRP: Array index %u (not supported)\n", array_index);
        send_error_response(src, invoke_id, SERVICE_CONFIRMED_WRITE_PROPERTY,
            ERROR_CLASS_SERVICES, ERROR_CODE_PROPERTY_IS_NOT_A_LIST);
        return -1;
    }
    
    // Property value [3] - opening tag
    if (decode_is_opening_tag_number(&service_request[offset], 3)) {
        offset += 1; // Skip opening tag
        
        // Get the value type and length
        tag_number = service_request[offset];
        len_value_type = decode_tag_number_and_value(&service_request[offset], 
            &tag_number, &len_value_type);
        offset += len_value_type;
        
        debug_printf(3, "WRP: Value tag %d, length %u\n", tag_number, len_value_type);
        
        // Store value and type for later processing after priority extraction
        uint32_t enum_value = 0;
        float real_value = 0.0;
        uint8_t value_tag = tag_number;
        
        // Handle different value types including NULL for priority relinquishing
        bool is_null_write = false;
        
        if (tag_number == BACNET_APPLICATION_TAG_NULL) {
            // NULL value for priority relinquishing
            is_null_write = true;
            debug_printf(2, "WRP: NULL write (priority relinquish)\n");
            offset += len_value_type; // Skip NULL value
            
        } else if (tag_number == BACNET_APPLICATION_TAG_ENUMERATED) {
            // Binary values (for Binary Outputs)
            len = decode_enumerated(&service_request[offset], len_value_type, (int*)&enum_value);
            offset += len;
            debug_printf(2, "WRP: Decoded enumerated value %u\n", enum_value);
                
        } else if (tag_number == BACNET_APPLICATION_TAG_REAL) {
            // Real values (for Analog Outputs)
            len = decode_real(&service_request[offset], &real_value);
            offset += len;
            debug_printf(2, "WRP: Decoded real value %.2f\n", real_value);
                
        } else {
            debug_printf(1, "WRP: Unsupported value type %d\n", tag_number);
            send_error_response(src, invoke_id, SERVICE_CONFIRMED_WRITE_PROPERTY,
                ERROR_CLASS_PROPERTY, ERROR_CODE_INVALID_DATA_TYPE);
            return -1;
        }
        
        // Skip closing tag [3]
        if (decode_is_closing_tag_number(&service_request[offset], 3)) {
            offset += 1;
        }
        
        // Optional priority [4] - context tag
        if (offset < service_len && decode_is_context_tag(&service_request[offset], 4)) {
            len = decode_tag_number_and_value(&service_request[offset], &tag_number, &len_value_type);
            offset += len;
            uint32_t priority_value = 0;
            len = decode_unsigned(&service_request[offset], len_value_type, &priority_value);
            offset += len;
            
            if (priority_value >= 1 && priority_value <= 16) {
                priority = (uint8_t)priority_value;
                debug_printf(2, "WRP: Using priority %u from request\n", priority);
            } else {
                debug_printf(1, "WRP: Invalid priority %u, using default 16\n", priority_value);
            }
        } else {
            debug_printf(3, "WRP: No priority specified, using default 16\n");
        }
        
        // Now execute the write with the correct priority
        debug_printf(2, "WRP: Executing write with priority %u\n", priority);
        
        if (is_null_write) {
            debug_printf(1, "WRP: Relinquishing priority %u for object type %d instance %u property %u\n",
                priority, object_type, instance, property);
            status = write_object_property_value(object_type, instance, property,
                BACNET_APPLICATION_TAG_NULL, NULL, priority);
        } else if (value_tag == BACNET_APPLICATION_TAG_ENUMERATED) {
            debug_printf(2, "WRP: Writing enumerated value %u to object type %d instance %u property %u at priority %u\n",
                enum_value, object_type, instance, property, priority);
            status = write_object_property_value(object_type, instance, property,
                BACNET_APPLICATION_TAG_ENUMERATED, &enum_value, priority);
        } else if (value_tag == BACNET_APPLICATION_TAG_REAL) {
            debug_printf(2, "WRP: Writing real value %.2f to object type %d instance %u property %u at priority %u\n",
                real_value, object_type, instance, property, priority);
            status = write_object_property_value(object_type, instance, property,
                BACNET_APPLICATION_TAG_REAL, &real_value, priority);
        }
        
        // Send response based on write result
        if (status == 0) {
            // Success - send SimpleACK
            send_simple_ack(src, invoke_id, SERVICE_CONFIRMED_WRITE_PROPERTY);
            debug_printf(2, "WRP: Write successful\n");
        } else if (status == -2) {
            // Object not found
            send_error_response(src, invoke_id, SERVICE_CONFIRMED_WRITE_PROPERTY,
                ERROR_CLASS_OBJECT, ERROR_CODE_UNKNOWN_OBJECT);
        } else if (status == -3) {
            // Property not writable
            send_error_response(src, invoke_id, SERVICE_CONFIRMED_WRITE_PROPERTY,
                ERROR_CLASS_PROPERTY, ERROR_CODE_WRITE_ACCESS_DENIED);
        } else {
            // General error
            send_error_response(src, invoke_id, SERVICE_CONFIRMED_WRITE_PROPERTY,
                ERROR_CLASS_SERVICES, ERROR_CODE_OTHER);
        }
        
    } else {
        debug_printf(1, "WRP: Missing property value\n");
        send_error_response(src, invoke_id, SERVICE_CONFIRMED_WRITE_PROPERTY,
            ERROR_CLASS_SERVICES, ERROR_CODE_MISSING_REQUIRED_PARAMETER);
        return -1;
    }
    
    return 0;
}