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
 Modified by Coleman Brumley <cbrumley@users.sourceforge.net> 20 Nov 2003
 Modified by Steve Karg <skarg@users.sourceforge.net> 23 October 2004
 -------------------------------------------
####COPYRIGHTEND####*/
//
// This file contains the bacnet enumeration to text conversions.
//
#include <stdio.h>              /* Standard I/O */
#include <stdlib.h>             /* Standard Library */
#include <stdarg.h>
#include <stddef.h>
#include <ctype.h>

#include "bacnet_enum.h"

static const char *defBACnetPropertyIdentifier[] = {
    "acked-transitions",        //(0)
    "ack-required",             //(1)
    "action",                   //(2)
    "action-text",              //(3)
    "active-text",              //(4)
    "active-vt-sessions",       //(5)
    "alarm-value",              //(6)
    "alarm-values",             //(7)
    "all",                      //(8)
    "all-writes-successful",    //(9)
    "apdu-segment-timeout",     //(10)
    "apdu-timeout",             //(11)
    "application-software-version",     //(12)
    "archive",                  //(13)
    "bias",                     //(14)
    "change-of-state-count",    //(15)
    "change-of-state-time",     //(16)
    "notification-class",       //(17)
    "(deleted in 135-2001)",    //(18)-deleted
    "controlled-variable-reference",    //(19)
    "controlled-variable-units",        //(20)
    "controlled-variable-value",        //(21)
    "cov-increment",            //(22)
    "datelist",                 //(23)
    "daylight-savings-status",  //(24)
    "deadband",                 //(25)
    "derivative-constant",      //(26)
    "derivative-constant-units",        //(27)
    "description",              //(28)
    "description-of-halt",      //(29)
    "device-address-binding",   //(30)
    "device-type",              //(31)
    "effective-period",         //(32)
    "elapsed-active-time",      //(33)
    "error-limit",              //(34)
    "event-enable",             //(35)
    "event-state",              //(36)
    "event-type",               //(37)
    "exception-schedule",       //(38)
    "fault-values",             //(39)
    "feedback-value",           //(40)
    "file-access-method",       //(41)
    "file-size",                //(42)
    "file-type",                //(43)
    "firmware-version",         //(44)
    "high-limit",               //(45)
    "inactive-text",            //(46)
    "in-process",               //(47)
    "instance-of",              //(48)
    "integral-constant",        //(49)
    "integral-constant-units",  //(50)
    "issue-confirmednotifications",     //(51)
    "limit-enable",             //(52)
    "list-of-group-members",    //(53)
    "list-of-object-property-references",       //(54)
    "list-of-session-keys",     //(55)
    "local-date",               //(56)
    "local-time",               //(57)
    "location",                 //(58)
    "low-limit",                //(59)
    "manipulated-variable-reference",   //(60)
    "maximum-output",           //(61)
    "max-apdu-length-accepted", //(62)
    "max-info-frames",          //(63)
    "max-master",               //(64)
    "max-pres-value",           //(65)
    "minimum-off-time",         //(66)
    "minimum-on-time",          //(67)
    "minimum-output",           //(68)
    "min-pres-value",           //(69)
    "model-name",               //(70)
    "modification-date",        //(71)
    "notify-type",              //(72)
    "number-of-APDU-retries",   //(73)
    "number-of-states",         //(74)
    "object-identifier",        //(75)
    "object-list",              //(76)
    "object-name",              //(77)
    "object-property-reference",        //(78)
    "object-type",              //(79)
    "optional",                 //(80)
    "out-of-service",           //(81)
    "output-units",             //(82)
    "event-parameters",         //(83)
    "polarity",                 //(84)
    "present-value",            //(85)
    "priority",                 //(86)
    "priority-array",           //(87)
    "priority-for-writing",     //(88)
    "process-identifier",       //(89)
    "program-change",           //(90)
    "program-location",         //(91)
    "program-state",            //(92)
    "proportional-constant",    //(93)
    "proportional-constant-units",      //(94)
    "protocol-conformance-class",       //(95)
    "protocol-object-types-supported",  //(96)
    "protocol-services-supported",      //(97)
    "protocol-version",         //(98)
    "read-only",                //(99)
    "reason-for-halt",          //(100)
    "recipient",                //(101)
    "recipient-list",           //(102)
    "reliability",              //(103)
    "relinquish-default",       //(104)
    "required",                 //(105)
    "resolution",               //(106)
    "segmentation-supported",   //(107)
    "setpoint",                 //(108)
    "setpoint-reference",       //(109)
    "state-text",               //(110)
    "status-flags",             //(111)
    "system-status",            //(112)
    "time-delay",               //(113)
    "time-of-active-time-reset",        //(114)
    "time-of-state-count-reset",        //(115)
    "time-synchronization-recipients",  //(116)
    "units",                    //(117)
    "update-interval",          //(118)
    "utc-offset",               //(119)
    "vendor-identifier",        //(120)
    "vendor-name",              //(121)
    "vt-classes-supported",     //(122)
    "weekly-schedule",          //(123)
    "attempted-samples",        //(124)
    "average-value",            //(125)
    "buffer-size",              //(126)
    "client-cov-increment",     //(127)
    "cov-resubscription-interval",      //(128)
    "current-notify-time",      //(129)
    "event-time-stamps",        //(130)
    "log-buffer",               //(131)
    "log-device-object-property",       //(132)
    "log-enable",               //(133)
    "log-interval",             //(134)
    "maximum-value",            //(135)
    "minimum-value",            //(136)
    "notification-threshold",   //(137)
    "previous-notify-time",     //(138)
    "protocol-revision",        //(139)
    "records-since-notification",       //(140)
    "record-count",             //(141)
    "start-time",               //(142)
    "stop-time",                //(143)
    "stop-when-full",           //(144)
    "total-record-count",       //(145)
    "valid-samples",            //(146)
    "window-interval",          //(147)
    "window-samples",           //(148)
    "maximum-value-timestamp",  //(149)
    "minimum-value-timestamp",  //(150)
    "variance-value",           //(151)
    "active-cov-subscriptions", //(152)
    "backup-failure-timeout",   //(153)
    "configuration-files",      //(154)
    "database-revision",        //(155)
    "direct-reading",           //(156)
    "last-restore-time",        //(157)
    "maintenance-required",     //(158)
    "member-of",                //(159)
    "mode",                     //(160)
    "operation-expected",       //(161)
    "setting",                  //(162)
    "silenced",                 //(163)
    "tracking-value",           //(164)
    "zone-members",             //(165)
    "life-safety-alarm-values", //(166)
    "max-segments-accepted",    //(167)
    "profile-name",             //(168)
    NULL                        // last
        // 0-511 are ASHRAE reserved
        // 512-4194303 are proprietary
};

