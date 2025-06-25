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
#include "os.h"
#include "bacnet_device.h"
#include "debug.h"
#include "main.h"
#include "ethernet.h"

// from html.c
extern void html_cleanup(void);

/* signal handler to trap Ctrl-C */
static void sig_int(int signo)
{
    debug_printf(5, "sig_int: entered\n");

    debug_printf(1, "sig_int: Shutting down...\n");

    debug_printf(2, "sig_int: Freeing memory...\n");
    device_cleanup();

    debug_printf(2, "sig_int: Closing 802.2 socket\n");
    ethernet_cleanup();

    if (mstp_sockfd >= 0) {     /* socket is still open */
        close(mstp_sockfd);
        debug_printf(2, "sig_int: Closed Loopback MSTP socket\n");
    }
    if (bip_sockfd >= 0) {      /* socket is still open */
        close(bip_sockfd);
        debug_printf(2, "sig_int: Closed BACnet/IP socket\n");
    }
    if (http_sockfd >= 0) {     /* close TCP server socket */
        close(http_sockfd);
        debug_printf(2, "sig_int: Closed TCP server socket\n");
    }

    html_cleanup();

    exit(0);
}

void signal_init(void)
{
    signal(SIGINT, sig_int);
    signal(SIGHUP, sig_int);
    signal(SIGTERM, sig_int);
}


/* end of signal_handler.c */
