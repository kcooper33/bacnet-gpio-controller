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
 Modified by Coleman Brumley <cbrumley@users.sourceforge.net> 20 Nov 2003
 -------------------------------------------
####COPYRIGHTEND####*/
//
// html generation and TCP socket handling.
//

//
// Note: Please read and follow:
// "Secure Programming for Linux and Unix HOWTO"
// especially Chapter 6, "Avoid Buffer Overflow"
//
// See also "Practical Unix & Internet Security, 3rd Edition"
// the chapter "Secure Programming Techniques"
// by Gene Spafford, Simson Garfinkel, Alan Schwartz
// http://www.onlamp.com/lpt/a/3365
//

#include "os.h"
#include "net.h"
#include "ethernet.h"
#include "bacnet_struct.h"
#include "bacnet_enum.h"
#include "bacnet_const.h"
#include "bacnet_api.h"
#include "bacnet_device.h"
#include "bacnet_object.h"
#include "bacnet_text.h"
#include "invoke_id.h"
#include "main.h"
#include "options.h"
#include "dstring.h"
#include "dbuffer.h"
#include "version.h"
#include "debug.h"

/* max number of bytes in one HTTP request/reply */
#define MAX_TCP_SIZE 30000
#define MAX_MIME_SIZE 100

static const char *RFC1123FMT = { "%a, %d %b %Y %H:%M:%S GMT" };
static const char *SERVER_NAME = { "BACnet4Linux" };
static const char *SERVER_URL = { "http://bacnet4linux.sourceforge.net/" };
static const char *PROTOCOL = { "HTTP/1.0" };

/* locals */
static int client_sockfd = -1;

// any necessary function prototypes
static void send_error(int status, char *title, char *extra_header,
    char *text);

// cleanup usually called by the sig-shutdown function
void html_cleanup(void)
{
    if (client_sockfd >= 0) {
        close(client_sockfd);
        debug_printf(3, "html: Closed http client socket\n");
    }

    return;
}

static void check_mime_type(OS_DString mime_type)
{
    if (mime_type) {
        // put a default in place if not valid
        // FIXME: is this an arbitrary limit of 100 bytes, or is it standard?
        if ((DString_Length(mime_type) <= 0)
            || (DString_Length(mime_type) >= 99))
            DString_Copy(mime_type, "text/html");
    }

    return;
}

static void check_html_content(OS_DString html_content)
{
    if ((html_content) && (DString_Length(html_content) == 0))
        send_error(404, "Not Found", NULL, "File not found.");

    return;
}

static size_t create_header(OS_DString dstring, int status,
    char *title, char *extra_header, char *mime_type, off_t length,
    time_t mod)
{
    time_t now;
    char timebuf[100];
    size_t header_size = 0;     // return value

    now = time((time_t *) 0);
    (void) strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
    header_size = DString_Printf(dstring,
        "%s %d %s\r\n"
        "Server: %s\r\n"
        "Date: %s\r\n", PROTOCOL, status, title, SERVER_NAME, timebuf);

    if (extra_header)
        header_size = DString_Append_Printf(dstring,
            "%s\r\n", extra_header);
    if (mime_type)
        header_size = DString_Append_Printf(dstring,
            "Content-Type: %s\r\n", mime_type);
    if (length >= 0)
        header_size = DString_Append_Printf(dstring,
            "Content-Length: %lld\r\n", (int64_t) length);
    if (mod != (time_t) - 1) {
        (void) strftime(timebuf, sizeof(timebuf), RFC1123FMT,
            gmtime(&mod));
        header_size = DString_Append_Printf(dstring,
            "Last-Modified: %s\r\n", timebuf);
    }
    header_size = DString_Append_Printf(dstring,
        "Connection: close\r\n" "\r\n");

    return header_size;
}

/* send_TCP sends a fully created document back to the browser */
static void send_TCP(char *mime_type, char *http_buffer, size_t http_size)
{
    OS_DString http_header;
    size_t http_header_size = 0;
    OS_DBuffer tcp_response;
    size_t tcp_size = 0;

    if (mime_type && http_buffer && http_size) {
        tcp_response = DBuffer_Create();
        http_header = DString_Create();
        if ((!http_header) || (!tcp_response)) {
            if (tcp_response)
                DBuffer_Delete(tcp_response);
            if (http_header)
                DString_Delete(http_header);
            return;
        }

        /* add headers and send back to browser */
        http_header_size =
            create_header(http_header, 200, "BACnet Network", NULL,
            mime_type, http_size, -1);
        tcp_size =
            DBuffer_Init(tcp_response, DString_Data(http_header),
            http_header_size);
        tcp_size = DBuffer_Append(tcp_response, http_buffer, http_size);
        debug_printf(2, "tcp: Sending %d bytes back to browser\n",
            tcp_size);
        write(client_sockfd, DBuffer_Data(tcp_response), tcp_size);
        // cleanup
        DString_Delete(http_header);
        DBuffer_Delete(tcp_response);
    }

    return;
}