//BACnetEngineering Units as defined in section 21 of ASHRAE 135-2001
static const char *defBACnetEngineeringUnits[] = {
/*  0 - 5*/ "square-meters", "square-feet", "milliamperes", "amperes",
    "ohms",
/*  6- 10*/ "volts", "kilovolts", "megavolts", "volt-amperes",
    "kilovolt-amperes",
/* 11- 15*/ "megavolt-amperes", "volt-amperes-reactive",
    "kilovolt-amperes-reactive", "megavolt-amperes-reactive",
    "degrees-phase",
/* 16- 20*/ "power-factor", "joules", "kilojoules", "watt-hours",
    "kilowatt-hours",
/* 21- 25*/ "btus", "therms", "ton-hours",
    "joules-per-kilogram-dry-air", "btus-per-pound-dry-air",
/* 26- 30*/ "cycles-per-hour", "cycles-per-minute", "hertz",
    "grams-of-water-per-kilogram-dry-air",
/* 31- 35*/ "percent-relative-humidity", "millimeters", "meters",
    "inches", "feet",
/* 36- 40*/ "watts-per-square-foot", "watts-per-square-meter",
    "lumens", "luxes", "foot-candles",
/* 41- 45*/ "kilograms", "pounds-mass", "tons", "kilograms-per-second",
    "kilograms-per-minute",
/* 46- 50*/ "kilograms-per-hour", "pounds-mass-per-minute",
    "pounds-mass-per-hour", "watts", "kilowatts",
/* 51- 55*/ "megawatts", "btus-per-hour", "horsepower",
    "tons-refrigeration", "pascals",
/* 56- 60*/ "kilopascals", "bars", "pounds-force-per-square-inch",
    "centimeters-of-water", "inches-of-water",
/* 61- 65*/ "millimeters-of-mercury", "centimeters-of-mercury",
    "inches-of-mercury", "degrees-celsius", "degrees-kelvin",
/* 66- 70*/ "degrees-fahrenheit", "degree-days-celsius",
    "degree-days-fahrenheit", "years", "months",
/* 71- 75*/ "weeks", "days", "hours", "minutes", "seconds",
/* 76- 80*/ "meters-per-second", "kilometers-per-hour",
    "feet-per-second", "feet-per-minute", "miles-per-hour",
/* 81- 85*/ "cubic-feet", "cubic-meters", "imperial-gallons", "liters",
    "us-gallons",
/* 85- 90*/ "cubic-feet-per-minute", "cubic-meters-per-second",
    "imperial-gallons-per-minute", "liters-per-second",
    "liters-per-minute",
/* 91- 95*/ "us-gallons-per-minute", "degrees-angular",
    "degrees-celsius-per-hour", "degrees-celsius-per-minute",
    "degrees-fahrenheit-per-hour",
/* 96-100*/ "degrees-fahrenheit-per-minute", "no-units",
    "parts-per-million", "parts-per-billion", "percent",
/*101-105*/ "percent-per-second", "per-minute", "per-second",
    "psi-per-degree-fahrenheit", "radians",
/*106-110*/ "revolutions-per-minute", "currency1", "currency2",
    "currency3", "currency4",
/*111-115*/ "currency5", "currency6", "currency7", "currency8",
    "currency9",
/*116-120*/ "currency10", "square-inches", "square-centimeters",
    "btus-per-pound", "centimeters",
/*121-125*/ "pounds-mass-per-second", "delta-degrees-fahrenheit",
    "delta-degrees-kelvin", "kilohms", "megohms",
/*126-130*/ "millivolts", "kilojoules-per-kilogram", "megajoules",
    "joulesperdegreekelvin", "joules-per-kilogram-degree-kelvin",
/*131-135*/ "kilohertz", "megahertz", "per-hour", "milliwatts",
    "hectopascals",
/*136-140*/ "millibars", "cubic-meters-per-hour", "liters-per-hour",
    "kilowatt-hours-per-square-meter",
    "kilowatt-hours-per-square-foot",
/*141-144*/ "megajoules-per-square-meter",
    "megajoules-per-square-foot",
    "watts-per-square-meter-degree-delvin", "cubic-feet-per-second",
/*145 - 165 Are from Addendum 135a */
/*145-150*/ "milliohms", "megawatt-hours", "kilo-btus", "mega-btus",
    "kilojoules-per-kilogram-dry-air",
    "megajoules-per-kilogram-dry-air",
/*151-154*/ "kilojoules-per-degree-Kelvin",
    "megajoules-per-degree-Kelvin", "newton", "grams-per-second",
/*155-150*/ "grams-per-minute", "tons-per-hour", "kilo-btus-per-hour",
    "hundredths-seconds", "milliseconds", "newton-meters",
/*161-164*/ "millimeters-per-second", "millimeters-per-minute",
    "meters-per-minute", "meters-per-hour",
/*165-170*/ "cubic-meters-per-minute",
    NULL                        // last
        // 0-255 are ASHRAE reserved
        // 256-65535 are proprietary
};

