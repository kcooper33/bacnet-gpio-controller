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
 Modified by Coleman Brumley <cbrumley@users.sourceforge.net> 18 Nov 2003
 -------------------------------------------
####COPYRIGHTEND####*/

#include "os.h"
#include "bacnet_struct.h"
#include "debug.h"
#include "main.h"
#include "options.h"

// for some reason, with dynamic memory, we get scrambled output
// Use alloc memory...
// #define SEND_BIP_DYN_MEM
int send_bip(struct BACnet_NPDU *npdu, uint8_t * apdu, int apdu_len)
{
    struct sockaddr_in bip_dest;
#ifdef SEND_BIP_DYN_MEM
    uint8_t *mtu = NULL;
#else
    static uint8_t mtu[DEFAULT_MTU] = { 0 };    // FIXME: not thread safe
#endif
    int mtu_len = 0;
    int i = 0;
    int bytes_sent = 0;
    int status = 1;             // return value

    /* Make sure the socket is open */
    if (bip_sockfd < 0) {
        debug_printf(4, "send_bip: IP socket is invalid!\n");
        return 0;
    }
    //npdu->dest.ip[IPLEN] = (char)0; 
    debug_printf(2, "send_bip: dest.ip=%s:%4X\n", inet_ntoa(npdu->dest.ip),
        BACnet_UDP_Port);

    /* load the destination IP address into the structure */
    if (npdu->dest.ip.s_addr > 0) {
        bip_dest.sin_family = AF_INET;
        bip_dest.sin_addr.s_addr = npdu->dest.ip.s_addr;
        bip_dest.sin_port = htons(BACnet_UDP_Port);     /* port to send to */
    } else {
        error_printf
            ("send_bip: Panic!: No destination IP address given!\n");
        return 0;
    }

#ifdef SEND_BIP_DYN_MEM
    mtu = calloc(1, DEFAULT_MTU);
    if (!mtu) {
        error_printf("unable to allocate memory: %s\n", strerror(errno));
        return 0;
    }
#endif
    mtu[0] = 0x81;
    if (npdu->local_broadcast)
        mtu[1] = 0x0B;          // Original-Broadcast-NPDU 
    else
        mtu[1] = 0x0A;          /* Original-Unicast-NPDU */
    mtu[2] = 0x00;              /* upper length byte */
    mtu[3] = 0x00;              // lower length byte - filled later...
    mtu_len = 4;
    mtu[mtu_len] = npdu->version;       /* NPDU... Version */
    mtu_len++;
    mtu[mtu_len] = npdu->control_byte;  /* Control Byte */
    mtu_len++;
    //FIXME
    //Correctly fill in the buffer
    if (npdu->dest_present) {
        mtu[mtu_len] = (int) (npdu->dest.net / 256);    /* upper 8 bits */
        mtu[mtu_len + 1] = npdu->dest.net - mtu[mtu_len] * 256; /* lower 8 bits */
        mtu[mtu_len + 2] = npdu->dest.len;
        mtu_len += 3;
        if (npdu->dest.len == 6) {      /* dest.adr is present for ethernet */
            for (i = 0; i < 6; i++) {   /* 6 chunks of mac */
                mtu[mtu_len] = npdu->dest.adr[i];
                mtu_len++;
            }
        } else if (npdu->dest.len == 1) {       /* dest.adr is present for arcnet, ms/tp */
            mtu[mtu_len] = npdu->dest.adr[0];
            mtu_len++;
        }
    }
    if (npdu->src_present) {
        /* upper 8 bits */
        mtu[mtu_len] = (int) (npdu->src.net / 256);
        /* lower 8 bits */
        mtu[mtu_len + 1] = npdu->src.net - mtu[mtu_len] * 256;
        mtu[mtu_len + 2] = npdu->src.len;
        mtu_len += 3;
        /* dest.adr is present for ethernet */
        if (npdu->src.len == 6) {
            /* 6 chunks of mac */
            for (i = 0; i < 6; i++) {
                mtu[mtu_len] = npdu->src.adr[i];
                mtu_len++;
            }
        }
    }
    /* hop count */
    if (npdu->dest_present) {
        mtu[mtu_len] = 0xFF;
        mtu_len++;
    }
    /* there is a network message, not an APDU */
    if (npdu->network_message) {
        mtu[mtu_len] = npdu->message_type;
        mtu_len++;
    }
    /* at this point only the APDU is remaining */
    memcpy(&mtu[mtu_len], apdu, apdu_len);
    mtu_len += apdu_len;
    // put the total length here
    mtu[2] = (int) (mtu_len / 256);     /* upper 8 bits */
    mtu[3] = mtu_len - mtu[2] * 256;    /* lower 8 bits */
    debug_printf(4, "send_bip: sending to %s\n",
        inet_ntoa(bip_dest.sin_addr));
    debug_dump_data(4, mtu, mtu_len);
    bytes_sent = sendto(bip_sockfd,
        &mtu, mtu_len,
        0, (struct sockaddr *) &bip_dest, sizeof(struct sockaddr));
    //bytes_sent = sendto( bip_sockfd, &buf, 6 /* buf length */, 0, &bip_dest, sizeof(bip_dest));
    if (bytes_sent < 0) {
        perror("BIP");
        error_printf
            ("send_bip: An error occurred sending %d bytes via BACnet/IP",
            bytes_sent);
        status = 0;
    }
#ifdef SEND_BIP_DYN_MEM
    free(mtu);
#endif

    return status;
}

/* end of send_bip.c */
