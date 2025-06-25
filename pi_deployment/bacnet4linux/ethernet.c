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

 Modified by Steve Karg <skarg@users.sourceforge.net> 22 Dec 2004
 -------------------------------------------
####COPYRIGHTEND####*/
//
// Handles network packet sending 
//
#include "os.h"
#include "bacnet_struct.h"
#include "bacnet_api.h"
#include "bacnet_const.h"
#include "bacnet_device.h"
#include "debug.h"
#include "main.h"
#include "net.h"
#include "pdu.h"

// commonly used comparison address for ethernet
uint8_t Ethernet_Broadcast[MAX_MAC_LEN] =
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
// commonly used empty address for ethernet quick compare
uint8_t Ethernet_Empty_MAC[MAX_MAC_LEN] = { 0, 0, 0, 0, 0, 0 };

// my local device data - MAC address
uint8_t Ethernet_MAC_Address[MAX_MAC_LEN] = { 0 };

// Use alloc memory...
// #define SEND_PACKET_DYN_MEM
static int eth802_sockfd = -1;  /* 802.2 file handle */
static struct sockaddr eth_addr = { 0 };        // used for binding 802.2

bool ethernet_valid(void)
{
    return (eth802_sockfd >= 0);
}

int ethernet_socket(void)
{
    return eth802_sockfd;
}

bool ethernet_cleanup(void)
{
    if (ethernet_valid())
        close(eth802_sockfd);
    eth802_sockfd = -1;

    return true;
}

/* opens an 802.2 socket to receive and send packets */
int ethernet_bind(struct sockaddr *eth_addr, char *interface_name)
{
    int sock_fd = -1;           /* Local fd var for the fd */
    int uid = 0;

    debug_printf(2, "ethernet: Opening 802.2 socket...\n");

    uid = getuid();
    if (uid != 0) {             /* not being run as root */
        error_printf
            ("ethernet: Not being run as root, so an 802.2 socket is impossible.\n");
        return sock_fd;
    }

    /* Attempt to open the socket for 802.2 ethernet frames */
    // obsolete, and should not be used in new programs...
    //if ((sock_fd = socket(AF_INET, SOCK_PACKET, htons(ETH_P_802_2))) < 0)
    if ((sock_fd = socket(PF_INET, SOCK_PACKET, htons(ETH_P_802_2))) < 0) {
        /* Error occured */
        error_printf("ethernet: Error opening socket : %s\n",
            strerror(errno));
        exit(-1);
    }

    debug_printf(2, "ethernet: 802.2 socket open on file descriptor %d\n",
        sock_fd);

    /* Bind the socket to an address */
    eth_addr->sa_family = AF_UNIX;

    /* Clear the memory before copying */
    memset(eth_addr->sa_data, '\0', sizeof(eth_addr->sa_data));

    /* Strcpy the interface name into the address */
    strncpy(eth_addr->sa_data, interface_name, sizeof(eth_addr->sa_data));

    /* Attempt to bind the socket to the interface */
    if (bind(sock_fd, eth_addr, sizeof(struct sockaddr)) != 0) {
        /* Bind problem, close socket and return */
        error_printf("ethernet: Unable to bind 802.2 socket : %s\n",
            strerror(errno));
        error_printf
            ("You might need to add the following to modules.conf\n"
            "(or in modutils/alias on Debian with update-modules):\n"
            "alias net-pf-17 af_packet\n" "Then follow it by:\n"
            "# modprobe af_packet\n");
        /* Close the socket */
        close(sock_fd);
        exit(-1);
    }

    debug_printf(2, "ethernet: 802.2 socket bound to %s\n",
        interface_name);

    return sock_fd;
}

bool ethernet_init(char *interface_name)
{
    get_local_hwaddr(interface_name, Ethernet_MAC_Address);
    debug_printf(2, "ethernet: Local MAC=%s\n",
        hwaddrtoa(Ethernet_MAC_Address));

    eth802_sockfd = ethernet_bind(&eth_addr, interface_name);

    return ethernet_valid();
}

/* function to send a packet out the 802.2 socket */
/* returns 1 on success, 0 on failure */
int ethernet_send(struct BACnet_NPDU *npdu, uint8_t * apdu, int apdu_len)
{
    int status = 0;
    int bytes = 0;
#ifdef SEND_PACKET_DYN_MEM
    uint8_t *mtu = NULL;
#else
    uint8_t mtu[DEFAULT_MTU] = { 0 };
#endif
    int mtu_len = 0;
    int packet_len = 0;
    int i = 0;

    debug_printf(5, "ethernet: send\n");
#ifdef SEND_PACKET_DYN_MEM
    mtu = calloc(1, DEFAULT_MTU);
    if (!mtu) {
        error_printf("unable to allocate memory: %s\n", strerror(errno));
        return 0;
    }
#endif
    // don't waste time if the socket is not valid
    if (eth802_sockfd < 0) {
        debug_printf(4, "ethernet: 802.2 socket is invalid!\n");
        goto free_packet;
    }

    /* encode destination ethernet mac if destination mac exists */
    if (memcmp(npdu->dest.mac, Ethernet_Empty_MAC,
            sizeof(npdu->dest.mac)) != 0) {
        for (i = 0; i < 6; i++) {
            mtu[i] = npdu->dest.mac[i];
        }
    } else {
        error_printf("Panic!: No destination MAC address given!\n");
        goto free_packet;
    }

    /* encode source ethernet mac if source mac exists */
    if (memcmp(npdu->src.mac, Ethernet_Empty_MAC,
            sizeof(npdu->dest.mac)) != 0) {
        for (i = 0; i < 6; i++) {
            mtu[i + 6] = npdu->src.mac[i];
        }
    } else {
        error_printf("Panic!: No source MAC address given!\n");
        goto free_packet;
    }

    mtu[14] = 0x82;             /* DSAP for BACnet */
    mtu[15] = 0x82;             /* SSAP for BACnet */
    mtu[16] = 0x03;             /* Control byte in header */
    mtu[17] = 0x01;             /* BACnet protocol version 1 */

    mtu[18] = npdu->control_byte;
    mtu_len = 19;               /* update buffer position */

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
        mtu[mtu_len] = (int) (npdu->src.net / 256);     /* upper 8 bits */
        mtu[mtu_len + 1] = npdu->src.net - mtu[mtu_len] * 256;  /* lower 8 bits */
        mtu[mtu_len + 2] = npdu->src.len;
        mtu_len += 3;
        if (npdu->src.len == 6) {       /* dest.adr is present for ethernet */
            for (i = 0; i < 6; i++) {   /* 6 chunks of mac */
                mtu[mtu_len + i] = npdu->src.adr[i];
                mtu_len++;
            }
        }
    }

    if (npdu->dest_present) {
        mtu[mtu_len] = 0xFF;    /* hop count */
        mtu_len++;
    }

    if (npdu->network_message) {        /* there is a network message, not an APDU */
        mtu[mtu_len] = npdu->message_type;
        mtu_len++;
    }

    /* at this point only the APDU is remaining */
    memcpy(&mtu[mtu_len], apdu, apdu_len);
    mtu_len += apdu_len;

    packet_len = mtu_len - 14;  /* packet length excluding the header */
    mtu[12] = (int) (packet_len / 256); /* upper 8 bits */
    mtu[13] = packet_len - mtu[12] * 256;       /* lower 8 bits */

    /* packet is now ready to go */