static const char *defBACnetObjectType[] = {
    "Analog Input",             // (0)
    "Analog output",            // (1)
    "Analog Value",             // (2)
    "Binary Input",             // (3)
    "Binary Output",            // (4)
    "Binary Value",             // (5)
    "Calendar",                 // (6)
    "Command",                  // (7)
    "Device",                   // (8)
    "Event Enrollment",         // (9)
    "File",                     // (10)
    "Group",                    // (11)
    "Loop",                     // (12)
    "Multi-State Input",        // (13)
    "Multi-State Output",       // (14)  
    "Notification Class",       // (15)
    "Program",                  // (16)
    "Schedule",                 // (17)
    "Averaging",                // (18) 
    "Multi-State Value",        // (19) 
    "Trend Log",                // (20)
    "Life Safety Point",        // 21
    "Life Safety Zone",         // 22
    NULL                        // last 
        // 0-127 are ASHRAE reserved
        // 128-1023 are proprietary
};

static const char *defBACnetAbortReason[] = {
    "Abort: Other",
    "Abort: Buffer Overflow",
    "Abort: Invalid APDU in this State",
    "Abort: Preempted by Higher Priority Task",
    "Abort: Segmentation Not Supported",
    // Enumerated values 0-63 are reserved for definition by ASHRAE.
    // Enumerated values 64-65535 may be used by others subject to
    // the procedures and constraints described in Clause 23.
    NULL                        // last 
};

