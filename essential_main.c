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
//
// main executable portion of this program. 
//
#include "os.h"
#include "bacnet_struct.h"
#include "bacnet_enum.h"
#include "bacnet_const.h"
#include "bacnet_api.h"
#include "bacnet_device.h"
#include "ethernet.h"
#include "invoke_id.h"
#include "net.h"
#include "debug.h"
#include "options.h"
#include "version.h"
#include "signal_handler.h"
#include "gpio_objects.h"

/* globals */

/* TCP socket file handles */
int http_sockfd = -1;
int bip_sockfd = -1;            /* BACnet/IP file handle */
int mstp_sockfd = -1;           /* MS/TP loopback file handle */

// global configuration options
// use COV for getting the Preset_Value from each object
int BACnet_COV_Support = 1;
// COV Lifetime indicates how often we renew our subscription (seconds)
int BACnet_COV_Lifetime = (5 * 60);
// number of concurrent queries (invoke ids)
int BACnet_Invoke_Ids = 1;
// port to run HTTP server on (80 is the normal WWW port)
int BACnet_HTTP_Port = 8000;
// A time master sends a global (or local) time sync to others     
// This value will send the time sync every x seconds when     
// x seconds is evenly divisible. If x seconds is zero, this
// is disabled     
int BACnet_Time_Sync_Seconds = 0;
// initial polling delay is how long to wait to begin 
// querying new devices (in seconds, 0=disable query)
int BACnet_Initial_Query_Delay = 5;
// my local device data - MAC address
struct in_addr BACnet_Device_IP_Address = { 0 };

// stores the local IP broadcast address which varies depending on subnet
struct in_addr Local_IP_Broadcast_Address = { 0 };

// filename used to read initialization of device database
char *readFile = NULL;
// filename used to save device database
char *writeFile = NULL;

// linked list of devices to accept I-Am's from
struct deviceRange {
    int min;
    int max;
    struct deviceRange *next;
};

struct deviceRange *deviceRangeList = NULL;
static struct deviceRange *deviceRangeTemp;

// prints a users guide with command line options listed.
static void Usage(char *program_name)
{
    printf("BACnet4Linux - %s\n"
        "An Open-Source BACnet Protocol stack for Linux.\n"
        "Copyright (C) 2003 Greg Holloway\n"
        "BACnet4Linux comes with ABSOLUTELY NO WARRANTY.\n"
        "This is free software, and you are welcome\n"
        "to redistribute it under certain conditions.\n"
        "\n" "Usage:\n" "%s [options]\n", Program_Version, program_name);
    // main options
    printf(" -c#    BACnet COV support (0=disable,1=enable)\n"
        " -C###  BACnet COV lifetime (seconds)\n"
        " -D#    debug level, larger is more verbose (0-9)\n"
        " -h###  HTTP server port (0-65534)\n"
        " -I###  Number of concurrent queries (invoke ids)\n"
        " -q###  Initial query delay (seconds, 0=disable query)\n"
        " -rfilename Initialize device database from XML 'filename'\n"
        " -s###  BACnet Sync Time Periodic (seconds,0=disabled)\n"
        " -wfilename Write device database to XML 'filename'\n"
        " -x###-###  eXclude devices except range ### to ### (multiple -x's OK)\n");
    options_usage();
    printf("default settings:\n"
        "-c%d -C%d -D%d -h%d -I%d -q%d -s%d\n",
        BACnet_COV_Support,
        BACnet_COV_Lifetime,
        debug_get_level(),
        BACnet_HTTP_Port,
        BACnet_Invoke_Ids,
        BACnet_Initial_Query_Delay, BACnet_Time_Sync_Seconds);
    options_default();

    return;
}

