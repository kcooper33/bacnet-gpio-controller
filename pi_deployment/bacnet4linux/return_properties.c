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
// this function returns the required properties for a certain object 
//
#include "bacnet_struct.h"
#include "bacnet_enum.h"
#include "bacnet_const.h"
#include "bacnet_api.h"
#include "bacnet_device.h"
#include "debug.h"

// FIXME: Should we only have required, optional, or proprietary properties? 
static enum BACnetPropertyIdentifier Device_Property_List[] = {
    PROP_OBJECT_NAME,
//    PROP_SYSTEM_STATUS,
//    PROP_VENDOR_NAME,
    PROP_VENDOR_IDENTIFIER,
//    PROP_MODEL_NAME,
//    PROP_FIRMWARE_REVISION,
//    PROP_APPLICATION_SOFTWARE_VERSION,
//  PROP_LOCATION,
//  PROP_DESCRIPTION,
//    PROP_PROTOCOL_VERSION,
//    PROP_PROTOCOL_REVISION,
//    PROP_PROTOCOL_SERVICES_SUPPORTED,
//    PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED,
    PROP_OBJECT_LIST,
    PROP_MAX_APDU_LENGTH_ACCEPTED,
    PROP_SEGMENTATION_SUPPORTED,
//  PROP_LOCAL_DATE,
//  PROP_LOCAL_TIME,
//  PROP_UTC_OFFSET,
//  PROP_DAYLIGHT_SAVINGS_STATUS,
//  PROP_APDU_SEGMENT_TIMEOUT,
//    PROP_APDU_TIMEOUT,
//    PROP_NUMBER_OF_APDU_RETRIES,
//  PROP_PROTOCOL_CONFORMANCE_CLASS, // deleted in version 1 revision 2
//  PROP_MAX_INFO_FRAMES,
//  PROP_MAX_MASTER,
    PROP_NO_PROPERTY
};

static enum BACnetPropertyIdentifier Analog_Input_Property_List[] = {
    PROP_OBJECT_NAME,
//  PROP_DESCRIPTION,
    PROP_PRESENT_VALUE,
    PROP_UNITS,
//    PROP_OUT_OF_SERVICE,
//  PROP_COV_INCREMENT,
    PROP_NO_PROPERTY
};

static enum BACnetPropertyIdentifier Analog_Output_Property_List[] = {
    PROP_OBJECT_NAME,
//  PROP_DESCRIPTION,
    PROP_PRESENT_VALUE,
    PROP_UNITS,
//    PROP_OUT_OF_SERVICE,
//  PROP_COV_INCREMENT,
//    PROP_PRIORITY_ARRAY,
    PROP_NO_PROPERTY
};

static enum BACnetPropertyIdentifier Analog_Value_Property_List[] = {
    PROP_OBJECT_NAME,
//  PROP_DESCRIPTION,
    PROP_PRESENT_VALUE,
    PROP_UNITS,
    PROP_OUT_OF_SERVICE,
//  PROP_COV_INCREMENT,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    PROP_NO_PROPERTY
};

static enum BACnetPropertyIdentifier Binary_Input_Property_List[] = {
    PROP_OBJECT_NAME,
//  PROP_DESCRIPTION,
    PROP_PRESENT_VALUE,
    PROP_UNITS,
    PROP_OUT_OF_SERVICE,
//  PROP_COV_INCREMENT,
    PROP_ACTIVE_TEXT,
    PROP_INACTIVE_TEXT,
    PROP_NO_PROPERTY
};

static enum BACnetPropertyIdentifier Binary_Output_Property_List[] = {
    PROP_OBJECT_NAME,
//  PROP_DESCRIPTION,
    PROP_PRESENT_VALUE,
    PROP_UNITS,
    PROP_OUT_OF_SERVICE,
//  PROP_COV_INCREMENT,
    PROP_ACTIVE_TEXT,
    PROP_INACTIVE_TEXT,
    PROP_PRIORITY_ARRAY,
    PROP_RELINQUISH_DEFAULT,
    PROP_NO_PROPERTY
};

static enum BACnetPropertyIdentifier Binary_Value_Property_List[] = {
    PROP_OBJECT_NAME,
//    PROP_DESCRIPTION,
    PROP_PRESENT_VALUE,
    PROP_UNITS,
//    PROP_OUT_OF_SERVICE,
//  PROP_COV_INCREMENT,
    PROP_ACTIVE_TEXT,
    PROP_INACTIVE_TEXT,
//    PROP_PRIORITY_ARRAY,
    PROP_NO_PROPERTY
};

static enum BACnetPropertyIdentifier Generic_Property_List[] = {
    PROP_OBJECT_NAME,
//  PROP_DESCRIPTION,
    PROP_NO_PROPERTY
};

enum BACnetPropertyIdentifier *getobjectprops(enum BACnetObjectType type)
{
    enum BACnetPropertyIdentifier *property_list = NULL;        // return value

    debug_printf(5, "getobjectprops: entered\n");

    switch (type) {
    case OBJECT_DEVICE:        /* Device object */
        property_list = Device_Property_List;
        break;
    case OBJECT_ANALOG_INPUT:  /* Analog Input */
        property_list = Analog_Input_Property_List;
        break;
    case OBJECT_ANALOG_OUTPUT: /* Analog Output */
        property_list = Analog_Output_Property_List;
        break;
    case OBJECT_ANALOG_VALUE:  /* Analog Value */
        property_list = Analog_Value_Property_List;
        break;
    case OBJECT_BINARY_INPUT:  /* Binary Input */
        property_list = Binary_Input_Property_List;
        break;
    case OBJECT_BINARY_OUTPUT: /* Binary Output */
        property_list = Binary_Output_Property_List;
        break;
    case OBJECT_BINARY_VALUE:  /* Binary Value */
        property_list = Binary_Value_Property_List;
        break;
    default:
        property_list = Generic_Property_List;
        break;
    }

    return property_list;
}
