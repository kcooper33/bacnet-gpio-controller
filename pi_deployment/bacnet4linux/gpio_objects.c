/*
 * GPIO Objects for BACnet4Linux
 * Creates BACnet objects for Raspberry Pi GPIO pins
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "bacnet_api.h"
#include "bacnet_struct.h"
#include "bacnet_enum.h"
#include "bacnet_const.h"
#include "bacnet_object.h"
#include "bacdcode.h"
#include "pdu.h"
#include "reject.h"
#include "options.h"
#include "bacnet_text.h"
#include "debug.h"
#include "gpio_objects.h"

// Status flag definitions
#define STATUS_FLAG_IN_ALARM 0
#define STATUS_FLAG_FAULT 1
#define STATUS_FLAG_OVERRIDDEN 2
#define STATUS_FLAG_OUT_OF_SERVICE 3

// Polarity definitions
#define POLARITY_NORMAL 0
#define POLARITY_REVERSE 1

// Priority array support (16 priority levels as per BACnet standard)
#define BACNET_MAX_PRIORITY 16
#define BACNET_NO_PRIORITY 0

// Priority array structure for each GPIO object
struct gpio_priority_array {
    union ObjectValue values[BACNET_MAX_PRIORITY];
    uint8_t priorities_set;  // Bitmask of which priorities are set
    union ObjectValue relinquish_default;
    uint8_t out_of_service;
};

// Global priority arrays for each GPIO object
static struct gpio_priority_array gpio_priorities[5]; // 5 GPIO objects

// Forward declaration for helper function
static void gpio_write_pin(uint32_t instance, float value);
static union ObjectValue gpio_get_effective_value(uint32_t instance);
static int gpio_get_object_index(uint32_t instance);
static int gpio_read_pin(uint32_t instance);

void gpio_objects_init(int device_id)
{
    struct ObjectRef_Struct *obj_ptr;
    
    debug_printf(1, "GPIO: Initializing GPIO objects for device %d\n", device_id);
    debug_printf(1, "GPIO: Objects before creation: %d\n", object_count(device_id));
    
    // Load configuration from JSON file and create objects dynamically
    FILE *config_file = fopen("gpio_pin_config.json", "r");
    if (config_file != NULL) {
        debug_printf(1, "GPIO: Loading configuration from gpio_pin_config.json\n");
        
        // Read file contents
        fseek(config_file, 0, SEEK_END);
        long file_size = ftell(config_file);
        fseek(config_file, 0, SEEK_SET);
        
        char *json_buffer = malloc(file_size + 1);
        if (json_buffer != NULL) {
            fread(json_buffer, 1, file_size, config_file);
            json_buffer[file_size] = '\0';
            
            // Parse and create objects based on configuration
            gpio_create_objects_from_config(device_id, json_buffer);
            
            free(json_buffer);
        }
        fclose(config_file);
    } else {
        debug_printf(1, "GPIO: No config file found, creating default objects\n");
        gpio_create_default_objects(device_id);
    }
    
    debug_printf(1, "GPIO: Objects after creation: %d\n", object_count(device_id));
    
    // Initialize priority arrays with default values
    memset(gpio_priorities, 0, sizeof(gpio_priorities));
    
    // Set relinquish defaults
    gpio_priorities[0].relinquish_default.enumerated = 0; // BO 4018 - OFF
    gpio_priorities[1].relinquish_default.enumerated = 0; // BI 3019 - No motion  
    gpio_priorities[2].relinquish_default.real = 20.0;    // AI 1020 - 20°C
    gpio_priorities[3].relinquish_default.real = 0.0;     // AO 2021 - 0%
    gpio_priorities[4].relinquish_default.enumerated = 0; // BO 4026 - OFF
    
    debug_printf(1, "GPIO: Initialization complete for device %d\n", device_id);
    
    // Test object lookup immediately after creation
    obj_ptr = object_find(device_id, OBJECT_BINARY_OUTPUT, 4018);
    debug_printf(1, "GPIO: Test lookup BO 4018: %s\n", obj_ptr ? "Found" : "NOT FOUND");
    obj_ptr = object_find(device_id, OBJECT_BINARY_INPUT, 3019);
    debug_printf(1, "GPIO: Test lookup BI 3019: %s\n", obj_ptr ? "Found" : "NOT FOUND");
}

void gpio_objects_update_values(int device_id)
{
    struct ObjectRef_Struct *obj_ptr;
    
    // Update GPIO 20 - Temperature Sensor (simulate reading)
    if ((obj_ptr = object_find(device_id, OBJECT_ANALOG_INPUT, 1020)) != NULL) {
        // Simulate temperature reading between 18-25°C
        float temp = 20.0 + (rand() % 50) / 10.0;
        obj_ptr->value.real = temp;
        debug_printf(4, "GPIO: Updated temperature to %.1f°C\n", temp);
    }
    
    // Update GPIO 19 - Motion Sensor (simulate motion detection)
    if ((obj_ptr = object_find(device_id, OBJECT_BINARY_INPUT, 3019)) != NULL) {
        // Random motion detection for demo
        if (rand() % 10 == 0) { // 10% chance of motion
            obj_ptr->value.enumerated = 1;
            debug_printf(3, "GPIO: Motion detected\n");
        } else {
            obj_ptr->value.enumerated = 0;
        }
    }
}

// GPIO ReadProperty handler - provides essential BACnet properties
int gpio_handle_read_property(struct BACnet_Device_Address *src, uint8_t invoke_id, 
                             BACNET_OBJECT_TYPE object_type, uint32_t instance,
                             BACNET_PROPERTY_ID property, 
                             uint32_t array_index, uint16_t src_max_apdu) {
    
    struct ObjectRef_Struct *obj_ptr;
    uint8_t *apdu;
    uint16_t apdu_len = 0;
    
    debug_printf(1, "GPIO: ReadProperty request for object type %d instance %u property %d\n",
        object_type, instance, property);
    
    // Find the GPIO object using the existing object system
    debug_printf(3, "GPIO: Searching for object - device %d, type %d, instance %u\n", 
        BACnet_Device_Instance, object_type, instance);
    obj_ptr = object_find(BACnet_Device_Instance, object_type, instance);
    if (!obj_ptr) {
        debug_printf(2, "GPIO: Object not found - device %d, type %d instance %u\n", 
            BACnet_Device_Instance, object_type, instance);
        debug_printf(2, "GPIO: Total objects in system: %d\n", object_count(BACnet_Device_Instance));
        return -1; // Object not found
    }
    
    debug_printf(2, "GPIO: Found object %s, handling property %d\n", 
        obj_ptr->name ? obj_ptr->name : "unnamed", property);
    
    // Handle all standard BACnet properties for GPIO objects
    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu = pdu_alloc();
            if (apdu) {
                apdu[0] = PDU_TYPE_COMPLEX_ACK;
                apdu[1] = invoke_id;
                apdu[2] = SERVICE_CONFIRMED_READ_PROPERTY;
                apdu_len = 3;
                
                apdu_len += encode_context_object_id(&apdu[apdu_len], 0, object_type, instance);
                apdu_len += encode_context_enumerated(&apdu[apdu_len], 1, property);
                apdu_len += encode_opening_tag(&apdu[apdu_len], 3);
                apdu_len += encode_tagged_object_id(&apdu[apdu_len], object_type, instance);
                apdu_len += encode_closing_tag(&apdu[apdu_len], 3);
                
                if (apdu_len <= src_max_apdu) {
                    send_npdu_address(src, &apdu[0], apdu_len);
                    pdu_free(apdu);
                    return 0;
                }
                pdu_free(apdu);
            }
            break;
            
        case PROP_OBJECT_NAME:
            apdu = pdu_alloc();
            if (apdu) {
                apdu[0] = PDU_TYPE_COMPLEX_ACK;
                apdu[1] = invoke_id;
                apdu[2] = SERVICE_CONFIRMED_READ_PROPERTY;
                apdu_len = 3;
                
                apdu_len += encode_context_object_id(&apdu[apdu_len], 0, object_type, instance);
                apdu_len += encode_context_enumerated(&apdu[apdu_len], 1, property);
                apdu_len += encode_opening_tag(&apdu[apdu_len], 3);
                apdu_len += encode_tagged_character_string(&apdu[apdu_len], 
                    obj_ptr->name ? obj_ptr->name : "GPIO Object");
                apdu_len += encode_closing_tag(&apdu[apdu_len], 3);
                
                if (apdu_len <= src_max_apdu) {
                    send_npdu_address(src, &apdu[0], apdu_len);
                    pdu_free(apdu);
                    return 0;
                }
                pdu_free(apdu);
            }
            break;
            
        case PROP_OBJECT_TYPE:
            apdu = pdu_alloc();
            if (apdu) {
                apdu[0] = PDU_TYPE_COMPLEX_ACK;
                apdu[1] = invoke_id;
                apdu[2] = SERVICE_CONFIRMED_READ_PROPERTY;
                apdu_len = 3;
                
                apdu_len += encode_context_object_id(&apdu[apdu_len], 0, object_type, instance);
                apdu_len += encode_context_enumerated(&apdu[apdu_len], 1, property);
                apdu_len += encode_opening_tag(&apdu[apdu_len], 3);
                apdu_len += encode_tagged_enumerated(&apdu[apdu_len], object_type);
                apdu_len += encode_closing_tag(&apdu[apdu_len], 3);
                
                if (apdu_len <= src_max_apdu) {
                    send_npdu_address(src, &apdu[0], apdu_len);
                    pdu_free(apdu);
                    return 0;
                }
                pdu_free(apdu);
            }
            break;
            
        case PROP_PRESENT_VALUE:
            apdu = pdu_alloc();
            if (apdu) {
                apdu[0] = PDU_TYPE_COMPLEX_ACK;
                apdu[1] = invoke_id;
                apdu[2] = SERVICE_CONFIRMED_READ_PROPERTY;
                apdu_len = 3;
                
                apdu_len += encode_context_object_id(&apdu[apdu_len], 0, object_type, instance);
                apdu_len += encode_context_enumerated(&apdu[apdu_len], 1, property);
                apdu_len += encode_opening_tag(&apdu[apdu_len], 3);
                
                if (object_type == OBJECT_BINARY_INPUT || object_type == OBJECT_BINARY_OUTPUT) {
                    apdu_len += encode_tagged_enumerated(&apdu[apdu_len], obj_ptr->value.enumerated);
                } else {
                    apdu_len += encode_tagged_real(&apdu[apdu_len], obj_ptr->value.real);
                }
                
                apdu_len += encode_closing_tag(&apdu[apdu_len], 3);
                
                if (apdu_len <= src_max_apdu) {
                    send_npdu_address(src, &apdu[0], apdu_len);
                    pdu_free(apdu);
                    return 0;
                }
                pdu_free(apdu);
            }
            break;
            
        case PROP_STATUS_FLAGS:
            apdu = pdu_alloc();
            if (apdu) {
                apdu[0] = PDU_TYPE_COMPLEX_ACK;
                apdu[1] = invoke_id;
                apdu[2] = SERVICE_CONFIRMED_READ_PROPERTY;
                apdu_len = 3;
                
                apdu_len += encode_context_object_id(&apdu[apdu_len], 0, object_type, instance);
                apdu_len += encode_context_enumerated(&apdu[apdu_len], 1, property);
                apdu_len += encode_opening_tag(&apdu[apdu_len], 3);
                
                // Status flags bitstring: [in-alarm, fault, overridden, out-of-service]
                BACNET_BIT_STRING bit_string;
                bitstring_init(&bit_string);
                bitstring_set_bit(&bit_string, 0, false); // in-alarm
                bitstring_set_bit(&bit_string, 1, false); // fault
                bitstring_set_bit(&bit_string, 2, false); // overridden
                bitstring_set_bit(&bit_string, 3, false); // out-of-service
                apdu_len += encode_tagged_bitstring(&apdu[apdu_len], &bit_string);
                
                apdu_len += encode_closing_tag(&apdu[apdu_len], 3);
                
                if (apdu_len <= src_max_apdu) {
                    send_npdu_address(src, &apdu[0], apdu_len);
                    pdu_free(apdu);
                    return 0;
                }
                pdu_free(apdu);
            }
            break;
            
        case PROP_OUT_OF_SERVICE:
            apdu = pdu_alloc();
            if (apdu) {
                apdu[0] = PDU_TYPE_COMPLEX_ACK;
                apdu[1] = invoke_id;
                apdu[2] = SERVICE_CONFIRMED_READ_PROPERTY;
                apdu_len = 3;
                
                apdu_len += encode_context_object_id(&apdu[apdu_len], 0, object_type, instance);
                apdu_len += encode_context_enumerated(&apdu[apdu_len], 1, property);
                apdu_len += encode_opening_tag(&apdu[apdu_len], 3);
                apdu_len += encode_tagged_enumerated(&apdu[apdu_len], 0); // Not out of service
                apdu_len += encode_closing_tag(&apdu[apdu_len], 3);
                
                if (apdu_len <= src_max_apdu) {
                    send_npdu_address(src, &apdu[0], apdu_len);
                    pdu_free(apdu);
                    return 0;
                }
                pdu_free(apdu);
            }
            break;
            
        case PROP_UNITS:
            apdu = pdu_alloc();
            if (apdu) {
                apdu[0] = PDU_TYPE_COMPLEX_ACK;
                apdu[1] = invoke_id;
                apdu[2] = SERVICE_CONFIRMED_READ_PROPERTY;
                apdu_len = 3;
                
                apdu_len += encode_context_object_id(&apdu[apdu_len], 0, object_type, instance);
                apdu_len += encode_context_enumerated(&apdu[apdu_len], 1, property);
                apdu_len += encode_opening_tag(&apdu[apdu_len], 3);
                
                if (object_type == OBJECT_BINARY_INPUT || object_type == OBJECT_BINARY_OUTPUT) {
                    apdu_len += encode_tagged_enumerated(&apdu[apdu_len], 95); // No units
                } else {
                    apdu_len += encode_tagged_enumerated(&apdu[apdu_len], 62); // Degrees Celsius
                }
                
                apdu_len += encode_closing_tag(&apdu[apdu_len], 3);
                
                if (apdu_len <= src_max_apdu) {
                    send_npdu_address(src, &apdu[0], apdu_len);
                    pdu_free(apdu);
                    return 0;
                }
                pdu_free(apdu);
            }
            break;
            
        case PROP_ACTIVE_TEXT:
            if (object_type == OBJECT_BINARY_INPUT || object_type == OBJECT_BINARY_OUTPUT) {
                apdu = pdu_alloc();
                if (apdu) {
                    apdu[0] = PDU_TYPE_COMPLEX_ACK;
                    apdu[1] = invoke_id;
                    apdu[2] = SERVICE_CONFIRMED_READ_PROPERTY;
                    apdu_len = 3;
                    
                    apdu_len += encode_context_object_id(&apdu[apdu_len], 0, object_type, instance);
                    apdu_len += encode_context_enumerated(&apdu[apdu_len], 1, property);
                    apdu_len += encode_opening_tag(&apdu[apdu_len], 3);
                    apdu_len += encode_tagged_character_string(&apdu[apdu_len], 
                        obj_ptr->units.states.active ? obj_ptr->units.states.active : "Active");
                    apdu_len += encode_closing_tag(&apdu[apdu_len], 3);
                    
                    if (apdu_len <= src_max_apdu) {
                        send_npdu_address(src, &apdu[0], apdu_len);
                        pdu_free(apdu);
                        return 0;
                    }
                    pdu_free(apdu);
                }
            }
            break;
            
        case PROP_INACTIVE_TEXT:
            if (object_type == OBJECT_BINARY_INPUT || object_type == OBJECT_BINARY_OUTPUT) {
                apdu = pdu_alloc();
                if (apdu) {
                    apdu[0] = PDU_TYPE_COMPLEX_ACK;
                    apdu[1] = invoke_id;
                    apdu[2] = SERVICE_CONFIRMED_READ_PROPERTY;
                    apdu_len = 3;
                    
                    apdu_len += encode_context_object_id(&apdu[apdu_len], 0, object_type, instance);
                    apdu_len += encode_context_enumerated(&apdu[apdu_len], 1, property);
                    apdu_len += encode_opening_tag(&apdu[apdu_len], 3);
                    apdu_len += encode_tagged_character_string(&apdu[apdu_len], 
                        obj_ptr->units.states.inactive ? obj_ptr->units.states.inactive : "Inactive");
                    apdu_len += encode_closing_tag(&apdu[apdu_len], 3);
                    
                    if (apdu_len <= src_max_apdu) {
                        send_npdu_address(src, &apdu[0], apdu_len);
                        pdu_free(apdu);
                        return 0;
                    }
                    pdu_free(apdu);
                }
            }
            break;
        case PROP_PRIORITY_ARRAY:
            // Handle priority array ONLY for output objects
            debug_printf(1, "GPIO: Priority array request for object type %d instance %u\n", object_type, instance);
            if (object_type == OBJECT_BINARY_OUTPUT || object_type == OBJECT_ANALOG_OUTPUT) {
                apdu = pdu_alloc();
                if (apdu) {
                    apdu[0] = PDU_TYPE_COMPLEX_ACK;
                    apdu[1] = invoke_id;
                    apdu[2] = SERVICE_CONFIRMED_READ_PROPERTY;
                    apdu_len = 3;
                    
                    apdu_len += encode_context_object_id(&apdu[apdu_len], 0, object_type, instance);
                    apdu_len += encode_context_enumerated(&apdu[apdu_len], 1, property);
                    apdu_len += encode_opening_tag(&apdu[apdu_len], 3);
                    
                    int obj_index = gpio_get_object_index(instance);
                    if (obj_index >= 0) {
                        struct gpio_priority_array *prio = &gpio_priorities[obj_index];
                        
                        if (array_index == 0) {
                            // Return array length (16)
                            apdu_len += encode_tagged_unsigned(&apdu[apdu_len], BACNET_MAX_PRIORITY);
                        } else if (array_index >= 1 && array_index <= BACNET_MAX_PRIORITY) {
                            // Return specific priority value or NULL
                            int prio_index = array_index - 1;
                            if (prio->priorities_set & (1 << prio_index)) {
                                if (object_type == OBJECT_BINARY_OUTPUT) {
                                    apdu_len += encode_tagged_enumerated(&apdu[apdu_len], 
                                        prio->values[prio_index].enumerated);
                                } else {
                                    apdu_len += encode_tagged_real(&apdu[apdu_len], 
                                        prio->values[prio_index].real);
                                }
                            } else {
                                // Priority not set - return NULL
                                apdu[apdu_len++] = 0x00; // BACnet NULL tag
                            }
                        } else {
                            // Invalid array index - return error
                            pdu_free(apdu);
                            send_error_address(src, invoke_id, SERVICE_CONFIRMED_READ_PROPERTY,
                                ERROR_CLASS_PROPERTY, ERROR_CODE_INVALID_ARRAY_INDEX);
                            return 0;
                        }
                    } else {
                        apdu[apdu_len++] = 0x00; // BACnet NULL tag
                    }
                    
                    apdu_len += encode_closing_tag(&apdu[apdu_len], 3);
                    
                    if (apdu_len <= src_max_apdu) {
                        send_npdu_address(src, &apdu[0], apdu_len);
                        pdu_free(apdu);
                        return 0; // Success
                    }
                }
                pdu_free(apdu);
            }
            return -1; // Not an output object or failed
            
        case PROP_RELINQUISH_DEFAULT:
            // Handle relinquish-default ONLY for output objects
            debug_printf(1, "GPIO: Relinquish-default request for object type %d instance %u\n", object_type, instance);
            if (object_type == OBJECT_BINARY_OUTPUT || object_type == OBJECT_ANALOG_OUTPUT) {
                apdu = pdu_alloc();
                if (apdu) {
                    apdu[0] = PDU_TYPE_COMPLEX_ACK;
                    apdu[1] = invoke_id;
                    apdu[2] = SERVICE_CONFIRMED_READ_PROPERTY;
                    apdu_len = 3;
                    
                    apdu_len += encode_context_object_id(&apdu[apdu_len], 0, object_type, instance);
                    apdu_len += encode_context_enumerated(&apdu[apdu_len], 1, property);
                    apdu_len += encode_opening_tag(&apdu[apdu_len], 3);
                    
                    debug_printf(1, "GPIO: *** RELINQUISH-DEFAULT READ REQUEST ***\n");
                    debug_printf(1, "GPIO: Reading relinquish-default for %s %u\n", 
                        object_type == OBJECT_BINARY_OUTPUT ? "Binary Output" : "Analog Output", instance);
                    
                    int obj_index = gpio_get_object_index(instance);
                    debug_printf(1, "GPIO: Object index lookup for instance %u: %d\n", instance, obj_index);
                    
                    // Use relinquish_defaults array directly
                    extern union ObjectValue relinquish_defaults[5];
                    
                    if (object_type == OBJECT_BINARY_OUTPUT) {
                        uint32_t stored_value = 0;
                        if (instance == 4018) {
                            stored_value = relinquish_defaults[0].enumerated;
                        } else if (instance == 4026) {
                            stored_value = relinquish_defaults[1].enumerated;
                        }
                        debug_printf(1, "GPIO: *** FOUND STORED VALUE - Binary Output %u relinquish-default: %u ***\n", 
                            instance, stored_value);
                        apdu_len += encode_tagged_enumerated(&apdu[apdu_len], stored_value);
                    } else if (object_type == OBJECT_ANALOG_OUTPUT) {
                        float stored_value = relinquish_defaults[2].real;
                        debug_printf(1, "GPIO: *** FOUND STORED VALUE - Analog Output %u relinquish-default: %.2f ***\n", 
                            instance, stored_value);
                        apdu_len += encode_tagged_real(&apdu[apdu_len], stored_value);
                    } else {
                        // Use default values for unknown objects (obj_index == -1)
                        debug_printf(1, "GPIO: *** OBJECT NOT FOUND - Using default for instance %u ***\n", instance);
                        if (object_type == OBJECT_BINARY_OUTPUT) {
                            debug_printf(1, "GPIO: *** RETURNING DEFAULT INACTIVE (0) for Binary Output %u ***\n", instance);
                            apdu_len += encode_tagged_enumerated(&apdu[apdu_len], 0); // INACTIVE
                        } else {
                            debug_printf(1, "GPIO: *** RETURNING DEFAULT 0.0 for Analog Output %u ***\n", instance);
                            apdu_len += encode_tagged_real(&apdu[apdu_len], 0.0); // 0%
                        }
                    }
                    
                    apdu_len += encode_closing_tag(&apdu[apdu_len], 3);
                    
                    if (apdu_len <= src_max_apdu) {
                        send_npdu_address(src, &apdu[0], apdu_len);
                        pdu_free(apdu);
                        return 0;
                    }
                    pdu_free(apdu);
                }
            }
            return -1; // Not an output object
            break;
        

            
        default:
            // Send proper BACnet error for unsupported properties
            debug_printf(1, "GPIO: Unsupported property %d for GPIO object type %d instance %u\n", 
                property, object_type, instance);
            send_error_address(src, invoke_id, SERVICE_CONFIRMED_READ_PROPERTY, 
                ERROR_CLASS_PROPERTY, ERROR_CODE_UNKNOWN_PROPERTY);
            return 0;
    }
    
    // If we get here, send a general error
    send_error_address(src, invoke_id, SERVICE_CONFIRMED_READ_PROPERTY, 
                      ERROR_CLASS_OBJECT, ERROR_CODE_UNKNOWN_OBJECT);
    
    return 0;
}

// Function to encode relinquish-default for read property requests
int gpio_encode_relinquish_default(uint8_t *apdu, BACNET_OBJECT_TYPE object_type, uint32_t instance)
{
    debug_printf(1, "GPIO: *** RELINQUISH-DEFAULT READ REQUEST ***\n");
    debug_printf(1, "GPIO: Reading relinquish-default for %s %u\n", 
        (object_type == OBJECT_BINARY_OUTPUT) ? "Binary Output" : "Analog Output", instance);
    
    int obj_index = gpio_get_object_index(instance);
    debug_printf(1, "GPIO: Object index lookup for instance %u: %d\n", instance, obj_index);
    
    // Use relinquish_defaults array directly instead of gpio_priorities
    extern union ObjectValue relinquish_defaults[5];
    
    if (object_type == OBJECT_BINARY_OUTPUT) {
        uint32_t stored_value = 0;
        if (instance == 4018) {
            stored_value = relinquish_defaults[0].enumerated;
        } else if (instance == 4026) {
            stored_value = relinquish_defaults[1].enumerated;
        }
        debug_printf(1, "GPIO: *** FOUND STORED VALUE - Binary Output %u relinquish-default: %u ***\n", 
            instance, stored_value);
        return encode_tagged_enumerated(&apdu[0], stored_value);
    } else if (object_type == OBJECT_ANALOG_OUTPUT) {
        float stored_value = relinquish_defaults[2].real;
        debug_printf(1, "GPIO: *** FOUND STORED VALUE - Analog Output %u relinquish-default: %.2f ***\n", 
            instance, stored_value);
        return encode_tagged_real(&apdu[0], stored_value);
    } else {
        debug_printf(1, "GPIO: *** OBJECT NOT FOUND - Using default for instance %u ***\n", instance);
        if (object_type == OBJECT_BINARY_OUTPUT) {
            debug_printf(1, "GPIO: *** RETURNING DEFAULT INACTIVE (0) for Binary Output %u ***\n", instance);
            return encode_tagged_enumerated(&apdu[0], 0); // INACTIVE
        } else {
            debug_printf(1, "GPIO: *** RETURNING DEFAULT 0.0 for Analog Output %u ***\n", instance);
            return encode_tagged_real(&apdu[0], 0.0); // 0%
        }
    }
}

int gpio_objects_write_property(int object_type,
    uint32_t instance, uint32_t property,
    uint8_t tag, void *value, uint8_t priority)
{
    struct ObjectRef_Struct *obj_ptr;
    
    debug_printf(2, "GPIO: WriteProperty request for object type %d instance %u property %d priority %u\n",
        object_type, instance, property, priority);
    
    // Find the GPIO object using the existing object system
    debug_printf(3, "GPIO: Searching for object - device %d, type %d, instance %u\n", 
        BACnet_Device_Instance, object_type, instance);
    obj_ptr = object_find(BACnet_Device_Instance, object_type, instance);
    if (!obj_ptr) {
        debug_printf(2, "GPIO: Object not found - device %d, type %d instance %u\n", 
            BACnet_Device_Instance, object_type, instance);
        return -2; // Object not found
    }
    
    debug_printf(2, "GPIO: Found object %s, writing property %d\n", 
        obj_ptr->name ? obj_ptr->name : "unnamed", property);
    
    // Handle present-value property (the main writable property)
    if (property == PROP_PRESENT_VALUE) {
        if (object_type == OBJECT_BINARY_OUTPUT) {
            // Binary Output - expect enumerated value (0=inactive, 1=active)
            if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                uint32_t enum_value = *(uint32_t*)value;
                
                // Update the object value
                obj_ptr->value.enumerated = (enum_value != 0) ? 1 : 0;
                
                // Update the actual GPIO pin
                gpio_write_pin(instance, obj_ptr->value.enumerated);
                
                debug_printf(2, "GPIO: Set Binary Output %u to %s\n", 
                    instance, obj_ptr->value.enumerated ? "ACTIVE" : "INACTIVE");
                return 0; // Success
            } else {
                debug_printf(1, "GPIO: Invalid data type %d for Binary Output\n", tag);
                return -3; // Invalid data type
            }
            
        } else if (object_type == OBJECT_ANALOG_OUTPUT) {
            // Analog Output - expect real value
            if (tag == BACNET_APPLICATION_TAG_REAL) {
                float real_value = *(float*)value;
                
                // Update the object value
                obj_ptr->value.real = real_value;
                
                // Update the actual GPIO pin (PWM or DAC)
                gpio_write_pin(instance, real_value);
                
                debug_printf(2, "GPIO: Set Analog Output %u to %.2f\n", 
                    instance, real_value);
                return 0; // Success
            } else {
                debug_printf(1, "GPIO: Invalid data type %d for Analog Output\n", tag);
                return -3; // Invalid data type
            }
            
        } else {
            debug_printf(1, "GPIO: Object type %d is not writable\n", object_type);
            return -3; // Not writable
        }
        
    } else if (property == PROP_RELINQUISH_DEFAULT) {
        // Handle relinquish-default property writes for output objects
        if (object_type == OBJECT_BINARY_OUTPUT || object_type == OBJECT_ANALOG_OUTPUT) {
            int obj_index = gpio_get_object_index(instance);
            if (obj_index >= 0) {
                struct gpio_priority_array *prio = &gpio_priorities[obj_index];
                
                if (object_type == OBJECT_BINARY_OUTPUT) {
                    if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                        uint32_t enum_value = *(uint32_t*)value;
                        prio->relinquish_default.enumerated = (enum_value != 0) ? 1 : 0;
                        
                        debug_printf(1, "GPIO: Set relinquish-default for Binary Output %u to %s\n", 
                            instance, prio->relinquish_default.enumerated ? "ACTIVE" : "INACTIVE");
                        
                        // Recalculate effective value and update GPIO
                        union ObjectValue effective = gpio_get_effective_value(instance);
                        obj_ptr->value = effective;
                        gpio_write_pin(instance, effective.enumerated);
                        
                        return 0;
                    } else {
                        debug_printf(1, "GPIO: Invalid tag %d for Binary Output relinquish-default\n", tag);
                        return -3;
                    }
                } else if (object_type == OBJECT_ANALOG_OUTPUT) {
                    if (tag == BACNET_APPLICATION_TAG_REAL) {
                        float real_value = *(float*)value;
                        prio->relinquish_default.real = real_value;
                        
                        debug_printf(1, "GPIO: Set relinquish-default for Analog Output %u to %.2f\n", 
                            instance, real_value);
                        
                        // Recalculate effective value and update GPIO
                        union ObjectValue effective = gpio_get_effective_value(instance);
                        obj_ptr->value = effective;
                        gpio_write_pin(instance, effective.real);
                        
                        return 0;
                    } else {
                        debug_printf(1, "GPIO: Invalid tag %d for Analog Output relinquish-default\n", tag);
                        return -3;
                    }
                }
            } else {
                debug_printf(1, "GPIO: Invalid instance %u for relinquish-default write\n", instance);
                return -3;
            }
        } else {
            debug_printf(1, "GPIO: Relinquish-default not supported for object type %d instance %u\n", 
                object_type, instance);
            return -3;
        }
        
    } else {
        debug_printf(2, "GPIO: Property %u is not writable\n", property);
        return -3; // Property not writable
    }
    
    return 0; // Default success return
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
            debug_printf(1, "GPIO: Unknown instance %u for write\n", instance);
            return;
    }
    
    debug_printf(1, "GPIO: Writing value %.2f to pin %d (Raspberry Pi 5 compatible)\n", value, gpio_pin);
    
    // For binary outputs, write digital value
    if (instance == 4018 || instance == 4026) {
        int digital_value = (value != 0.0) ? 1 : 0;
        
        debug_printf(1, "GPIO: Setting pin %d to %s for Raspberry Pi 5\n", 
            gpio_pin, digital_value ? "HIGH" : "LOW");
        
        // Method 1: Try libgpiod (preferred for Pi 5)
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "gpioset gpiochip4 %d=%d 2>&1", gpio_pin, digital_value);
        debug_printf(1, "GPIO: Trying libgpiod command: %s\n", cmd);
        int result = system(cmd);
        
        if (result == 0) {
            debug_printf(1, "GPIO: *** SUCCESS (libgpiod) *** Pin %d set to %s (%.1fV)\n", 
                gpio_pin, digital_value ? "HIGH" : "LOW", digital_value ? 3.3 : 0.0);
            return;
        }
        
        debug_printf(1, "GPIO: libgpiod failed with exit code %d, trying gpiozero\n", result);
        
        // Method 2: Python gpiozero fallback with detailed error logging
        snprintf(cmd, sizeof(cmd), 
            "timeout 3 python3 -c \"try:\n  from gpiozero import LED\n  import time\n  led=LED(%d)\n  led.%s()\n  time.sleep(0.1)\n  print('GPIO_%d_SUCCESS')\nexcept Exception as e:\n  print(f'GPIO_%d_ERROR: {e}')\n\" 2>&1", 
            gpio_pin, digital_value ? "on" : "off", gpio_pin, gpio_pin);
        debug_printf(1, "GPIO: Trying gpiozero command for pin %d\n", gpio_pin);
        
        FILE *fp = popen(cmd, "r");
        if (fp != NULL) {
            char output[256];
            if (fgets(output, sizeof(output), fp) != NULL) {
                // Remove newline
                output[strcspn(output, "\n")] = 0;
                debug_printf(1, "GPIO: Python output: %s\n", output);
                
                if (strstr(output, "GPIO_SUCCESS") != NULL) {
                    debug_printf(1, "GPIO: *** SUCCESS (gpiozero) *** Pin %d set to %s\n", 
                        gpio_pin, digital_value ? "HIGH" : "LOW");
                    pclose(fp);
                    return;
                }
            }
            pclose(fp);
        }
        
        debug_printf(1, "GPIO: gpiozero failed, trying sysfs\n");
        
        // Method 3: Traditional sysfs (may not work on Pi 5)
        
        // Write to actual GPIO pin
        char gpio_path[64];
        char value_str[8];
        int fd;
        
        // Export GPIO pin if not already exported
        snprintf(gpio_path, sizeof(gpio_path), "/sys/class/gpio/gpio%d", gpio_pin);
        if (access(gpio_path, F_OK) != 0) {
            fd = open("/sys/class/gpio/export", O_WRONLY);
            if (fd >= 0) {
                snprintf(value_str, sizeof(value_str), "%d", gpio_pin);
                write(fd, value_str, strlen(value_str));
                close(fd);
                usleep(100000); // Wait 100ms for export
            }
        }
        
        // Set pin direction to output
        snprintf(gpio_path, sizeof(gpio_path), "/sys/class/gpio/gpio%d/direction", gpio_pin);
        fd = open(gpio_path, O_WRONLY);
        if (fd >= 0) {
            write(fd, "out", 3);
            close(fd);
        }
        
        // Write the value
        snprintf(gpio_path, sizeof(gpio_path), "/sys/class/gpio/gpio%d/value", gpio_pin);
        fd = open(gpio_path, O_WRONLY);
        if (fd >= 0) {
            snprintf(value_str, sizeof(value_str), "%d", digital_value);
            ssize_t bytes_written = write(fd, value_str, strlen(value_str));
            close(fd);
            
            if (bytes_written > 0) {
                debug_printf(1, "GPIO: *** SUCCESS *** Pin %d set to %s (%.1fV)\n", 
                    gpio_pin, digital_value ? "HIGH" : "LOW", digital_value ? 3.3 : 0.0);
            } else {
                debug_printf(1, "GPIO: ERROR: Write failed for pin %d\n", gpio_pin);
            }
        } else {
            debug_printf(1, "GPIO: ERROR: Cannot open %s for writing\n", gpio_path);
            
            // Try alternative method using system commands
            char cmd[128];
            snprintf(cmd, sizeof(cmd), "echo %d > /sys/class/gpio/gpio%d/value 2>/dev/null", 
                digital_value, gpio_pin);
            int result = system(cmd);
            if (result == 0) {
                debug_printf(1, "GPIO: *** SUCCESS (fallback) *** Pin %d set to %s\n", 
                    gpio_pin, digital_value ? "HIGH" : "LOW");
            } else {
                debug_printf(1, "GPIO: ERROR: All methods failed for pin %d\n", gpio_pin);
            }
        }
        
    } else if (instance == 2021) {
        // For analog outputs, write PWM value (0-100%)
        int pwm_value = (int)(value * 255.0 / 100.0); // Convert percentage to 0-255
        if (pwm_value > 255) pwm_value = 255;
        if (pwm_value < 0) pwm_value = 0;
        
        debug_printf(2, "GPIO: Would set PWM pin %d to %d (%.1f%%)\n", 
            gpio_pin, pwm_value, value);
            
        // TODO: Add actual PWM write
        // Setup PWM and set duty cycle
    }
}

// Helper function to read GPIO pin value
static int gpio_read_pin(uint32_t instance)
{
    // Map BACnet instance to GPIO pin number
    int gpio_pin = -1;
    
    switch (instance) {
        case 3019: gpio_pin = 19; break;  // Motion Sensor
        case 1020: gpio_pin = 20; break;  // Temperature Sensor (if real GPIO)
        default:
            debug_printf(1, "GPIO: Unknown input instance %u for read\n", instance);
            return 0;
    }
    
    debug_printf(2, "GPIO: Reading GPIO pin %d for instance %u\n", gpio_pin, instance);
    
    // Method 1: Try libgpiod (preferred for Pi 5)
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "gpioget gpiochip4 %d 2>/dev/null", gpio_pin);
    
    FILE *fp = popen(cmd, "r");
    if (fp != NULL) {
        char output[16];
        if (fgets(output, sizeof(output), fp) != NULL) {
            int value = atoi(output);
            pclose(fp);
            debug_printf(2, "GPIO: libgpiod read pin %d as %s\n", 
                gpio_pin, value ? "HIGH" : "LOW");
            return value;
        }
        pclose(fp);
    }
    
    debug_printf(2, "GPIO: libgpiod failed, trying Python\n");
    
    // Method 2: Try Python GPIO reading
    snprintf(cmd, sizeof(cmd), 
        "python3 -c \"try:\n  import RPi.GPIO as GPIO\n  GPIO.setmode(GPIO.BCM)\n  GPIO.setup(%d, GPIO.IN)\n  val=GPIO.input(%d)\n  GPIO.cleanup(%d)\n  print(val)\nexcept Exception as e:\n  print('0')\" 2>/dev/null", 
        gpio_pin, gpio_pin, gpio_pin);
    
    fp = popen(cmd, "r");
    if (fp != NULL) {
        char output[16];
        if (fgets(output, sizeof(output), fp) != NULL) {
            int value = atoi(output);
            pclose(fp);
            debug_printf(2, "GPIO: Python read pin %d as %s\n", 
                gpio_pin, value ? "HIGH" : "LOW");
            return value;
        }
        pclose(fp);
    }
    
    debug_printf(2, "GPIO: Python failed, trying sysfs\n");
    
    // Method 3: Try sysfs (fallback)
    char gpio_path[64];
    snprintf(gpio_path, sizeof(gpio_path), "/sys/class/gpio/gpio%d/value", gpio_pin);
    
    FILE *value_file = fopen(gpio_path, "r");
    if (value_file != NULL) {
        char value_str[8];
        if (fgets(value_str, sizeof(value_str), value_file) != NULL) {
            int value = atoi(value_str);
            fclose(value_file);
            debug_printf(2, "GPIO: sysfs read pin %d as %s\n", 
                gpio_pin, value ? "HIGH" : "LOW");
            return value;
        }
        fclose(value_file);
    }
    
    debug_printf(1, "GPIO: All read methods failed for pin %d\n", gpio_pin);
    return 0; // Default to LOW if all methods fail
}

// Function to update input values from GPIO pins
void gpio_update_inputs(int device_id)
{
    struct ObjectRef_Struct *obj_ptr;
    static time_t last_update = 0;
    time_t current_time = time(NULL);
    
    // Only update every 1 second to avoid excessive polling
    if (current_time - last_update < 1) {
        return;
    }
    last_update = current_time;
    
    // Update Binary Input 3019 (GPIO 19 - Motion Sensor)
    obj_ptr = object_find(device_id, OBJECT_BINARY_INPUT, 3019);
    if (obj_ptr != NULL) {
        int gpio_value = gpio_read_pin(3019);
        // Convert 0/1 to BACnet enumerated values (0=INACTIVE, 1=ACTIVE)
        int new_value = gpio_value ? 1 : 0;
        
        if (obj_ptr->value.enumerated != new_value) {
            debug_printf(1, "GPIO: Binary Input 3019 changed: %s -> %s (GPIO pin 19 = %s)\n",
                obj_ptr->value.enumerated ? "ACTIVE" : "INACTIVE",
                new_value ? "ACTIVE" : "INACTIVE",
                gpio_value ? "HIGH" : "LOW");
            obj_ptr->value.enumerated = new_value;
        }
    }
}

// Create default GPIO objects (fallback)
void gpio_create_default_objects(int device_id)
{
    struct ObjectRef_Struct *obj_ptr;
    
    // GPIO 18 - Binary Output (Test LED)
    if ((obj_ptr = object_new(device_id, OBJECT_BINARY_OUTPUT, 4018)) != NULL) {
        obj_ptr->name = strdup("Test LED");
        obj_ptr->value.enumerated = 0;
        obj_ptr->units.states.active = strdup("ON");
        obj_ptr->units.states.inactive = strdup("OFF");
        debug_printf(2, "GPIO: Created default Binary Output 4018 - Test LED\n");
    }
    
    // GPIO 19 - Binary Input (Motion Sensor)
    if ((obj_ptr = object_new(device_id, OBJECT_BINARY_INPUT, 3019)) != NULL) {
        obj_ptr->name = strdup("Motion Sensor");
        obj_ptr->value.enumerated = 0;
        obj_ptr->units.states.active = strdup("Motion");
        obj_ptr->units.states.inactive = strdup("No Motion");
        debug_printf(2, "GPIO: Created default Binary Input 3019 - Motion Sensor\n");
    }
}

// Parse JSON configuration and create GPIO objects
void gpio_create_objects_from_config(int device_id, const char *json_config)
{
    struct ObjectRef_Struct *obj_ptr;
    
    // Simple JSON parsing for GPIO configuration
    // Look for enabled pins and create corresponding BACnet objects
    
    const char *ptr = json_config;
    char pin_str[8], name[64], direction[16], high_unit[32], low_unit[32];
    int instance, enabled;
    
    // Parse each GPIO pin configuration
    for (int gpio_pin = 0; gpio_pin <= 23; gpio_pin++) {
        snprintf(pin_str, sizeof(pin_str), "\"%d\"", gpio_pin);
        
        // Find this pin's configuration in JSON
        const char *pin_config = strstr(ptr, pin_str);
        if (pin_config == NULL) continue;
        
        // Extract enabled status
        const char *enabled_ptr = strstr(pin_config, "\"enabled\":");
        if (enabled_ptr == NULL) continue;
        enabled_ptr = strstr(enabled_ptr, ":");
        if (enabled_ptr == NULL) continue;
        enabled_ptr++;
        while (*enabled_ptr == ' ' || *enabled_ptr == '\t') enabled_ptr++;
        enabled = (strncmp(enabled_ptr, "true", 4) == 0);
        
        if (!enabled) continue; // Skip disabled pins
        
        // Extract name
        const char *name_ptr = strstr(pin_config, "\"name\":");
        if (name_ptr != NULL) {
            name_ptr = strchr(name_ptr, '"');
            if (name_ptr != NULL) {
                name_ptr++; // Skip opening quote
                const char *name_end = strchr(name_ptr, '"');
                if (name_end != NULL) {
                    int name_len = name_end - name_ptr;
                    if (name_len < sizeof(name)) {
                        strncpy(name, name_ptr, name_len);
                        name[name_len] = '\0';
                    } else {
                        snprintf(name, sizeof(name), "GPIO %d", gpio_pin);
                    }
                } else {
                    snprintf(name, sizeof(name), "GPIO %d", gpio_pin);
                }
            } else {
                snprintf(name, sizeof(name), "GPIO %d", gpio_pin);
            }
        } else {
            snprintf(name, sizeof(name), "GPIO %d", gpio_pin);
        }
        
        // Extract direction
        const char *dir_ptr = strstr(pin_config, "\"direction\":");
        if (dir_ptr != NULL) {
            dir_ptr = strchr(dir_ptr, '"');
            if (dir_ptr != NULL) {
                dir_ptr++; // Skip opening quote
                const char *dir_end = strchr(dir_ptr, '"');
                if (dir_end != NULL) {
                    int dir_len = dir_end - dir_ptr;
                    if (dir_len < sizeof(direction)) {
                        strncpy(direction, dir_ptr, dir_len);
                        direction[dir_len] = '\0';
                    } else {
                        strcpy(direction, "input");
                    }
                } else {
                    strcpy(direction, "input");
                }
            } else {
                strcpy(direction, "input");
            }
        } else {
            strcpy(direction, "input");
        }
        
        // Extract high_unit (active text)
        const char *high_ptr = strstr(pin_config, "\"high_unit\":");
        if (high_ptr != NULL) {
            high_ptr = strchr(high_ptr, '"');
            if (high_ptr != NULL) {
                high_ptr++; // Skip opening quote
                const char *high_end = strchr(high_ptr, '"');
                if (high_end != NULL) {
                    int high_len = high_end - high_ptr;
                    if (high_len < sizeof(high_unit)) {
                        strncpy(high_unit, high_ptr, high_len);
                        high_unit[high_len] = '\0';
                    } else {
                        strcpy(high_unit, "High");
                    }
                } else {
                    strcpy(high_unit, "High");
                }
            } else {
                strcpy(high_unit, "High");
            }
        } else {
            strcpy(high_unit, "High");
        }
        
        // Extract low_unit (inactive text)
        const char *low_ptr = strstr(pin_config, "\"low_unit\":");
        if (low_ptr != NULL) {
            low_ptr = strchr(low_ptr, '"');
            if (low_ptr != NULL) {
                low_ptr++; // Skip opening quote
                const char *low_end = strchr(low_ptr, '"');
                if (low_end != NULL) {
                    int low_len = low_end - low_ptr;
                    if (low_len < sizeof(low_unit)) {
                        strncpy(low_unit, low_ptr, low_len);
                        low_unit[low_len] = '\0';
                    } else {
                        strcpy(low_unit, "Low");
                    }
                } else {
                    strcpy(low_unit, "Low");
                }
            } else {
                strcpy(low_unit, "Low");
            }
        } else {
            strcpy(low_unit, "Low");
        }
        
        // Extract instance
        const char *inst_ptr = strstr(pin_config, "\"instance\":");
        if (inst_ptr != NULL) {
            inst_ptr = strchr(inst_ptr, ':');
            if (inst_ptr != NULL) {
                inst_ptr++;
                while (*inst_ptr == ' ' || *inst_ptr == '\t') inst_ptr++;
                instance = atoi(inst_ptr);
            } else {
                instance = (gpio_pin == 0) ? 24 : gpio_pin;
            }
        } else {
            instance = (gpio_pin == 0) ? 24 : gpio_pin;
        }
        
        // Create BACnet object based on direction
        if (strcmp(direction, "output") == 0) {
            // Create Binary Output
            int bacnet_instance = 4000 + instance;
            if ((obj_ptr = object_new(device_id, OBJECT_BINARY_OUTPUT, bacnet_instance)) != NULL) {
                obj_ptr->name = strdup(name);
                obj_ptr->value.enumerated = 0;
                obj_ptr->units.states.active = strdup(high_unit);
                obj_ptr->units.states.inactive = strdup(low_unit);
                debug_printf(1, "GPIO: Created Binary Output %d (GPIO %d) - %s\n", 
                    bacnet_instance, gpio_pin, name);
            }
        } else {
            // Create Binary Input
            int bacnet_instance = 3000 + instance;
            if ((obj_ptr = object_new(device_id, OBJECT_BINARY_INPUT, bacnet_instance)) != NULL) {
                obj_ptr->name = strdup(name);
                obj_ptr->value.enumerated = 0;
                obj_ptr->units.states.active = strdup(high_unit);
                obj_ptr->units.states.inactive = strdup(low_unit);
                debug_printf(1, "GPIO: Created Binary Input %d (GPIO %d) - %s\n", 
                    bacnet_instance, gpio_pin, name);
            }
        }
    }
}

// Helper function to get object index from instance
static int gpio_get_object_index(uint32_t instance)
{
    switch(instance) {
        case 4018: return 0; // BO Test LED
        case 3019: return 1; // BI Motion Sensor
        case 1020: return 2; // AI Temperature
        case 2021: return 3; // AO Fan Control
        case 4026: return 4; // BO Main Relay
        default: return -1;
    }
}

// Helper function to get effective value from priority array
static union ObjectValue gpio_get_effective_value(uint32_t instance)
{
    int index = gpio_get_object_index(instance);
    union ObjectValue result;
    
    if (index < 0) {
        // Invalid instance, return zero
        result.real = 0.0;
        return result;
    }
    
    struct gpio_priority_array *prio = &gpio_priorities[index];
    
    // If out of service, return present value (not priority array)
    if (prio->out_of_service) {
        struct ObjectRef_Struct *obj_ptr = object_find(BACnet_Device_Instance, 
            (instance >= 4000) ? OBJECT_BINARY_OUTPUT : 
            (instance >= 3000) ? OBJECT_BINARY_INPUT :
            (instance >= 2000) ? OBJECT_ANALOG_OUTPUT : OBJECT_ANALOG_INPUT, 
            instance);
        if (obj_ptr) {
            return obj_ptr->value;
        }
    }
    
    // Check priority array from highest to lowest priority (1-16)
    for (int i = 0; i < BACNET_MAX_PRIORITY; i++) {
        if (prio->priorities_set & (1 << i)) {
            return prio->values[i];
        }
    }
    
    // No priority set, return relinquish default
    return prio->relinquish_default;
}