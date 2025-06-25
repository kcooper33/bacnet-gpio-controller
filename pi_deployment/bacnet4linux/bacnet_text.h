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
#ifndef BACNET_TEXT_H
#define BACNET_TEXT_H

#include "bacnet_enum.h"

const char *enum_to_text_service_confirmed(BACNET_CONFIRMED_SERVICE
    enumeration);
const char *enum_to_text_service_unconfirmed(BACNET_UNCONFIRMED_SERVICE
    enumeration);
const char *enum_to_text_object(enum BACnetObjectType object);
const char *enum_to_text_property(enum BACnetPropertyIdentifier property);
const char *enum_to_text_units(enum BACnetEngineeringUnits units);
const char *enum_to_text_error_code(enum BACnetErrorCode enumeration);
const char *enum_to_text_error_class(enum BACnetErrorClass enumeration);
const char *enum_to_text_reject_reason(enum BACnetRejectReason
    enumeration);
const char *enum_to_text_abort_reason(enum BACnetAbortReason enumeration);

/* pass in an object type name string */
/* returns an object type enumeration */
int text_to_enum_object(const char *search_name);
/* pass in an object property string */
/* returns an object property enumeration */
int text_to_enum_property(const char *search_name);

#endif
