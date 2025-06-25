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
#ifndef BACNET_STRUCT_H
#define BACNET_STRUCT_H

/* BACnet common structures, unions used */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdbool.h>

#include "bacnet_enum.h"
#include "bacnet_const.h"
#include "keylist.h"

/* Structures */

#define STRUCTURES

// saved info from I-Am
struct BACnet_Device_Address {
    struct in_addr ip;
    uint8_t mac[MAX_MAC_LEN];
    // the following are used if the device is behind a router
    // FIXME: is local needed?  We could use net = 0 to indicate local
    bool local;                 /* false=remote device, true=local device */
    int len;                    /* length of MAC address */
    int net;                    /* BACnet network number */
    char adr[MAX_MAC_LEN];      /* hwaddr (MAC) address */
};

/* Data structure for a BACnet NPDU */
struct BACnet_NPDU {
    struct BACnet_Device_Address src;
    struct BACnet_Device_Address dest;
    bool local_broadcast;       // true for BIP for BVLL
    bool src_present;           /* 1==snet,slen,hop_count present, 0==snet,slen,sadr,hop_count absent (bit 5) */
    bool dest_present;          /* 1==dnet,dlen,hop_count present, 0==dnet,dlen,dadr,hop_count absent (bit 5) */
    unsigned int version;       /* Protocol Version Number */
    int control_byte;           /* control byte (bit encoded) */
    bool network_message;       /* 1==net message, 0==APDU (bit 7) */
    bool expecting_reply;       /* 1==reply expected, 0==no reply expected (bit 2) */
    signed char net_priority;   /* 0-3 with 3 being highest priority (bits 1,0) */
    int message_type;           /* if there is no APDU, the network message will be here */
    int vendor_id;              /* if the message_type is 0x80-0xFF, then the vendor id will be here */
    int hop_count;              /* hop count to limit circular network damage */
    uint8_t *pdu;               // dynamically allocated buffer
    int pdu_len;                // len of dynamically allocated buffer
};

enum device_state {
    DEVICE_STATE_INIT = 0,
    DEVICE_STATE_QUERY_DEVICE_PROPERTIES = 1,
    DEVICE_STATE_QUERY_OBJECT_LIST = 2,
    DEVICE_STATE_QUERY_OBJECT_LIST_PROPERTIES = 3,
    DEVICE_STATE_SUBSCRIBE_COV = 4,
    DEVICE_STATE_REQUEST_PRESENT_VALUE = 5,
    DEVICE_STATE_IDLE = 6,
    DEVICE_STATE_ERROR = 7,
};

/* structure used to keep information on a device */
struct BACnet_Device_Info {
    // object properties
    int device;                 /* BACnet device address */
    int vendor_id;              /* Vendor ID # */
    int max_apdu;               /* maximum APDU length accepted */
    int seg_support;            /* segmentation supported (0==both, 1==transmit, 2==receive, 3==none) */
    char *device_name;          /* Device's Object Name */
    int true_num_objects;       /* true number of objects in this device (value of 'objectlist[0]') */
    // local vars for operation
    time_t last_found;          /* time this device last responded */
    int prop_count;             /* which property we are gathering */
    int object_index;           /* which object index we are gathering */
    enum device_state state;    /* which step in the gathering process we are at */
    // stores the list of objects
    OS_Keylist object_list;     /* handle to list of interesting objects */
    // the device address
    struct BACnet_Device_Address src;
};

////////////  Unions / Structures to build an Object Reference //////////
/* union to hold the value an object has */
union ObjectValue {
    float real;                 /* single precision floating point */
    unsigned char binary;       /* binary */
    int integer;                /* unsigned */
    int enumerated;             /* enumerated */
};
/* structure to hold binary active/inactive state names */
struct BinaryStateNames_Struct {
    char *active;
    char *inactive;
};
/* union to hold analog units or binary states */
union ObjectUnits_Union {
    BACNET_ENGINEERING_UNITS units;     /* analog units */
    struct BinaryStateNames_Struct states;      /* binary units */
};
/* structure to hold an object reference */
struct ObjectRef_Struct {
    BACNET_OBJECT_TYPE type;    /* what type of object? */
    int instance;               /*    the instance of this object */
    char *name;                 /*    pointer to where the name of the object is stored */
    union ObjectValue value;    /*  value this object currently has */
    union ObjectUnits_Union units;      /*  analog units / binary states (20 chars) */
    time_t last_subscribe_COV;  /* time of last subscribe COV */
};

///////////////////// End Object Reference ////////////////////////////////

////////////////// Super 'Object' that has all properties /////////////////
#define MAXSTRING 256

struct AllPropertyObject_Struct {
    char active_text[MAXSTRING];
    int apdu_segment_timeout;
    int apdu_timeout;
    char application_software_version[MAXSTRING];
    float cov_increment;
    float deadband;
    char description[MAXSTRING];
    char firmware_revision[MAXSTRING];
    char inactive_text[MAXSTRING];
    char location[MAXSTRING];
    int max_apdu_length_accepted;
    char model_name[MAXSTRING];
    int number_of_apdu_retries;

};

#endif