// Takes each of the arguments passed by the main function
// and sets parameters if it matches one of the predefined args.
static void Interpret_Arguments(int argc, char *argv[])
{
    int i = 0;                  /* used to index through arguments */
    char *p_arg = NULL;         /* points to current argument */
    long number = 0;            /* used for strtol */
    char *p_data = NULL;        /* points to data portion of argument */
    int deviceRangeMin, deviceRangeMax; /* for assembling list of device ID ranges */

    if (!argv)
        return;

    /* skip 1st one [0] - its the command line for the filename */
    for (i = 1; i < argc; i++) {
        p_arg = argv[i];
        if (p_arg[0] == '-') {
            p_data = p_arg + 2;
            switch (p_arg[1]) {
            case 'c':
                number = strtol(p_data, NULL, 0);
                if (number)
                    BACnet_COV_Support = 1;
                else
                    BACnet_COV_Support = 0;
                break;
            case 'C':
                number = strtol(p_data, NULL, 0);
                BACnet_COV_Lifetime = number;
                break;

            case 'D':
                number = strtol(p_data, NULL, 0);
                if ((number >= 0L) && (number <= 9))
                    debug_set_level(number);
                else
                    printf("Invalid debug level. Using default.\n");
                break;

            case 'h':
                number = strtol(p_data, NULL, 0);
                if ((number >= 0L) && (number <= 0xFFFFL))
                    BACnet_HTTP_Port = number;
                else
                    printf("Invalid BACnet HTTP port number. "
                        "Using default.\n");
                break;

            case 'I':
                number = strtol(p_data, NULL, 0);
                if (number >= 0L)
                    BACnet_Invoke_Ids = number;
                else
                    printf("Invalid number of concurrent Invoke Ids. "
                        "Using default.\n");
                break;

            case 'q':
                number = strtol(p_data, NULL, 0);
                BACnet_Initial_Query_Delay = number;
                break;

            case 'r':
                readFile = p_data;
                break;

            case 's':
                number = strtol(p_data, NULL, 0);
                BACnet_Time_Sync_Seconds = number;
                break;

            case 'w':
                writeFile = p_data;
                break;


            case 'x':

                number =
                    sscanf(p_data, "%d-%d", &deviceRangeMin,
                    &deviceRangeMax);

                if (number == 2) {
                    if (deviceRangeMin > deviceRangeMax) {
                        fprintf(stderr,
                            "Device range spec error: Min GT Max\n");
                        exit(1);
                    }
                    if (NULL == deviceRangeList) {
                        deviceRangeList =
                            calloc(1, sizeof(struct deviceRange));
                        if (NULL == deviceRangeList) {
                            fprintf(stderr,
                                "Out of memory allocating deviceRangelist\n");
                            exit(1);
                        }
                        deviceRangeList->min = deviceRangeMin;
                        deviceRangeList->max = deviceRangeMax;
                    } else {
                        deviceRangeTemp = deviceRangeList;
                        while (deviceRangeTemp->next)
                            deviceRangeTemp = deviceRangeTemp->next;
                        deviceRangeTemp->next =
                            calloc(1, sizeof(struct deviceRange));
                        if (NULL == deviceRangeTemp->next) {
                            fprintf(stderr,
                                "Out of memory allocating deviceRangelist\n");
                            exit(1);
                        }
                        deviceRangeTemp = deviceRangeTemp->next;
                        deviceRangeTemp->min = deviceRangeMin;
                        deviceRangeTemp->max = deviceRangeMax;
                    }
                }

                break;
            case '-':
                if (strcmp(p_data, "help") == 0)
                    Usage(argv[0]);
                exit(1);
                break;
            default:
                break;
            }                   /* end of arguments beginning with - */
        }                       /* dash arguments */
    }                           /* end of arg loop */
    options_interpret_arguments(argc, argv);
    return;
}

// dummy function 
void receive_mstp(void)
{

}