static void send_HTML(OS_DString mime_type,     /* html doc Content-Type field */
    OS_DString html_content)
{
    if (client_sockfd < 0)      /* no way to send anything back */
        return;

    if ((!mime_type) || (!html_content))
        return;

    check_mime_type(mime_type);
    check_html_content(html_content);

    debug_printf(3, "http: Sending %s of %d bytes back to browser\n",
        DString_Data(mime_type), DString_Length(html_content));

    send_TCP(DString_Data(mime_type),
        DString_Data(html_content), DString_Length(html_content));

    return;
}

static void send_error(int status, char *title, char *extra_header,
    char *text)
{
    size_t len = 0;
    OS_DString dstring;

    dstring = DString_Create();
    if (!dstring) {
        error_printf("html: unable to create string for error %d.\n");
        return;
    }
    len = create_header(dstring, status, title, extra_header,
        "text/html", -1, -1);
    len = DString_Append_Printf(dstring,
        "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\n"
        "<BODY BGCOLOR=\"#cc9999\"><H4>%d %s</H4>\n",
        status, title, status, title);
    len = DString_Append_Printf(dstring, "%s\n", text);
    len = DString_Append_Printf(dstring,
        "<HR>\n<ADDRESS><A target=\"_blank\" "
        "HREF=\"%s\">%s</A></ADDRESS>\n</BODY></HTML>\n",
        SERVER_URL, SERVER_NAME);
    debug_printf(2, "tcp: Sending %d bytes back to browser\n", len);
    write(client_sockfd, DString_Data(dstring), len);
    // cleanup
    DString_Delete(dstring);
}

static size_t create_html_header(OS_DString dstring, int refresh)
{
    size_t header_size = 0;     // return value

    header_size = DString_Printf(dstring,
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
        "<HTML>\n" "<HEAD>\n" "<TITLE>BACnet Network</TITLE>\n");
    header_size = DString_Append_Printf(dstring,
        "<meta http-equiv=\"Content-Type\" content=\"text/html; "
        "charset=utf-8\">\n");
    if (refresh)
        header_size = DString_Append_Printf(dstring,
            "<meta http-equiv=\"refresh\" content=\"%d\">\n", refresh);
    header_size = DString_Append_Printf(dstring,
        "<link rel=StyleSheet href=\"html.css\" type=\"text/css\">\n"
        "</HEAD>\n");

    return header_size;
}

static const char *get_object_bgcolor(int object_type)
{
    const char *bgcolor = "#AF206F";

    // create the background colors
    switch (object_type) {
    case OBJECT_ANALOG_INPUT:
    case OBJECT_BINARY_INPUT:
        bgcolor = "#FF0000";
        break;
    case OBJECT_ANALOG_OUTPUT:
    case OBJECT_BINARY_OUTPUT:
        bgcolor = "#00FF00";
        break;
    case OBJECT_ANALOG_VALUE:
    case OBJECT_BINARY_VALUE:
        bgcolor = "#7F7FFF";
        break;
    default:
        break;
    }

    return bgcolor;
}

