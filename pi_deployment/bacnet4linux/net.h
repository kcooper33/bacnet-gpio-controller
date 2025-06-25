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
#ifndef NET_H
#define NET_H

//Macros to get the destination address from a packet
#if defined IP_RECVDSTADDR
#define DSTADDR_SOCKOPT IP_RECVDSTADDR
#define DSTADDR_DATASIZE (CMSG_SPACE(sizeof(struct in_addr)))
#define dstaddr(x) (CMSG_DATA(x))
#elif defined IP_PKTINFO
#define DSTADDR_SOCKOPT IP_PKTINFO
#define DSTADDR_DATASIZE (CMSG_SPACE(sizeof(struct in_pktinfo)))
#define dstaddr(x) (&(((struct in_pktinfo *)(CMSG_DATA(x)))->ipi_addr))
#else
# error "can't determine socket option"
#endif

union control_data {
    struct cmsghdr cmsg;
    u_char data[DSTADDR_DATASIZE];
};

#define MAC_ADDRESS_STRING_LEN 18       // FF:FF:FF:FF:FF:FF\0
void set_mac_address_string(char *address_string, size_t len,
    unsigned char *buf);
/* network socket functions */
int open_mstp_socket(int port);
int open_bip_socket(char *ifname, int port);
int open_tcp_socket(int port);
int open_tcp_client(int server_socket);

int get_local_ip_address(char *ifname, struct in_addr *addr);
int get_local_ip_broadcast_address(char *ifname, struct in_addr *addr);
int get_local_ip_address_mask(char *ifname, struct in_addr *mask);
int get_local_hwaddr(const char *ifname, unsigned char *mac);

void get_dest_address_string(struct msghdr msgh, char *lpstrAddr,
    size_t len);
char *hwaddrtoa(unsigned char *hwaddr);

#endif