static const char *defBACnetRejectReason[] = {
    "Reject: Other",
    "Reject: Buffer Overflow",
    "Reject: Inconsistent Parameters",
    "Reject: Invalid Parameter Data Type",
    "Reject: Invalid Tag",
    "Reject: Missing Required Parameter",
    "Reject: Parameter Out of Range",
    "Reject: Too Many Arguments",
    "Reject: Undefined Enumeration",
    "Reject: Unrecognized Service",
    // Enumerated values 0-63 are reserved for definition by ASHRAE.
    // Enumerated values 64-65535 may be used by others subject to
    // the procedures and constraints described in Clause 23.
    NULL                        // last 
};

static const char *defBACnetErrorClass[] = {
    "ERROR CLASS: DEVICE",
    "ERROR CLASS: OBJECT",
    "ERROR CLASS: PROPERTY",
    "ERROR CLASS: RESOURCES",
    "ERROR CLASS: SECURITY",
    "ERROR CLASS: SERVICES",
    "ERROR CLASS: VT",
    // Enumerated values 0-63 are reserved for definition by ASHRAE.
    // Enumerated values 64-65535 may be used by others subject to
    // the procedures and constraints described in Clause 23.
    NULL                        // last 
};

static const char *defBACnetErrorCode[] = {
    "ERROR CODE: OTHER",
    "ERROR CODE: AUTHENTICATION_FAILED",
    "ERROR CODE: CONFIGURATION_IN_PROGRESS",
    "ERROR CODE: DEVICE_BUSY",
    "ERROR CODE: DYNAMIC_CREATION_NOT_SUPPORTED",
    "ERROR CODE: FILE_ACCESS_DENIED",
    "ERROR CODE: INCOMPATIBLE_SECURITY_LEVELS",
    "ERROR CODE: INCONSISTENT_PARAMETERS",
    "ERROR CODE: INCONSISTENT_SELECTION_CRITERION",
    "ERROR CODE: INVALID_DATA_TYPE",
    "ERROR CODE: INVALID_FILE_ACCESS_METHOD",
    "ERROR CODE: ERROR_CODE_INVALID_FILE_START_POSITION",
    "ERROR CODE: INVALID_OPERATOR_NAME",
    "ERROR CODE: INVALID_PARAMETER_DATA_TYPE",
    "ERROR CODE: INVALID_TIME_STAMP",
    "ERROR CODE: KEY_GENERATION_ERROR",
    "ERROR CODE: MISSING_REQUIRED_PARAMETER",
    "ERROR CODE: NO_OBJECTS_OF_SPECIFIED_TYPE",
    "ERROR CODE: NO_SPACE_FOR_OBJECT",
    "ERROR CODE: NO_SPACE_TO_ADD_LIST_ELEMENT",
    "ERROR CODE: NO_SPACE_TO_WRITE_PROPERTY",
    "ERROR CODE: NO_VT_SESSIONS_AVAILABLE",
    "ERROR CODE: OBJECT_DELETION_NOT_PERMITTED",
    "ERROR CODE: OBJECT_IDENTIFIER_ALREADY_EXISTS",
    "ERROR CODE: OPERATIONAL_PROBLEM",
    "ERROR CODE: PASSWORD_FAILURE",
    "ERROR CODE: PROPERTY_IS_NOT_A_LIST",
    "ERROR CODE: READ_ACCESS_DENIED",
    "ERROR CODE: SECURITY_NOT_SUPPORTED",
    "ERROR CODE: SERVICE_REQUEST_DENIED",
    "ERROR CODE: TIMEOUT",
    "ERROR CODE: UNKNOWN_OBJECT",
    "ERROR CODE: UNKNOWN_PROPERTY",
    "ERROR CODE: this enumeration was removed",
    "ERROR CODE: UNKNOWN_VT_CLASS",
    "ERROR CODE: UNKNOWN_VT_SESSION",
    "ERROR CODE: UNSUPPORTED_OBJECT_TYPE",
    "ERROR CODE: VALUE_OUT_OF_RANGE",
    "ERROR CODE: VT_SESSION_ALREADY_CLOSED",
    "ERROR CODE: VT_SESSION_TERMINATION_FAILURE",
    "ERROR CODE: WRITE_ACCESS_DENIED",
    "ERROR CODE: CHARACTER_SET_NOT_SUPPORTED",
    "ERROR CODE: INVALID_ARRAY_INDEX",
    "ERROR CODE: COV_SUBSCRIPTION_FAILED",
    "ERROR CODE: NOT_COV_PROPERTY",
    "ERROR CODE: OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED",
    "ERROR CODE: INVALID_CONFIGURATION_DATA",
    // Enumerated values 0-255 are reserved for definition by ASHRAE.
    // Enumerated values 256-65535 may be used by others subject to
    // the procedures and constraints described in Clause 23.
    // The last enumeration used in this version is 46.
    NULL                        // last 
};