/* main execution loop */
int main(int argc,              /* number of arguments entered at command line */
    char *argv[])
{                               /* arguments in string form. */
    time_t t = 0;               // used with the time sync
    time_t startup_delay_time = 0;      // number of seconds to wait before polling
    time_t t1 = 0, t2 = 0;      // used to tell the amount of time difference
    bool relax = false;         // relax if leisurely polling time or values

    // interpret the command line arguments and possibly exit
    Interpret_Arguments(argc, argv);
    // show a splash screen of init parameters
    debug_printf(1, "BACnet4Linux %s\n", Program_Version);
    debug_printf(2, "MAIN: Program built: %s %s\n", __TIME__, __DATE__);
    debug_printf(2, "MAIN: Configuration from command line parameters:\n");
    debug_printf(2, "MAIN:      Acting as BACnet Device: %d\n",
        BACnet_Device_Instance);
    debug_printf(2, "MAIN:      Vendor ID: %d\n",
        BACnet_Vendor_Identifier);
    debug_printf(2, "MAIN:      APDU Timeout: %d seconds\n",
        BACnet_APDU_Timeout);
    debug_printf(2, "MAIN:      802.2 Ethernet interface: %s (%s)\n",
        BACnet_Device_Interface,
        BACnet_Ethernet_Enable ? "enabled" : "disabled");
    debug_printf(2, "MAIN:      Using BACnet/IP port: %d (0x%X)\n",
        BACnet_UDP_Port, BACnet_UDP_Port);
    debug_printf(2, "MAIN:      Using HTTP port: %d\n", BACnet_HTTP_Port);
    debug_printf(2, "MAIN:      COV Support: %s\n",
        BACnet_COV_Support ? "enabled" : "disabled");
    debug_printf(2, "MAIN: Structure Sizeofs:\n");
    debug_printf(2, "MAIN:      Device    : %4d bytes\n",
        sizeof(struct BACnet_Device_Info));
    debug_printf(2, "MAIN:      ObjectRef : %4d bytes\n",
        sizeof(struct ObjectRef_Struct));
    debug_printf(2, "MAIN:      NPDU      : %4d bytes\n",
        sizeof(struct BACnet_NPDU));
    debug_printf(2,
        "MAIN:      InvokeID  : %4d bytes * %d IDs = %d bytes\n",
        sizeof(struct Invoke_Status_Struct), MAXINVOKEIDS,
        MAXINVOKEIDS * sizeof(struct Invoke_Status_Struct));

    invoke_id_init();

    // initialize device cache
    device_init();

    // add me to the device cache - stores local details.
    get_local_ip_address(BACnet_Device_Interface,
        &BACnet_Device_IP_Address);
    debug_printf(2, "MAIN: LocalIP=%s\n",
        inet_ntoa(BACnet_Device_IP_Address));
    get_local_ip_broadcast_address(BACnet_Device_Interface,
        &Local_IP_Broadcast_Address);

    /* Attempt to open the 802.2 socket */
    if (BACnet_Ethernet_Enable)
        ethernet_init(BACnet_Device_Interface);

    /* Attempt to open the BACnet/IP socket */
    if (BACnet_UDP_Port)
        bip_sockfd =
            open_bip_socket(BACnet_Device_Interface, BACnet_UDP_Port);

    /* Attempt to open the TCP socket */
    if (BACnet_HTTP_Port)
        http_sockfd = open_tcp_socket(BACnet_HTTP_Port);
    debug_printf(3, "MAIN: HTTP ServerSocket=%d\n", http_sockfd);

    /* Attempt to open the MSTP loopback socket */
    if (BACnet_MSTP_Port)
        mstp_sockfd = open_mstp_socket(BACnet_MSTP_Port);

    /* Establish a signal handler for ctrl-c, SIGHUP and SIGTERM to provide clean bailout */
    signal_init();

    /* Create local device entry before adding GPIO objects */
    device_add(BACnet_Device_Instance);
    
    /* Initialize GPIO objects for Raspberry Pi */
    // Cleanup any existing GPIO state before initialization
    debug_printf(1, "MAIN: Performing GPIO cleanup before initialization\n");
    system("python3 -c \"import RPi.GPIO as GPIO; GPIO.setmode(GPIO.BCM); GPIO.cleanup()\" 2>/dev/null");
    system("python3 -c \"from gpiozero import Device; Device.pin_factory.reset()\" 2>/dev/null");
    
    gpio_objects_init(BACnet_Device_Instance);
    
    debug_printf(3, "MAIN: Ready to go...\n\n");
    // epoch time in seconds
    t1 = time(NULL);
    /* send initial I-Am and Who-Is */
    send_iam(BACnet_Device_Instance, BACnet_Vendor_Identifier);
    /* send Who-Is to all devices (-1) on the network */
    // note: generally not a good network practice, unless you are a 
    // bacnet device browser, which we are.

    if (BACnet_Initial_Query_Delay)
        send_whois(-1);

    /* start endless main loop (ctrl-c is the way out) */
    while (1) {
        /* Update GPIO input values from hardware */
        gpio_update_inputs(BACnet_Device_Instance);
        
        /* needed for coming select */
        fd_set read_fds;
        int max;

        /* time to wait if no activity */
        struct timeval select_timeout;

        if (relax) {
            select_timeout.tv_sec = 1;
            select_timeout.tv_usec = 0;
        } else {
            select_timeout.tv_sec = 0;
            select_timeout.tv_usec = 1000;
        }

        /* cleanup outstanding invoke IDs */
        invoke_id_cleanup();

        FD_ZERO(&read_fds);     /* clear the file handle set */
        max = 0;                /* reset max */

        /* Add the open file handles to the select set and update max */
        if (mstp_sockfd >= 0) {
            /* mstp interface, if open */
            FD_SET(mstp_sockfd, &read_fds);
            if (max < mstp_sockfd)
                max = mstp_sockfd;
        }
        if (ethernet_valid()) {
            FD_SET(ethernet_socket(), &read_fds);       /* 802.2 interface */
            if (max < ethernet_socket())
                max = ethernet_socket();
        }
        if (bip_sockfd >= 0) {
            FD_SET(bip_sockfd, &read_fds);      /* BACnet/IP interface */
            if (max < bip_sockfd)
                max = bip_sockfd;
        }
        if (http_sockfd >= 0) {
            FD_SET(http_sockfd, &read_fds);
            if (max < http_sockfd)
                max = http_sockfd;
        }

        /* check for activity on the file descriptors and process if there is */
        /* CLB -- In theory we can add a receive here for MSTP, considering the driver is
           implemented correctly.  Right now, I'm thinking of an MSTP kernel module 
           that exports a flag to indicate a reception as well as a buffer.  
           -------------------------------------------------------------------------------------
           We could also have the MSTP kernel driver send a loopback message to a socket
           opened here (say, 17285 ( or whatever port we use in the PUP interface).  
           That would probably be the most correct way of doing things, since we 
           can use it in the select function below.  This would be very similar to the PUP 
           driver loopback interface we've implemented.
           This method would allow the MSTP driver to be a low level kernel module (with a 
           bottom end layer) to accomodate the low latency requirements of MSTP).
           -------------------------------------------------------------------------------------
           There will also need to be a port opened on the loopback for generic reads and writes
           I want there to be a central process that runs and sends queries to either PUP, BACnet, LON, whateverOA
           -------------------------------------------------------------------------------------
         */
        if (select(max + 1, &read_fds, NULL, NULL, &select_timeout) > 0) {
            // http socket has data waiting
            if ((http_sockfd >= 0) && (FD_ISSET(http_sockfd, &read_fds)))
                receive_http();
            // 802.2 socket has data waiting
            if (ethernet_valid()
                && (FD_ISSET(ethernet_socket(), &read_fds)))
                ethernet_receive(ethernet_socket());
            // BACnet/IP socket has data waiting
            if ((bip_sockfd >= 0) && (FD_ISSET(bip_sockfd, &read_fds)))
                receive_bip();
            // MS/TP socket has data waiting
            if ((mstp_sockfd >= 0) && (FD_ISSET(mstp_sockfd, &read_fds)))
                receive_mstp();
        }
        // wait before polling
        if (BACnet_Initial_Query_Delay) {
            if (startup_delay_time < BACnet_Initial_Query_Delay) {
                t2 = time(NULL);
                if (t2 != -1) {
                    // check for change of seconds to spit out message
                    if ((t2 - t1) != startup_delay_time) {
                        startup_delay_time = t2 - t1;
                        debug_printf(2,
                            "MAIN: Delaying query: %d seconds of %d seconds.\n",
                            startup_delay_time,
                            BACnet_Initial_Query_Delay);
                    }
                }
            } else {
                /* query any newly found devices */
                relax = query_new_device();
                /* verify that all known controllers are still online */
                check_online_status();
            }

            /* re-sychronize all devices' time and date at the top of every hour */
            /* I use xntp to ensure the time is always accurate */
            if (BACnet_Time_Sync_Seconds) {
                t = time(NULL);
                if ((t % BACnet_Time_Sync_Seconds) == 0)
                    time_synch();
            }
        }
        /* end of select on file activity */
    }
    /* while (1) */
}

/* end of main.c */