//#ifdef OUTPUTPACKETS
    //display_packet(npdu);
//#endif

    /* quick sanity check */
    if (mtu_len > DEFAULT_MTU) {        /* the maximum number of bytes in one shot */
        error_printf
            ("Attempted (and failed) to send a packet larger than %d bytes.\n",
            DEFAULT_MTU);
        goto free_packet;
    }
    if (mtu_len < 17) {         /* the minimum number of bytes in one shot */
        error_printf
            ("Attempted (and failed) to send a packet smaller than 17 bytes.\n");
        goto free_packet;
    }

    debug_printf(4, "send_packet: sending to %s\n", hwaddrtoa(&mtu[0]));
    debug_dump_data(4, mtu, mtu_len);
    /* Send the packet */
    bytes =
        sendto(eth802_sockfd, &mtu, mtu_len, 0,
        (struct sockaddr *) &eth_addr, sizeof(struct sockaddr));

    /* Now, make sure we sent correctly */
    if (bytes < 0) {            /* Error has occurred */
        error_printf("Error sending packet: %s\n", strerror(errno));
        goto free_packet;
    }
    // got this far - must be good!
    status = 1;

  free_packet:
#ifdef SEND_PACKET_DYN_MEM
    free(mtu);
#endif

    return status;
}

/* receives an 802.2 framed packet */
int ethernet_receive(int eth802_sockfd)
{
    int received_bytes;
    int status = 0;
    int pdu_len = 0;
    uint8_t *buf = NULL;
    struct BACnet_NPDU *npdu = NULL;

    /* Make sure the socket is open */
    if (eth802_sockfd <= 0)
        return 0;

    buf = calloc(1, DEFAULT_MTU);
    npdu = npdu_alloc();
    if (!buf || !npdu) {
        error_printf("unable to allocate memory: %s\n", strerror(errno));
        if (buf)
            free(buf);
        if (npdu)
            npdu_free(npdu);
        return 0;
    }

    /* Attempt a read */
    received_bytes = read(eth802_sockfd, &buf[0], DEFAULT_MTU);

    /* See if there is a problem */
    if (received_bytes < 0) {
        error_printf("802.2: Read error in receiving packet: %s\n",
            strerror(errno));
        goto free_packet;
    } else
        debug_printf(4, "802.2: Bytes read: %d\n", received_bytes);

    /* the signature of an 802.2 BACnet packet */
    if ((buf[14] != 0x82) && (buf[15] != 0x82)) {
        debug_printf(4, "802.2: Non-BACnet packet\n");
        goto free_packet;
    }
    debug_printf(2, "802.2: A %d byte BACnet packet has been received.\n",
        received_bytes);
    /* determine destination and source ethernet MAC */
    //These are the DA and SA packet int the MPDU of the 8802-3 packet
    memmove(npdu->dest.mac, &buf[0], sizeof(npdu->dest.mac));
    memmove(npdu->src.mac, &buf[6], sizeof(npdu->src.mac));
    debug_printf(3, "802.2: Dest=%s\n", hwaddrtoa(npdu->dest.mac));
    debug_printf(3, "802.2: Src=%s\n", hwaddrtoa(npdu->src.mac));

    /* added in case the Ethernet card is in promiscious mode */
    if ((memcmp(npdu->dest.mac, Ethernet_MAC_Address,
                sizeof(npdu->dest.mac)) != 0)
        && (memcmp(npdu->dest.mac, Ethernet_Broadcast,
                sizeof(npdu->dest.mac)) != 0)) {
        debug_printf(2, "802.2: This packet isn't for us\n");
        goto free_packet;
    }

    pdu_len = buf[12] * 256 + buf[13];
    npdu->pdu_len = pdu_len - 3 /* DSAP, SSAP, LLC Control */ ;
    npdu->pdu = &buf[17];
    /* hand off buf for decoding into NPDU */
    receive_npdu(npdu);
    /* a BACnet packet was successfully read */
    status = 1;
  free_packet:
    npdu_free(npdu);
    free(buf);

    return status;
}