static const char *defBACnetServiceConfirmed[] = {
    // Alarm and Event Services
    "CONFIRMED SERVICE: ACKNOWLEDGE_ALARM",
    "CONFIRMED SERVICE: COV_NOTIFICATION",
    "CONFIRMED SERVICE: EVENT_NOTIFICATION",
    "CONFIRMED SERVICE: GET_ALARM_SUMMARY",
    "CONFIRMED SERVICE: GET_ENROLLMENT_SUMMARY",
    "CONFIRMED SERVICE: SUBSCRIBE_COV",
    "CONFIRMED SERVICE: ATOMIC_READ_FILE",
    "CONFIRMED SERVICE: ATOMIC_WRITE_FILE",
    "CONFIRMED SERVICE: ADD_LIST_ELEMENT",
    "CONFIRMED SERVICE: REMOVE_LIST_ELEMENT",
    "CONFIRMED SERVICE: CREATE_OBJECT",
    "CONFIRMED SERVICE: DELETE_OBJECT",
    "CONFIRMED SERVICE: READ_PROPERTY",
    "CONFIRMED SERVICE: READ_PROPERTY_CONDITIONAL",
    "CONFIRMED SERVICE: READ_PROPERTY_MULTIPLE",
    "CONFIRMED SERVICE: WRITE_PROPERTY",
    "CONFIRMED SERVICE: WRITE_PROPERTY_MULTIPLE",
    "CONFIRMED SERVICE: DEVICE_COMMUNICATION_CONTROL",
    "CONFIRMED SERVICE: PRIVATE_TRANSFER",
    "CONFIRMED SERVICE: TEXT_MESSAGE",
    "CONFIRMED SERVICE: REINITIALIZE_DEVICE",
    "CONFIRMED SERVICE: VT_OPEN",
    "CONFIRMED SERVICE: VT_CLOSE",
    "CONFIRMED SERVICE: VT_DATA",
    "CONFIRMED SERVICE: AUTHENTICATE",
    "CONFIRMED SERVICE: REQUEST_KEY",
    "CONFIRMED SERVICE: READ_RANGE",
    "CONFIRMED SERVICE: LIFE_SAFETY_OPERATION",
    "CONFIRMED SERVICE: SUBSCRIBE_COV_PROPERTY",
    "CONFIRMED SERVICE: GET_EVENT_INFORMATION",
    // Services added after 1995
    // readRange (26) see Object Access Services
    // lifeSafetyOperation (27) see Alarm and Event Services
    // subscribeCOVProperty (28) see Alarm and Event Services
    // getEventInformation (29) see Alarm and Event Services
    NULL                        // last 
};

