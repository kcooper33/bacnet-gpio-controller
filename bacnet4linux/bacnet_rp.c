/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2004 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#include "os.h"
#include "options.h"

static void usage(char *argv0)
{
    (void) fprintf(stderr, "usage:  %s "
        "[options] <device instance> <object> <instance> <property> [index]"
        " \n", argv0);
    options_usage();
    options_default();
    exit(1);
}

static int interpret_options(int argc, char *argv[])
{
    int argn = 1;               // counts the arguments 

    while ((argn < argc) && (argv[argn][0] == '-')) {
        if (strcmp(argv[argn], "--help") == 0) {
            usage(argv[0]);
        }
    argn++}
    options_interpret(argc, argv);

    return (argn);
}

/* argc - number of arguments entered at command line */
/* argv - arguments in string form. */
int main(int argc, char *argv[])
{
    int argn = 0;               // counts the arguments 
    int fixed_args = 0;
    long device_instance = -1;
    long object_type = -1;
    long object_instance = -1;
    long object_property = -1;
    long object_property_index = -1;

    // interpret the system options that start with a dash
    argn = interpret_options(argc, argv);
    // now interpret the fixed arguments
    // <device instance> <object> <instance> <property> [index]
    while (argn < argc) {
        switch (fixed_args) {
            // <device instance>
        case 0:
            device_instance = strtol(argv[argn], NULL, 0);
            break;
            // <object> 
        case 1:
            if (isalpha(argv[argn][0]))
                object_type = text_to_enum_object(argv[argn]);
            else
                object_type = strtol(argv[argn], NULL, 0);
            break;
            // <instance>
        case 2:
            object_instance = strtol(argv[argn], NULL, 0);
            break;
            // <property>
        case 3:
            if (isalpha(argv[argn][0]))
                object_property = text_to_enum_property(argv[argn]);
            else
                object_property = strtol(argv[argn], NULL, 0);
            break;
            // [index]
        case 4:
            object_property_index = strtol(argv[argn], NULL, 0);
            break;
        default:
            break;
        }
        fixed_args++;
        argn++;
    }
    if ((device_instance == -1) ||
        (object_type == -1) ||
        (object_instance == -1) || (object_property = -1))
        usage(argv[0]);
    invoke_id_init();
    device_init();
    get_local_ip_address(BACnet_Device_Interface,
        &BACnet_Device_IP_Address);
    get_local_ip_broadcast_address(BACnet_Device_Interface,
        &Local_IP_Broadcast_Address);
    // init the sockets
    /* Attempt to open the 802.2 socket */
    if (BACnet_Ethernet_Enable)
        ethernet_init(BACnet_Device_Interface);
    /* Attempt to open the BACnet/IP socket */
    if (BACnet_UDP_Port)
        bip_sockfd =
            open_bip_socket(BACnet_Device_Interface, BACnet_UDP_Port);
    // bind to the device id
    send_whois(device_instance);
    // wait for the response (or apdu timeout)
    // send the read request
    // wait for the response (or apdu timeout)
    // print the response or error
    // exit
    return 0;
}