static void construct_object_list(long device_id, OS_DString response_html)
{
    struct BACnet_Device_Info *dev_ptr = NULL;
    struct ObjectRef_Struct *obj_ptr = NULL;    /* a single object reference */
    int i;                      // counter
    const char *bgcolor = "#AF206F";
    const char *valuecolor = "#FF0000";
    OS_DString object_value = NULL;     // used to store the object value
    int num_objects = 0;        // number of objects in the device
    int previous_object_type = -1;

    object_value = DString_Create();
    if (!object_value) {
        if (object_value)
            DString_Delete(object_value);
        return;
    }

    debug_printf(3, "http: creating object list for Device %ld\n",
        device_id);
    dev_ptr = device_get(device_id);
    if (dev_ptr) {
        // since the object list is sorted by key (object+id)
        // we don't have to sort them here
        num_objects = object_count(device_id);
        for (i = 0; i < num_objects; i++) {
            obj_ptr = object_get_by_index(dev_ptr, i);
            if (!obj_ptr)
                continue;
            // keeps them together...
            if (previous_object_type != obj_ptr->type) {
                previous_object_type = obj_ptr->type;
                bgcolor = get_object_bgcolor(obj_ptr->type);
                DString_Append_Printf(response_html,
                    "<tr>"
                    "<th colspan=4 bgcolor=\"%s\">"
                    "<center>%s Objects</center></th>"
                    "</tr>\n",
                    bgcolor, enum_to_text_object(obj_ptr->type));
            }
            // create the line with the object info
            switch (obj_ptr->type) {
            case OBJECT_ANALOG_INPUT:
            case OBJECT_ANALOG_OUTPUT:
            case OBJECT_ANALOG_VALUE:
            case OBJECT_LOOP:
                if (obj_ptr->value.real > 100)
                    DString_Printf(object_value,
                        "%.0f %s",
                        obj_ptr->value.real,
                        enum_to_text_units(obj_ptr->units.units));
                else if (obj_ptr->value.real > 10)
                    DString_Printf(object_value,
                        "%.1f %s",
                        obj_ptr->value.real,
                        enum_to_text_units(obj_ptr->units.units));
                else if (obj_ptr->value.real > 1)
                    DString_Printf(object_value,
                        "%.2f %s",
                        obj_ptr->value.real,
                        enum_to_text_units(obj_ptr->units.units));
                else
                    DString_Printf(object_value,
                        "%.3f %s",
                        obj_ptr->value.real,
                        enum_to_text_units(obj_ptr->units.units));
                valuecolor = "#CCCCCC";
                break;
            case OBJECT_BINARY_INPUT:
            case OBJECT_BINARY_OUTPUT:
            case OBJECT_BINARY_VALUE:
            case OBJECT_CALENDAR:
            case OBJECT_SCHEDULE:
                if (obj_ptr->value.binary) {    /* Active */
                    DString_Printf(object_value, "%s",
                        obj_ptr->units.states.active);
                    valuecolor = "#00FF00";
                } else {        /* Inactive */
                    DString_Printf(object_value,
                        "%s", obj_ptr->units.states.inactive);
                    valuecolor = "#FF0000";
                }
                break;
            case OBJECT_COMMAND:
            case OBJECT_DEVICE:
            case OBJECT_EVENT_ENROLLMENT:
            case OBJECT_FILE:
            case OBJECT_GROUP:
            case OBJECT_MULTI_STATE_INPUT:
            case OBJECT_MULTI_STATE_OUTPUT:
            case OBJECT_NOTIFICATION_CLASS:
            case OBJECT_PROGRAM:
            case OBJECT_AVERAGING:
            case OBJECT_MULTI_STATE_VALUE:
            case OBJECT_TRENDLOG:
            case OBJECT_LIFE_SAFETY_POINT:
            case OBJECT_LIFE_SAFETY_ZONE:
            default:
                DString_Printf(object_value, "--");
                valuecolor = "#FF0000";
                break;
            }
            DString_Append_Printf(response_html,
                "<tr>"
                "<td width=\"50\" bgcolor=\"%s\">%s</td>"
                "<td width=\"60\">%d</td>"
                "<td width=\"100\" bgcolor=\"%s\">%s</td>"
                "<td><a href=\"object%07d%04d%07d.html\" "
                "target=\"objectlist\">%s</a></td>"
                "</tr>\n",
                bgcolor,
                enum_to_text_object(obj_ptr->type),
                obj_ptr->instance,
                valuecolor,
                DString_Data(object_value),
                dev_ptr->device,
                obj_ptr->type, obj_ptr->instance, obj_ptr->name);
        }
    }
    DString_Delete(object_value);
}

// objectlist???.html 
// constructs an object list HTML page based on the URL passed in
// the constructed HTML page is returned in html_response
static void construct_object_list_page(char *url, OS_DString response_html)
{
    long device_id = 0;         // holds the decoded device id

    if (url && response_html) {
        device_id = strtol(&url[11], NULL, 10);
        create_html_header(response_html, 60);
        DString_Append_Printf(response_html,
            "<body>\n" "<p>Monitoring %d objects in Device %d.</p>"
            /* table header for object list */
            "<table width=\"100%%\" border=1>\n"
            "<colgroup span=\"4\"></colgroup>\n"
            "<tr>"
            "<th width=\"150px\">Object Type</th>"
            "<th width=\"60px\">Instance</th>"
            "<th width=\"100px\">Value</th>"
            "<th>Object Name</th>" "</tr>\n",
            object_count(device_id), device_id);
        construct_object_list(device_id, response_html);
        DString_Concat(response_html,
            "</table>\n"
            "<H4><a href=\"status.html\">Server Status</a></H4>\n"
            "<H4><a href=\"network.html\">Device List</a></H4>\n"
            "</body>\n" "</html>\n");
    }

    return;
}