static const char *defBACnetServiceUnconfirmed[] = {
    "UNCONFIRMED SERVICE: I_AM",
    "UNCONFIRMED SERVICE: I_HAVE",
    "UNCONFIRMED SERVICE: COV_NOTIFICATION",
    "UNCONFIRMED SERVICE: EVENT_NOTIFICATION",
    "UNCONFIRMED SERVICE: PRIVATE_TRANSFER",
    "UNCONFIRMED SERVICE: TEXT_MESSAGE",
    "UNCONFIRMED SERVICE: TIME_SYNCHRONIZATION",
    "UNCONFIRMED SERVICE: WHO_HAS",
    "UNCONFIRMED SERVICE: WHO_IS",
    "UNCONFIRMED SERVICE: UTC_TIME_SYNCHRONIZATION",
    // Other services to be added as they are defined.
    // All choice values in this production are reserved
    // for definition by ASHRAE.
    // Proprietary extensions are made by using the
    // UnconfirmedPrivateTransfer service. See Clause 23.
    NULL                        // last
};

// returns the number of entries in a NULL terminated list of strings
static unsigned string_list_len(const char **string_list)
{
    unsigned len = 0;

    if (string_list) {
        while (string_list[len]) {
            len++;
        }
    }

    return len;
}

// compare strings, but ignore differences made by space or hyphen
int name_comp(const char *string1, const char *string2)
{
    int diff_count = 0;
    while (*string1 && *string2) {
        if (tolower(*string1) != tolower(*string2)) {
            if ((isspace(*string1) || (*string1 == '-')
                    || (*string1 == '_')) && (isspace(*string2)
                    || (*string2 == '-') || (*string2 == '_'))) {
                // great! do nothing...
            } else if ((isspace(*string2) || (*string2 == '-')
                    || (*string2 == '_')) && (isspace(*string1)
                    || (*string1 == '-') || (*string1 == '_'))) {
                // great! do nothing...
            } else
                diff_count++;
        }
    }

    return diff_count;
}

// generic function to return the enumeration
static int text_to_enum(const char *search_name,
    unsigned *len, const char **string_list)
{
    int index = -1;             // return value
    int i = 0;                  // counter

    if (*len == 0)
        *len = string_list_len(string_list);
    for (i = 0; i < *len; i++) {
        if (name_comp(string_list[index], search_name) == 0) {
            index = i;
            break;
        }
    }

    return index;
}

// generic function to return the text
static const char *enum_to_text(int enumeration,
    const char *unknown_text,
    int proprietary_limit,
    const char *proprietary_text,
    int invalid_limit,
    const char *invalid_text, unsigned *len, const char **string_list)
{
    const char *text = unknown_text;

    if (*len == 0)
        *len = string_list_len(string_list);
    if (enumeration < *len)
        text = string_list[enumeration];
    else if (enumeration > invalid_limit)
        text = invalid_text;
    else if (enumeration > proprietary_limit)
        text = proprietary_text;

    return text;
}

// generic function to return the text
static const char *enum_to_text_simple(int enumeration,
    const char *unknown_text, unsigned *len, const char **string_list)
{
    const char *text = unknown_text;

    if (*len == 0)
        *len = string_list_len(string_list);
    if (enumeration < *len)
        text = string_list[enumeration];

    return text;
}

const char *enum_to_text_service_confirmed(BACNET_CONFIRMED_SERVICE
    enumeration)
{
    static unsigned len = 0;    // length of the string list

    return enum_to_text_simple(enumeration,
        "Confirmed Service: unknown service", &len,
        defBACnetServiceConfirmed);
}

const char *enum_to_text_service_unconfirmed(BACNET_UNCONFIRMED_SERVICE
    enumeration)
{
    static unsigned len = 0;    // length of the string list

    return enum_to_text_simple(enumeration,
        "Unconfirmed Service: unknown service", &len,
        defBACnetServiceUnconfirmed);
}

/* pass in an error code */
/* return a text string */
const char *enum_to_text_error_code(enum BACnetErrorCode enumeration)
{
    static unsigned len = 0;    // length of the string list

    return enum_to_text(enumeration, "Error Code: unknown code", 255,   // proprietary_limit
        "Error Code: proprietary code", 65535,  // invalid_limit
        "Error Code: invalid code", &len, defBACnetErrorCode);
}

