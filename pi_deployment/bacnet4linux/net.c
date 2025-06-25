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
//
// All network functions are stored in here
//
#include "os.h"
#include "debug.h"
#include "net.h"
#include "bacnet_const.h"

void set_mac_address_string(char *address_string, size_t len,
    uint8_t * addr)
{
    if (len >= MAC_ADDRESS_STRING_LEN) {
        sprintf(address_string, "%02X:%02X:%02X:%02X:%02X:%02X",
            (unsigned) addr[0], (unsigned) addr[1], (unsigned) addr[2],
            (unsigned) addr[3], (unsigned) addr[4], (unsigned) addr[5]);
    }

}

char *hwaddrtoa(unsigned char *hwaddr)
{
    static char address_string[MAC_ADDRESS_STRING_LEN] = { "" };

    set_mac_address_string(address_string, sizeof(address_string), hwaddr);

    return address_string;
}

//
//Relevent network ioctl functions
//
//SIOCGIFADDR:       /* Get interface address */
//SIOCGIFBRDADDR:    /* Get the broadcast address */
//SIOCGIFNETMASK:    /* Get the netmask for the interface */

int get_local_ifr_ioctl(char *ifname, struct ifreq *ifr, int request)
{
    int fd;
    int rv;                     // return value

    strncpy(ifr->ifr_name, ifname, sizeof(ifr->ifr_name));
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd < 0)
        rv = fd;
    else
        rv = ioctl(fd, request, ifr);

    return rv;
}

int get_local_address_ioctl(char *ifname, struct in_addr *addr,
    int request)
{
    struct ifreq ifr = { {{0}} };
    struct sockaddr_in *tcpip_address;
    int rv;                     // return value

    rv = get_local_ifr_ioctl(ifname, &ifr, request);
    if (rv >= 0) {
        tcpip_address = (struct sockaddr_in *) &ifr.ifr_addr;
        memcpy(addr, &tcpip_address->sin_addr, sizeof(struct in_addr));
    }

    return rv;
}

int get_local_ip_address(char *ifname, struct in_addr *addr)
{
    return get_local_address_ioctl(ifname, addr, SIOCGIFADDR);
}

int get_local_ip_broadcast_address(char *ifname, struct in_addr *addr)
{
    return get_local_address_ioctl(ifname, addr, SIOCGIFBRDADDR);
}

int get_local_ip_address_mask(char *ifname, struct in_addr *mask)
{
    return get_local_address_ioctl(ifname, mask, SIOCGIFNETMASK);
}

/* function to open the TCP loopback socket for MSTP driver */
int open_mstp_socket(int port)
{
    struct sockaddr_in server_address;
    int server_sockfd = -1;
    int server_len;
    //Create an unnamed socket for the server
    //
    debug_printf(2, "NET: Opening loopback MSTP socket...\n");
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd < 0) {    /* 0 is TCP */
        error_printf("NET: Error opening loopback MSTP socket %s\n",
            strerror(errno));
        exit(-1);
    }
    /* Success */
    debug_printf(2,
        "NET: Loopback MSTP socket open on file descriptor %d\n",
        server_sockfd);
    //Name that socket
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    //which permanent socket should we use???
    //ideas include:
    //0xAAE1, 0xAAF1, etc.
    //8080 isn't bad either, since most proxys have that port open...
    server_address.sin_port = htons(9734);
    server_len = sizeof(server_address);
    bind(server_sockfd, (struct sockaddr *) &server_address, server_len);
    //Create a connection queue and wait for clients
    listen(server_sockfd, 5);   //open a listener, with a max of 5 connections/clients
    debug_printf(2,
        "NET: Loopback MSTP socket listening on local port %d\n",
        ntohs(server_address.sin_port));
    return server_sockfd;
}