static void construct_index_page(OS_DString response)
{
    if (response)
        DString_Copy(response,
            "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\""
            " \"http://www.w3.org/TR/html4/frameset.dtd\">\n"
            "<HTML>\n"
            "<HEAD>\n"
            "<TITLE>BACnet Network</TITLE>\n"
            "<meta http-equiv=\"Content-Type\" content=\"text/html; "
            "charset=utf-8\">\n"
            "<link rel=StyleSheet href=\"html.css\" type=\"text/css\">\n"
            "</HEAD>\n"
            "<frameset cols=\"250,*\">\n"
            "<frame src=\"/network.html\" name=\"network\" "
            "frameborder=\"1\" scrolling=yes noresize>\n"
            "<frameset rows=\"75,*\">\n"
            "<frame src=\"/title.html\" name=\"device\" "
            "frameborder=\"1\" scrolling=no noresize>\n"
            "<frame src=\"/status.html\" name=\"objectlist\" "
            "frameborder=\"1\" scrolling=yes noresize>\n"
            "</frameset>\n" "</frameset>\n</html>\n");
}

static void construct_title_page(OS_DString response)
{
    if (response) {
        create_html_header(response, 0);
        DString_Append_Printf(response,
            "<body>"
            "<center>"
            "<h1><a target=\"_blank\" href=\"%s\">%s</a></h1>"
            "</center>" "</body>" "</html>", SERVER_URL, SERVER_NAME);
    }
}

static const char *get_state_bgcolor(int state)
{
    const char *bgColors[] = {  // color coding of device state for HTML page 
        "#FFFFFF",              // White, signifies invalid state 
        "#FF0000",              // Red 
        "#FF9900",              // Orange 
        "#FFFF00",              // Yellow 
        "#00FF00",              // Green 
        "#999900",              // Brown 
        "#FF00FF"               // Magenta 
    };
    const char *state_color;

    if (state < 7)
        state_color = bgColors[state];
    else
        state_color = bgColors[0];

    return state_color;
}

static void construct_network_page(OS_DString response_html)
{
    struct BACnet_Device_Info *dev_ptr = NULL;
    struct BACnet_Device_Info *dev2_ptr = NULL;
    int i, j;                   // counters 
    time_t t = 0;               // used to show current time 
    int num_devices = 0;        // used to hold the number of devices that we have 
    OS_DString device_html;     // used to form each device 

    device_html = DString_Create();
    if (response_html && device_html) {
        t = time(NULL);         /* find current time */
        create_html_header(response_html, 20);
        DString_Append_Printf(response_html,
            "<body>\n"
            "<div align=\"center\"><a href=\"http://www.bacnet.org\" "
            "target=\"parent\"><img src=\"BACnetLogo.png\" "
            "alt=\"Visit the BACnet website at http://www.bacnet.org\">"
            "</a></div>\n"
            "<H4>%s</H4>\n"
            "<hr>\n"
            "<table width=\"100\%\" border=1>\n<colgroup span=\"2\">"
            "</colgroup>\n"
            "<tr>"
            "<th width=\"70px\">Device<br>Address</th>"
            "<th>Device<br>Name</th>" "</tr>\n", ctime(&t));
        // get a snapshot of the number - they may be changing... 
        num_devices = device_count();
        for (i = 0; i < num_devices; i++) {
            dev_ptr = device_record(i);
            if (!dev_ptr)
                continue;
            /* local network device (no routers) */
            if (dev_ptr->src.local) {
                /* remote devices are linked to their local router */
                DString_Printf(device_html,
                    "<tr>"
                    "<td><a href=\"device%d.html\" target=\"device\">%d</a></td>"
                    "<td BGCOLOR=\"%s\">%s</td>"
                    "</tr>\n",
                    dev_ptr->device,
                    dev_ptr->device,
                    get_state_bgcolor(dev_ptr->state),
                    dev_ptr->device_name);
                DString_Concat(response_html, DString_Data(device_html));
                for (j = 0; j < num_devices; j++) {     /* around again */
                    dev2_ptr = device_record(j);
                    if (!dev_ptr)
                        continue;
                    /* no comparison here */
                    if (i == j)
                        continue;
                    /* another local device, which couldn't be routed */
                    if (dev2_ptr->src.local)
                        continue;
                    /* both the known local device and another device originated from the same MAC */
                    if (memcmp(dev_ptr->src.mac, dev2_ptr->src.mac,
                            MAX_MAC_LEN) == 0) {
                        /* a device that is routed through the local device */
                        DString_Printf(device_html,
                            "<tr>"
                            "<td>&nbsp&nbsp&nbsp&nbsp&nbsp<a href=\"device%d.html\" target=\"device\">%d</a></td>"
                            "<td BGCOLOR=\"%s\">%s</td>"
                            "</tr>\n",
                            dev2_ptr->device, dev2_ptr->device,
                            get_state_bgcolor(dev_ptr->state),
                            dev2_ptr->device_name);
                        DString_Concat(response_html,
                            DString_Data(device_html));
                    }
                }
            }
        }
        DString_Concat(response_html,
            "</table>\n" "<hr>\n"
            "<H4><a href=\"status.html\">Server Status</a></H4>\n"
            "</body>\n" "</html>\n");
        DString_Delete(device_html);
    }

    return;
}

