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
#include "bacnet_struct.h"
#include "bacnet_const.h"
#include "version.h"

// common global configuration options
// the device instance that this will be (0-4194303)
int BACnet_Device_Instance = 2;
// APDU timeout (for retries) in seconds 
int BACnet_APDU_Timeout = 10;
// the BACnet Vendor ID that will be used (0 == ASHRAE)
// Note: ASHRAE maintains the list of Vendor Ids - see them for one.
int BACnet_Vendor_Identifier = 6;
// the ethernet interface that will be used for 802.2
char BACnet_Device_Interface[MAX_INTERFACE_CHARS] = "eth0";
// port for BACnet/IP (47808 = 0xBAC0 is the default BACnet/IP port)
int BACnet_UDP_Port = 0xBAC0;
// port to run MS/TP state machine from
//int BACnet_MSTP_Port = 0x4284;
// set to zero since it is not implemented by default
// and it will cause problems if data is found on that socket
int BACnet_MSTP_Port = 0;
// This flag enables the BACnet Ethernet 802.2 (enable=1, disable=0)
int BACnet_Ethernet_Enable = 1;

// prints a users guide with command line options listed.
void options_usage(void)
{
    printf(" -d###  BACnet device instance number (0-4194303)\n"
        " -e#    BACnet Ethernet (0=disable,1=enable)\n"
        " -iname BACnet Ethernet interface name (eth0, eth1, etc.)\n"
        " -m###  BACnet MS/TP port number (0-65534)\n"
        " -p###  BACnet/IP UDP port number (0=disabled,1-65534,0xBAC0)\n"
        " -t###  BACnet APDU timeout (seconds)\n"
        " -v###  BACnet Vendor Identifier (0-65534)\n"
        " -V     Returns the version information\n");

    return;
}

void options_default(void)
{
    printf("-d%d -e%d -i%s -m%d -p%d -t%d -v%d\n",
        BACnet_Device_Instance,
        BACnet_Ethernet_Enable,
        BACnet_Device_Interface,
        BACnet_MSTP_Port,
        BACnet_UDP_Port, BACnet_APDU_Timeout, BACnet_Vendor_Identifier);

    return;
}

// Takes each of the arguments passed by the main function
// and sets parameters if it matches one of the predefined args.
void options_interpret_arguments(int argc, char *argv[])
{
    int i = 0;                  /* used to index through arguments */
    char *p_arg = NULL;         /* points to current argument */
    long number = 0;            /* used for strtol */
    char *p_data = NULL;        /* points to data portion of argument */

    if (!argv)
        return;

    /* skip 1st one [0] - its the command line for the filename */
    for (i = 1; i < argc; i++) {
        p_arg = argv[i];
        if (p_arg[0] == '-') {
            p_data = p_arg + 2;
            switch (p_arg[1]) {
            case 'd':
                number = strtol(p_data, NULL, 0);
                if ((number >= 0L) && (number <= BACNET_MAX_ID))
                    BACnet_Device_Instance = number;
                else
                    printf("Invalid BACnet Device Instance number. "
                        "Using default.\n");
                break;
            case 'e':
                number = strtol(p_data, NULL, 0);
                if (number)
                    BACnet_Ethernet_Enable = 1;
                else
                    BACnet_Ethernet_Enable = 0;
                break;
            case 'i':
                if (p_data[0] != 0)
                    strncpy(BACnet_Device_Interface, p_data,
                        MAX_INTERFACE_CHARS);
                else
                    printf("Invalid BACnet Ethernet interface. "
                        "Using default.\n");
                break;
            case 'p':
                number = strtol(p_data, NULL, 0);
                if ((number >= 0L) && (number <= 0xFFFFL))
                    BACnet_UDP_Port = number;
                else
                    printf("Invalid BACnet UDP Port number. "
                        "Using default.\n");
                break;
            case 't':
                number = strtol(p_data, NULL, 0);
                BACnet_APDU_Timeout = number;
                break;
            case 'v':
                number = strtol(p_data, NULL, 0);
                if ((number >= 0L) && (number <= 0xFFFFL))
                    BACnet_Vendor_Identifier = number;
                else
                    printf("Invalid BACnet Vendor Identifier number. "
                        "Using default.\n");
                break;
            case 'V':
                (void) printf("%s\n", Program_Version);
                exit(0);
                break;
            case '-':
                if (strcmp(p_data, "help") == 0) {
                    options_usage();
                    options_default();
                    exit(1);
                }
                break;
            default:
                break;
            }                   /* end of arguments beginning with - */
        }                       /* dash arguments */
    }                           /* end of arg loop */
    return;
}
