/*####COPYRIGHTBEGIN####
 -------------------------------------------
 GPIO Objects Header for BACnet4Linux - Raspberry Pi Integration
 -------------------------------------------
####COPYRIGHTEND####*/

#ifndef GPIO_OBJECTS_H
#define GPIO_OBJECTS_H

#include "bacnet_struct.h"
#include "bacnet_enum.h"

// Function prototypes
void gpio_objects_init(int device_id);
void gpio_create_default_objects(int device_id);
void gpio_create_objects_from_config(int device_id, const char *json_config);
void gpio_update_inputs(int device_id);
void gpio_objects_update_values(int device_id);
int gpio_handle_read_property(struct BACnet_Device_Address *src, uint8_t invoke_id, 
                             BACNET_OBJECT_TYPE object_type, uint32_t instance,
                             BACNET_PROPERTY_ID property, 
                             uint32_t array_index, uint16_t src_max_apdu);
int gpio_objects_write_property(int object_type,
                               uint32_t instance, uint32_t property,
                               uint8_t tag, void *value, uint8_t priority);
int gpio_encode_relinquish_default(uint8_t *apdu, BACNET_OBJECT_TYPE object_type, uint32_t instance);

#endif /* GPIO_OBJECTS_H */