/* pass in an error class */
/* return a text string */
const char *enum_to_text_error_class(enum BACnetErrorClass enumeration)
{
    static unsigned len = 0;    // length of the string list

    return enum_to_text(enumeration, "Error Class: unknown class", 63,  // proprietary_limit
        "Error Class: proprietary class", 65535,        // invalid_limit
        "Error Class: invalid class", &len, defBACnetErrorClass);
}

/* pass in an reject reason */
/* return a text string */
const char *enum_to_text_reject_reason(enum BACnetRejectReason enumeration)
{
    static unsigned len = 0;    // length of the string list

    return enum_to_text(enumeration, "Reject: unknown reject reason", 63,       // proprietary_limit
        "Reject: proprietary reason", 65535,    // invalid_limit
        "Reject: invalid reason", &len, defBACnetRejectReason);
}

/* pass in an abort reason */
/* return a text string */
const char *enum_to_text_abort_reason(enum BACnetAbortReason enumeration)
{
    static unsigned len = 0;    // length of the string list

    return enum_to_text(enumeration, "Abort: unknown abort reason", 63, // proprietary_limit
        "Abort: proprietary reason", 65535,     // invalid_limit
        "Abort: invalid reason", &len, defBACnetAbortReason);
}

/* pass in an object type */
/* return a text string */
const char *enum_to_text_object(enum BACnetObjectType object)
{
    static unsigned len = 0;    // length of the string list

    return enum_to_text(object, "Unknown Object", 127,  // proprietary_limit
        "Proprietary Object", 1023,     // invalid_limit
        "Invalid Object", &len, defBACnetObjectType);
}

/* pass in an object type name string */
/* returns an object type enumeration */
int text_to_enum_object(const char *search_name)
{
    static unsigned len = 0;    // length of the string list

    return text_to_enum(search_name, &len, defBACnetObjectType);
}

/* returns a text property name from the enumerated type */
const char *enum_to_text_property(enum BACnetPropertyIdentifier property)
{
    static unsigned len = 0;    // length of the string list

    return enum_to_text(property, "Unknown Property", 511,      // proprietary_limit
        "Proprietary Property", 4194303,        // invalid_limit
        "Invalid Property", &len, defBACnetPropertyIdentifier);
}

/* pass in an object property string */
/* returns an object property enumeration */
int text_to_enum_property(const char *search_name)
{
    static unsigned len = 0;    // length of the string list

    return text_to_enum(search_name, &len, defBACnetPropertyIdentifier);
}

/* returns a text units name from the enumerated type */
const char *enum_to_text_units(enum BACnetEngineeringUnits units)
{
    static unsigned len = 0;    // length of the string list

    return enum_to_text(units, "Unknown Units", 255,    // proprietary_limit
        "Proprietary Units", 65535,     // invalid_limit
        "Invalid Units", &len, defBACnetEngineeringUnits);
}

#ifdef TEST
#include <assert.h>
#include <string.h>

#include "ctest.h"

void testEngineeringUnits(Test * pTest)
{
    int i;
    const char *pstring = NULL;
    const char *test_string = NULL;

    for (i = 0; i < 260; i++) {
        pstring = enum_to_text_units((enum BACnetEngineeringUnits) i);
        //test_string = defBACnetEngineeringUnits[i];
        //if (test_string == NULL)
        //  printf("units index=%d",i);
        ct_test(pTest, pstring != NULL);
    }

    // test the boundary points
    // 0-255 are ASHRAE reserved
    // 256-65535 are proprietary
    pstring = enum_to_text_units((enum BACnetEngineeringUnits) 0);
    test_string = defBACnetEngineeringUnits[0];
    ct_test(pTest, pstring == test_string);

    // last known
    pstring = enum_to_text_units((enum BACnetEngineeringUnits) 163);
    test_string = defBACnetEngineeringUnits[163];
    ct_test(pTest, pstring == test_string);

    // last unknowns - should match if we haven't filled all the enums
    pstring = enum_to_text_units((enum BACnetEngineeringUnits) 254);
    test_string = enum_to_text_units((enum BACnetEngineeringUnits) 255);
    ct_test(pTest, pstring == test_string);

    // first proprietary
    // last proprietary
    pstring = enum_to_text_units((enum BACnetEngineeringUnits) 256);
    test_string = enum_to_text_units((enum BACnetEngineeringUnits) 65535);
    ct_test(pTest, pstring == test_string);

    // invalid
    pstring = enum_to_text_units((enum BACnetEngineeringUnits) 65536);
    ct_test(pTest, pstring != test_string);

    return;
}