/* function to open the UDP socket for BACnet/IP */
int open_bip_socket(char *ifname, int port)
{
    struct sockaddr_in addr;
    int sock_fd = -1;
    int sockopt;
    int status = 0;             // return from socket lib calls
//    struct ifreq ifr;

    debug_printf(2, "NET: Opening UDP socket...\n");

    /* Attempt to create a new socket */
    sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd < 0) {
        error_printf("NET: Error opening UDP socket %s\n",
            strerror(errno));
        exit(EXIT_FAILURE);
    }
    // Allow us to use the same socket for sending and receiving
    // This makes sure that the src port is correct when sending
    sockopt = 1;
    status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR,
        &sockopt, sizeof(sockopt));
    if (status != 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    sockopt = 1;
    status = setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST,
        &sockopt, sizeof(sockopt));
    if (status != 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    /* Success */
    debug_printf(2, "NET: UDP socket open on file descriptor %d\n",
        sock_fd);

    // get the local ip address so that we can bind in order to listen
    // for broadcast and directed packets
//    if (get_local_ifr_ioctl(ifname, &ifr, SIOCGIFADDR) < 0) {
//        error_printf("NET: Error getting local ip address: %s\n",
//            strerror(errno));
//        exit(EXIT_FAILURE);
//    }

    /* We need to get the interface address to bind to */
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);        /* set port to listen on */
    memset(&(addr.sin_zero), '\0', 8);
    /* Otherwise, attempt to bind it to an adress */
    status = bind(sock_fd,
        (const struct sockaddr *) &addr, sizeof(struct sockaddr));
    if (status != 0) {
        error_printf("NET: Error binding UDP socket %s\n",
            strerror(errno));
        exit(EXIT_FAILURE);
    }
    // bind so that we can get broadcasts
//    if (setsockopt
//        (sock_fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) != 0)
//    {
//        perror("setsockopt");
//        exit(EXIT_FAILURE);
//    }

    /* Success */
    debug_printf(2, "NET: UDP socket listening on local port %d\n",
        ntohs(addr.sin_port));

    return sock_fd;
}

/* function to open TCP socket for web-interface */
int open_tcp_socket(int port)
{
    int server_len = 0;
    struct sockaddr_in server_address = { 0 };
    int flags = 0;
    int retry = 0;
    int sock_fd = -1;

    debug_printf(2, "NET: Opening TCP server socket...\n");
    /* open TCP socket */
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        error_printf("NET: Unable to open TCP socket: %s.\n",
            strerror(errno));
        exit(-1);
    }
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); /* receive from anybody */
    server_address.sin_port = htons(port);      /* http server port */
    server_len = sizeof(server_address);
    debug_printf(2, "NET: TCP server socket open on file descriptor %d\n",
        sock_fd);
    /* this is done the way it is to make restarts of the program easier */
    /* in Linux, TCP sockets have a 2 minute wait period before closing */
    while (bind(sock_fd, (struct sockaddr *) &server_address, server_len)
        < 0) {
        error_printf
            ("NET: Error binding TCP server socket: %s.  Retrying...\n",
            strerror(errno));
        sleep(5);               /* wait a second to see if it clears */
        retry++;
        /* more than 4 minutes has elapsed, there must be something wrong */
        if (retry > ((4 * 60) / 5))
            exit(-1);
    }
    if (listen(sock_fd, 5) < 0) {
        error_printf("NET: Error making TCP server socket listen: %s.\n",
            strerror(errno));
        exit(-1);
    }
    flags = fcntl(sock_fd, F_GETFL, 0);
    fcntl(sock_fd, F_SETFL, O_NONBLOCK | flags);
    /* TCP socket is ready to go */
    debug_printf(2, "NET: TCP server socket listening on local port %d\n",
        port);

    return sock_fd;
}

int open_tcp_client(int server_socket)
{
    int client_socket = 0;
    int client_len = 0;
    struct sockaddr_in client_address = { 0 };
    int retry = 0;              // counts number of retries
    int wait = 5;               // seconds

    debug_printf(2, "NET: Opening TCP client socket...\n");
    do {
        client_socket = accept(server_socket,
            (struct sockaddr *) &client_address, &client_len);
        error_printf("NET: Error in accept(): %s.  Retry #%d\n",
            strerror(errno), retry + 1);
        sleep(wait);            /* wait a second to see if it clears */
        retry++;
        /* more than 4 minutes has elapsed, there must be something wrong */
        if (retry > ((4 * 60) / wait))
            exit(-1);
    } while (client_socket == -1);

    debug_printf(2, "NET: TCP client socket accepting data.\n");

    return client_socket;
}

/* function to find the local ethernet MAC address */
int get_local_hwaddr(const char *ifname, unsigned char *mac)
{
    struct ifreq ifr;
    int fd;
    int rv;                     // return value - error value from df or ioctl call

    /* determine the local MAC address */
    strcpy(ifr.ifr_name, ifname);
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd < 0)
        rv = fd;
    else {
        rv = ioctl(fd, SIOCGIFHWADDR, &ifr);
        if (rv >= 0)            /* worked okay */
            memcpy(mac, ifr.ifr_hwaddr.sa_data, IFHWADDRLEN);
    }

    return rv;
}