// device???.html 
static void construct_device_info_page(char *url, OS_DString response_html)
{
    struct BACnet_Device_Info *dev_ptr = NULL;
    long device_id = 0;         // holds the decoded device id
    OS_DString device_html;     // used to form each device

    device_html = DString_Create();
    if (!device_html)
        return;

    if (url && response_html) {
        create_html_header(response_html, 0);
        DString_Append_Printf(response_html,
            "<body>\n"
            "<table width=\"100\%\" border=1>\n"
            "<colgroup span=\"4\"></colgroup>\n"
            "<tr align=\"center\">"
            "<td>Device</td>"
            "<td width=\"100px\">Vendor</td>"
            "<td width=\"75px\">SNET</td>"
            "<td>SADR</td>" "<td>IP</td>" "</tr>\n");
        // figure out the device id from the url filename
        device_id = strtol(&url[7], NULL, 10);
        debug_printf(3, "http: creating info for Device %ld\n", device_id);
        dev_ptr = device_get(device_id);
        if (dev_ptr) {
            /* local network */
            if (dev_ptr->src.local) {
                DString_Printf(device_html,
                    "<tr align=\"center\">"
                    "<td>%d - %s</td>"
                    "<td><img src=\"id%d.png\" alt=\"%d\"></td>"
                    "<td>Local</td>"
                    "<td>%s</td>"
                    "<td>%s</td>"
                    "</tr>\n",
                    dev_ptr->device, dev_ptr->device_name,
                    dev_ptr->vendor_id, dev_ptr->vendor_id,
                    hwaddrtoa(dev_ptr->src.mac),
                    inet_ntoa(dev_ptr->src.ip));
            }
            /* routed device */
            else {
                DString_Printf(device_html,
                    "<tr align=\"center\">"
                    "<td>%d - %s</td>"
                    "<td><img src=\"id%d.png\" alt=\"%d\"></td>"
                    "<td>%d</td>"
                    "<td>%s</td>"
                    "</tr>\n",
                    dev_ptr->device, dev_ptr->device_name,
                    dev_ptr->vendor_id, dev_ptr->vendor_id,
                    dev_ptr->src.net, hwaddrtoa(dev_ptr->src.adr));
            }
            DString_Concat(response_html, DString_Data(device_html));
            // finish the table
            DString_Concat(response_html, "</table>\n");

            /* link to load object list */
            DString_Printf(device_html,
                "<script type=\"text/javascript\">parent.objectlist.location "
                "= \"objectlist%d.html\"</script>\n", dev_ptr->device);
            DString_Concat(response_html, DString_Data(device_html));
        } else {
            DString_Concat(response_html, "</table>\n");
        }
        DString_Concat(response_html, "</body>\n" "</html>\n");
    }
    DString_Delete(device_html);

    return;
}

