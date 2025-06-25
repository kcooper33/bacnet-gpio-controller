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
#ifndef INVOKE_ID_H
#define INVOKE_ID_H

#include "os.h"
#include "bacnet_struct.h"

/* this enumerated type and structure are used to hold the status of all */
/* invoke IDs, whether they are in use or not */
enum Invoke_Status {
    INVOKE_STATUS_NOACTIVITY = 0,
    INVOKE_STATUS_SENT = 1,
    INVOKE_STATUS_RESENT = 2,
    INVOKE_STATUS_RECEIVED = 3,
    INVOKE_STATUS_NOANSWER = 4
};

struct Invoke_Status_Struct {
    enum Invoke_Status status;  /* current status of this invoke ID */
    time_t time_sent;           /* time that the request was sent */
    struct BACnet_NPDU npdu;    /* NPDU that was sent */
    uint8_t apdu[MAX_APDU];
    int apdu_len;
};

/* invoke ID functions */
void invoke_id_init(void);
int invoke_id(void);
int invoke_id_in_use(void);
void invoke_id_reset(int invokeID);
void invoke_id_cleanup(void);

enum Invoke_Status invoke_id_status(int id);
time_t invoke_id_time_sent(int id);
struct BACnet_NPDU *invoke_id_npdu();

void invoke_id_set_status(int id, enum Invoke_Status status);
void invoke_id_set_time_sent(int id, time_t time_sent);
void invoke_id_send_npdu(int id, struct BACnet_NPDU *npdu, uint8_t * apdu,
    int apdu_len);

#endif
