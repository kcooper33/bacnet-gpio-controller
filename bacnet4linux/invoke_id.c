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
 Modified by Coleman Brumley <cbrumley@users.sourceforge.net> 19 Nov 2003
 -------------------------------------------
####COPYRIGHTEND####*/
//
// Invoke ID functions are in here
//
#include "os.h"
#include "debug.h"
#include "bacnet_const.h"
#include "bacnet_struct.h"
#include "bacnet_api.h"
#include "ethernet.h"
#include "invoke_id.h"
#include "main.h"
#include "net.h"

extern int BACnet_APDU_Timeout;

static struct Invoke_Status_Struct Invoke_Id[MAXINVOKEIDS + 1];

/* current status of this invoke ID */
enum Invoke_Status invoke_id_status(int id)
{
    enum Invoke_Status status = INVOKE_STATUS_NOACTIVITY;

    if (id <= MAXINVOKEIDS)
        status = Invoke_Id[id].status;

    return status;
}
void invoke_id_set_status(int id, enum Invoke_Status status)
{
    if (id <= MAXINVOKEIDS)
        Invoke_Id[id].status = status;
}

/* time that the request was sent */
time_t invoke_id_time_sent(int id)
{
    time_t time_sent = 0;

    if (id <= MAXINVOKEIDS)
        time_sent = Invoke_Id[id].time_sent;

    return time_sent;
}
void invoke_id_set_time_sent(int id, time_t time_sent)
{
    if (id <= MAXINVOKEIDS)
        Invoke_Id[id].time_sent = time_sent;
}

/* NPDU that was sent */
struct BACnet_NPDU *invoke_id_npdu(int id)
{
    struct BACnet_NPDU *npdu = NULL;

    if (id <= MAXINVOKEIDS)
        npdu = &Invoke_Id[id].npdu;

    return npdu;
}

/* returns the next available Invoke ID for use */
/* but does not change the status of that invoke ID */
int invoke_id(void)
{
    int i;
    static int lastID = 0;      /* the ID that was used for the last request */

    debug_printf(5, "invoke-id: Entered 'get_invoke_id'\n");

    for (i = (lastID + 1); i <= MAXINVOKEIDS; i++) {    /* from 'lastID' to end */
        if (Invoke_Id[i].status == INVOKE_STATUS_NOACTIVITY) {  /* this Invoke ID is not in use */
            lastID = i;
            return i;           /* return it */
        }
    }
    for (i = 0; i < lastID; i++) {      /* from beginning to 'lastID' */
        /* this Invoke ID is not in use */
        if (Invoke_Id[i].status == INVOKE_STATUS_NOACTIVITY) {
            lastID = i;
            return i;           /* return it */
        }
    }
    return -1;
}

/* checks that all Invoke IDs are available for use */
/* this is used to slow down network requests to a sub-Linux pace :) */
int invoke_id_in_use(void)
{
    int i;
    int in_use_count = 0;

    debug_printf(5, "invoke-id: Entered 'verify_invoke_ids'\n");

    /* entire range */
    for (i = 0; i <= MAXINVOKEIDS; i++) {
        /* this Invoke ID is in use */
        if (Invoke_Id[i].status != INVOKE_STATUS_NOACTIVITY)
            in_use_count++;
    }

    return in_use_count;
}

/* function to reset one invoke ID to a sane state */
void invoke_id_reset(int invokeID)
{
    debug_printf(9, "invoke-id: Entered 'reset_invoke_id'\n");

    if ((invokeID >= 0) && (invokeID <= MAXINVOKEIDS)) {        /* valid ID */
        Invoke_Id[invokeID].status = INVOKE_STATUS_NOACTIVITY;  /* default unused state */
        Invoke_Id[invokeID].time_sent = 0;      /* the epoch */
        memset(&Invoke_Id[invokeID].npdu, '\0',
            sizeof(struct BACnet_NPDU));
    }
}

/* function to clean up all outstanding invoke IDs */
/* this is continually called from main() */
void invoke_id_cleanup(void)
{
    int i;
    time_t t;
    static time_t last_cleanup = 0;
    struct BACnet_NPDU *npdu;
    uint8_t *apdu;
    int apdu_len;

    debug_printf(9, "invoke-id: Entered 'cleanup_invoke_ids'\n");

    t = time(NULL);             /* get current time */
    if (t != last_cleanup) {    /* ensures this doesn't run faster than once a second */
        last_cleanup = t;       /* right now */
        for (i = 0; i <= MAXINVOKEIDS; i++) {   /* loop thru invoke ID structure */
            if ((Invoke_Id[i].status == INVOKE_STATUS_SENT) &&
                ((t - Invoke_Id[i].time_sent) >= BACnet_APDU_Timeout)) {
                /* this ID has been in "sent" for more than the APDU timeout */
                /* should retry this send */

                debug_printf(1, "invoke-id: #%d timed out. Retrying...\n",
                    i);
                npdu = &Invoke_Id[i].npdu;
                apdu = &Invoke_Id[i].apdu[0];
                apdu_len = Invoke_Id[i].apdu_len;

                /* send 802.2 packet */
                if (memcmp(npdu->dest.mac, Ethernet_Empty_MAC,
                        sizeof(npdu->dest.mac)) != 0) {
                    debug_printf(3,
                        "invoke-id: sending b/eth packet to %s\n",
                        hwaddrtoa(npdu->dest.mac));
                    ethernet_send(npdu, apdu, apdu_len);
                }
                if (npdu->dest.ip.s_addr > 0) {
                    debug_printf(3,
                        "invoke-id: sending bip packet to %s\n",
                        inet_ntoa(npdu->dest.ip));
                    /* send b/ip packet */
                    send_bip(npdu, apdu, apdu_len);
                }
                Invoke_Id[i].time_sent = t;
                Invoke_Id[i].status = INVOKE_STATUS_RESENT;
            } else if ((Invoke_Id[i].status == INVOKE_STATUS_RESENT) &&
                ((t - Invoke_Id[i].time_sent) >= BACnet_APDU_Timeout)) {
                error_printf
                    ("invoke-id: Request with Invoke ID %d has failed.\n",
                    i);
                invoke_id_reset(i);     /* give up */
            }
        }
    }
}

void invoke_id_send_npdu(int id, struct BACnet_NPDU *npdu, uint8_t * apdu,
    int apdu_len)
{
    time_t time_sent;

    if (npdu && (id <= MAXINVOKEIDS)) {
        memcpy(&Invoke_Id[id].npdu, npdu, sizeof(Invoke_Id[id].npdu));
        memcpy(&Invoke_Id[id].apdu, apdu, sizeof(Invoke_Id[id].apdu));
        Invoke_Id[id].apdu_len = apdu_len;
        Invoke_Id[id].status = INVOKE_STATUS_SENT;
        time_sent = time(NULL);
        Invoke_Id[id].time_sent = time_sent;
    }

    return;
}


void invoke_id_init(void)
{
    int i;                      // counter 
    /* initialize Invoke ID structure */
    for (i = 0; i < MAXINVOKEIDS; i++)
        invoke_id_reset(i);     /* one invoke ID */
}

/* end of invoke_id.c */