// objectDDDDDDDOOOOIIIIIII.html 
// FIXME: perhaps object-device-type-instance.html is easier to parse?
static void construct_object_page(char *url, OS_DString response_html)
{
    int j = 0;                  // loop counter
    long device_id = 0;         // holds the decoded device id
    long objecttype = 0;
    long objectinstance = 0;
    char deviceinstancestr[10] = { "" };
    char objecttypestr[6] = { "" };
    char objectinstancestr[6] = { "" };
    enum BACnetPropertyIdentifier *prop_ptr;    /* pointer to an array of properties */
    struct BACnet_Device_Info *dev_ptr = NULL;
    struct ObjectRef_Struct *obj_ptr = NULL;    /* a single object reference */

    if (url && response_html) {
        create_html_header(response_html, 20);
        DString_Append_Printf(response_html,
            "<body>\n"
            "<table width=\"100\%\" cols=2 border=1>\n"
            "<tr>" "<th width=\"50%\">Property Name</th>"
            "<th width=\"50%\">Property Value</th>" "</tr>\n");

        /* break properties out of URL */
        strncpy(deviceinstancestr, &url[7], 7);
        device_id = strtol(deviceinstancestr, NULL, 10);
        debug_printf(3, "http: Looking for device %ld\n", device_id);
        dev_ptr = device_get(device_id);
        if (dev_ptr) {          /* a valid device */
            /* break properties out of URL */
            strncpy(objecttypestr, &url[14], 4);
            objecttype = strtol(objecttypestr, NULL, 10);
            strncpy(objectinstancestr, &url[18], 7);
            objectinstance = strtol(objectinstancestr, NULL, 10);
            /* object list */
            for (j = 0; j < object_count(dev_ptr->device); j++) {
                obj_ptr = object_get_by_index(dev_ptr, j);
                if (!obj_ptr)
                    continue;
                if ((obj_ptr->type == objecttype) &&
                    (obj_ptr->instance == objectinstance)) {
                    /* an object that is known to exist */
                    debug_printf(3,
                        "html: Requesting Device %d Properties for %s %d \n",
                        dev_ptr->device, enum_to_text_object(objecttype),
                        objectinstance);
                    /* properties of this type of object */
                    prop_ptr = getobjectprops(objecttype);
                    while (*prop_ptr != PROP_NO_PROPERTY) {
                        /* send read-property to object properties */
                        read_property(dev_ptr->device, objecttype,
                            objectinstance, *prop_ptr,
                            -1 /* array index */ );
                        prop_ptr++;     /* increment to next property */
                    }
                    break;      /* exit for j loop */
                }
            }                   /* all known objects list */
        }
        DString_Concat(response_html,
            "</table>\n" "</body>\n" "</html>\n");
    }

    return;
}

static void construct_status_page(OS_DString response_html)
{
    time_t t = 0;               // used for the current time
    float duration = 0.0;       // used to calculate the server duration uptime
    OS_DString status_html;     // used to form each status

    status_html = DString_Create();
    if (!status_html)
        return;

    if (response_html) {
        t = time(NULL);         /* find current time */
        create_html_header(response_html, 60);
        DString_Append_Printf(response_html,
            "<body>\n"
            "<div align=\"center\"><img src=\"BACnetLogo.png\" "
            "alt=\"Visit the BACnet website at http://www.bacnet.org\"></div>\n"
            "<div align=\"center\"><img src=\"linux.png\" "
            "alt=\"Powered by Linux\"></div>\n" "<H4>%s</H4>\n",
            ctime(&t));
        DString_Concat(response_html,
            "\n<hr>\n" "<table border=\"0\">\n"
            "<COLGROUP span=\"2\"></COLGROUP>\n");

        DString_Printf(status_html,
            "<tr>"
            "<th colspan=\"2\">"
            "<a target=\"_blank\" href=\"%s\">%s</a>"
            "</th>"
            "</tr>" "<tr>"
            "<td colspan=\"2\">%s built at %s on %s</td>" "</tr>\n",
            SERVER_URL, SERVER_NAME, Program_Version, __TIME__, __DATE__);
        DString_Concat(response_html, DString_Data(status_html));

        DString_Concat(response_html,
            "<tr>" "<th colspan=\"2\">Configuration:</th>" "</tr>\n");
        DString_Printf(status_html,
            "<tr>" "<td>Ethernet interface</td>"
            "<td>%s %s</td>" "</tr>\n", BACnet_Device_Interface,
            hwaddrtoa(Ethernet_MAC_Address));
        DString_Concat(response_html, DString_Data(status_html));

        DString_Printf(status_html,
            "<tr>" "<td>IP interface</td>"
            "<td>%s %s</td>" "</tr>\n",
            BACnet_Device_Interface, inet_ntoa(BACnet_Device_IP_Address));
        DString_Concat(response_html, DString_Data(status_html));

        DString_Printf(status_html,
            "<tr>" "<td>B/IP Port</td>"
            "<td>%s %4X</td>" "</tr>\n",
            BACnet_Device_Interface, BACnet_UDP_Port);
        DString_Concat(response_html, DString_Data(status_html));

        DString_Printf(status_html,
            "<tr>" "<td>IP broadcast</td>"
            "<td>%s %s</td>" "</tr>\n",
            BACnet_Device_Interface,
            inet_ntoa(Local_IP_Broadcast_Address));
        DString_Concat(response_html, DString_Data(status_html));

        DString_Printf(status_html,
            "<tr>" "<td>Acting as Device</td>"
            "<td>%d</td>" "</tr>\n", BACnet_Device_Instance);
        DString_Concat(response_html, DString_Data(status_html));

        DString_Printf(status_html,
            "<tr>" "<td>Using HTTP port</td>"
            "<td>%d</td>" "</tr>\n", BACnet_HTTP_Port);
        DString_Concat(response_html, DString_Data(status_html));

        DString_Printf(status_html,
            "<tr>" "<th colspan=\"2\">Structure Sizeofs:</th>" "</tr>\n");
        DString_Concat(response_html, DString_Data(status_html));

        DString_Printf(status_html,
            "<tr>" "<td>Device</td>"
            "<td>%4d bytes</td>" "</tr>\n",
            sizeof(struct BACnet_Device_Info));
        DString_Concat(response_html, DString_Data(status_html));

        DString_Printf(status_html,
            "<tr>" "<td>ObjectRef</td>"
            "<td>%4d bytes</td>" "</tr>\n",
            sizeof(struct ObjectRef_Struct));
        DString_Concat(response_html, DString_Data(status_html));

        DString_Printf(status_html,
            "<tr>" "<td>NPDU</td>" "<td>%4d bytes</td>"
            "</tr>\n", sizeof(struct BACnet_NPDU));
        DString_Concat(response_html, DString_Data(status_html));

        DString_Printf(status_html,
            "<tr>" "<td>InvokeID</td>"
            "<td>%4d bytes</td>" "</tr>\n",
            sizeof(struct Invoke_Status_Struct));
        DString_Concat(response_html, DString_Data(status_html));

        DString_Concat(response_html, "</table>\n");

        duration = (float) (debug_uptime()) / 3600.0;
        DString_Printf(status_html,
            "<p>Server has been running for %.2f hours, "
            "and is monitoring %d objects in %d devices.</p>\n",
            duration, object_total_count(), device_count());
        DString_Concat(response_html, DString_Data(status_html));
        DString_Append_Printf(response_html,
            "<hr>\n"
            "<H4><a href=\"network.html\">Return to Device List</a></H4>\n"
            "<div align=\"center\">\n"
            "<a target=\"_blank\" href=\"%s\">%s</a><br>"
            "<br>"
            "<a href=\"http://validator.w3.org/check/referer\">"
            "<img src=\"valid-html401.png\" "
            "alt=\"Valid HTML 4.01!\" height=\"31\" width=\"88\">"
            "</a>\n</div>\n" "</body>\n" "</html>\n",
            SERVER_URL, SERVER_NAME);
    }
    DString_Delete(status_html);
}