void testPropertyIdentifier(Test * pTest)
{
    int i;
    const char *pstring = NULL;
    const char *test_string = NULL;

    for (i = 0; i < 520; i++) {
        pstring = enum_to_text_property((enum BACnetPropertyIdentifier) i);
        ct_test(pTest, pstring != NULL);
    }

    // test the boundary points
    // 0-511 are ASHRAE reserved
    // 512-4194303 are proprietary
    pstring = enum_to_text_property((enum BACnetPropertyIdentifier) 0);
    test_string = defBACnetPropertyIdentifier[0];
    ct_test(pTest, strcmp(pstring, test_string) == 0);

    // last known
    pstring = enum_to_text_property((enum BACnetPropertyIdentifier) 168);
    test_string = defBACnetPropertyIdentifier[168];
    ct_test(pTest, strcmp(pstring, test_string) == 0);

    // last unknowns
    pstring = enum_to_text_property((enum BACnetPropertyIdentifier) 510);
    test_string =
        enum_to_text_property((enum BACnetPropertyIdentifier) 511);
    ct_test(pTest, strcmp(pstring, test_string) == 0);

    // first proprietary
    // last proprietary
    pstring = enum_to_text_property((enum BACnetPropertyIdentifier) 512);
    test_string =
        enum_to_text_property((enum BACnetPropertyIdentifier) 4194303);
    ct_test(pTest, strcmp(pstring, test_string) == 0);

    // invalid
    pstring =
        enum_to_text_property((enum BACnetPropertyIdentifier) 4194304);
    ct_test(pTest, strcmp(pstring, test_string) != 0);

    return;
}

void testObjectType(Test * pTest)
{
    int i;
    const char *pstring = NULL;
    const char *test_string = NULL;

    for (i = 0; i < 140; i++) {
        pstring = enum_to_text_object((enum BACnetObjectType) i);
        ct_test(pTest, pstring != NULL);
    }

    // test the boundary points
    // 0-127 are ASHRAE reserved
    // 128-1023 are proprietary
    pstring = enum_to_text_object((enum BACnetObjectType) 0);
    test_string = defBACnetObjectType[0];
    ct_test(pTest, strcmp(pstring, test_string) == 0);

    // last known
    pstring = enum_to_text_object((enum BACnetObjectType) 20);
    test_string = defBACnetObjectType[20];
    ct_test(pTest, strcmp(pstring, test_string) == 0);

    // last unknowns
    pstring = enum_to_text_object((enum BACnetObjectType) 126);
    test_string = enum_to_text_object((enum BACnetObjectType) 127);
    ct_test(pTest, strcmp(pstring, test_string) == 0);

    // first proprietary
    // last proprietary
    pstring = enum_to_text_object((enum BACnetObjectType) 128);
    test_string = enum_to_text_object((enum BACnetObjectType) 1023);
    ct_test(pTest, strcmp(pstring, test_string) == 0);

    // invalid
    pstring = enum_to_text_object((enum BACnetObjectType) 1024);
    ct_test(pTest, strcmp(pstring, test_string) != 0);

    return;
}

#ifdef TEST_BACNET_TEXT
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("bacnet text", NULL);

    /* individual tests */
    rc = ct_addTestFunction(pTest, testEngineeringUnits);
    assert(rc);
    rc = ct_addTestFunction(pTest, testPropertyIdentifier);
    assert(rc);
    rc = ct_addTestFunction(pTest, testObjectType);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);

    ct_destroy(pTest);

    return 0;
}
#endif                          // TEST_BACNET_TEXT
#endif                          // TEST