static char *get_mime_type(char *name)
{
    char *dot;

    dot = strrchr(name, '.');
    if (dot == (char *) 0)
        return "text/plain; charset=iso-8859-1";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=iso-8859-1";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";
    return "text/plain; charset=iso-8859-1";
}

                                                                                                                                                                                                                                                                                                                                                                                                                     /* receive commands from browser on TCP socket and send back responses in html *//* file handle for TCP client socket is global */
void receive_http(void)
{
    int i = 0;                  /* general purpose counters */
    int numread = 0;
    FILE *pFile = NULL;         /* file handle for reading files */
    size_t content_length = 0;  /* size of html content in bytes */
    OS_DString content_type;    /* html doc Content-Type field */
    OS_DString response;
    OS_DString filename;
    OS_DBuffer file_buffer;
    size_t bytes_read = 0;
    char file_chunk[512] = { "" };      // for reading a chunk of file
    char *url;                  /* URL of browser's request GET _____ */
    //sockaddr client_addr = {0};
    //socklen_t client_addr_len = 0;

    // get some memory off the heap
    url = calloc(1, MAX_TCP_SIZE);
    response = DString_Create();
    filename = DString_Create();
    content_type = DString_Create();
    client_sockfd = accept(http_sockfd, NULL, NULL);
    if ((client_sockfd < 0) || (!response) ||
        (!filename) || (!content_type) || (!url)) {
        if (client_sockfd >= 0)
            close(client_sockfd);
        else
            error_printf("html: client socket not available.\n");

        if (url)
            free(url);
        else
            error_printf("html: unable to allocate string for url.\n");

        if (response)
            DString_Delete(response);
        else
            error_printf
                ("html: unable to allocate string for response.\n");

        if (filename)
            DString_Delete(filename);
        else
            error_printf
                ("html: unable to allocate string for filename.\n");

        if (content_type)
            DString_Delete(content_type);
        else
            error_printf
                ("html: unable to allocate string for content type.\n");

        return;
    }

    numread = read(client_sockfd, &url[0], MAX_TCP_SIZE);
    if (numread > 0) {
        // truncate if necessary
        if (numread < (MAX_TCP_SIZE - 1))
            numread = (MAX_TCP_SIZE - 1);
        url[numread] = '\0';
        debug_printf(3, "html: Client sockfd %d\n", client_sockfd);
        debug_printf(2, "html: %d bytes received\n", numread);
        debug_printf(3, "html: http request:\n%s", url);
        /* default content type is html */
        content_length = DString_Copy(content_type, "text/html");
        if (content_length <= 0)
            goto http_cleanup;

        /* GET method from browser */
        if (strncmp(url, "GET", 3) == 0) {
            for (i = 4; i < numread; i++) {     /* dump GET */
                /* space separating html file from protocol */
                if ((url[i] == ' ') || (i > MAX_TCP_SIZE))
                    break;
                url[i - 4] = url[i];
            }
            url[i - 4] = '\0';  /* terminate URL string */
            debug_printf(2, "html: Browser is asking to GET %s\n", url);
            // setup response buffer
            DString_Copy(response, NULL);
            ///////////////////// FRAME INDEX ////////////////////////////////////     
            if ((strcmp(url, "/index.html") == 0)
                || (strlen(url) == 1)) {
                construct_index_page(response);
                send_HTML(content_type, response);
            }
            if ((strcmp(url, "/title.html") == 0)
                || (strlen(url) == 1)) {
                construct_title_page(response);
                send_HTML(content_type, response);
            }
            //////////////////// NETWORK OF CONTROLLERS //////////////////////////
            else if (strcmp(url, "/network.html") == 0) {
                construct_network_page(response);
                send_HTML(content_type, response);
            }
            //////////////////////// DEVICE INFORMATION //////////////////////////
            /* device information and table header pane for a specific device */
            else if ((strncmp(url, "/device", 7) == 0) &&
                (strncmp(&url[strlen(url) - 5], ".html", 5) == 0)) {
                construct_device_info_page(url, response);
                send_HTML(content_type, response);
            }
            //////////////////// OBJECT LIST /////////////////////////////////////    
            /* object list pane for a specific device */
            else if ((strncmp(url, "/objectlist", 11) == 0) &&
                (strncmp(&url[strlen(url) - 5], ".html", 5) == 0)) {
                construct_object_list_page(url, response);
                send_HTML(content_type, response);
            }
            //////////////////// INDIVIDUAL OBJECT PROPERTIES /////////////////////////////////////    
            /* specific object */
            else if ((strncmp(url, "/object", 7) == 0) &&
                (strncmp(&url[25], ".html", 5) == 0)) {
                construct_object_page(url, response);
                send_HTML(content_type, response);
            }
            //////////////////// STATUS OF BACnet for Linux //////////////////////////
            else if (strcmp(url, "/status.html") == 0) {
                construct_status_page(response);
                send_HTML(content_type, response);
            }
            //////////////////////// EXTERNAL FILES (images, css) /////////////////////////////////
            else if (strncmp(&url[strlen(url) - 5], ".html", 5) != 0) {
                DString_Printf(filename, "html/%s", &url[1]);
                DString_Copy(content_type,
                    get_mime_type(DString_Data(filename)));
                file_buffer = DBuffer_Create();
                if (file_buffer) {
                    /* file opened for reading */
                    if ((pFile =
                            fopen(DString_Data(filename), "r")) != NULL) {
                        while (!feof(pFile)) {
                            bytes_read =
                                fread(file_chunk, 1, sizeof(file_chunk),
                                pFile);
                            if (bytes_read) {
                                content_length =
                                    DBuffer_Append(file_buffer, file_chunk,
                                    bytes_read);
                            }
                        }
                        debug_printf(3,
                            "http: Read %d bytes from file named %s\n",
                            content_length, DString_Data(filename));
                        fclose(pFile);
                        send_TCP(DString_Data(content_type),
                            DBuffer_Data(file_buffer),
                            DBuffer_Size(file_buffer));
                    } else {
                        send_error(404, "Not Found", NULL,
                            "File not found.");
                    }
                    DBuffer_Delete(file_buffer);
                }
            } else {
                send_error(404, "Not Found", NULL, "File not found.");
                debug_printf(3, "html: URL \"%s\" not found\n", url);
            }
        } else
            send_error(501, "Not Implemented", NULL,
                "That method is not implemented.");
        // cleanup
        DString_Delete(content_type);
    } else
        debug_printf(3, "http: numread=%d\n", numread);

  http_cleanup:
    free(url);
    DString_Delete(response);
    DString_Delete(filename);
    close(client_sockfd);

    return;
}
