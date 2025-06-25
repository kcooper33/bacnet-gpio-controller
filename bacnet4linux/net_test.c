/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2003 Steve Karg

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
#include "net.h"

#ifndef MAX_TCP_SIZE
#define MAX_TCP_SIZE 32000
#endif
static char html_response[MAX_TCP_SIZE];
static const char *HTML_HEADER =
    "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
    "<HTML>\n" "<HEAD>\n" "<TITLE>HTTP Test Page</TITLE>\n" "</HEAD>\n";
static const char *HTML_FOOTER = "</body>\n" "</html>\n";

static int client_socket;
static int server_socket;
static int Server_Port = 8000;

static void signal_handler(int signo)
{
    if (client_socket > 0)
        close(client_socket);

    if (server_socket > 0)
        close(server_socket);

    exit(0);
}

static void send_TCP(char *mime_type, int content_length)
{
    int i;
    char http_header[200] = "";
    size_t header_len = 0;

    /* no way to send anything back */
    if (client_socket < 0)
        return;

    if ((strlen(mime_type) <= 0) || (strlen(mime_type) > 99))
        sprintf(mime_type, "text/html");

    if (content_length) {
        /* add headers and send back to browser */
        snprintf(http_header, 200,
            "HTTP/1.0 200 Ok\r\n"
            "Server: BACNET_HTTP\r\n"
            "Content-type: %s\r\n"
            "Content-length: %d\r\n"
            "Connection: close\r\n" "\r\n", mime_type, content_length);

        header_len = strlen(http_header);
        for (i = content_length; i >= 0; i--) {
            /* jog content to allow room for header */
            *(html_response + i + header_len) = *(html_response + i);
        }
        memcpy(html_response, http_header, header_len);
        content_length += header_len;
        write(client_socket, html_response, content_length);
    }
    close(client_socket);
}

static void construct_index_page(char *url, char *html_response)
{
    char temp_html[1000] = "";
    (void) url;
    html_response[0] = 0;
    strcat(html_response, HTML_HEADER);
    sprintf(temp_html,
        "<h1>Server Configuration</h1>\n"
        "<p>Server is running on port %d.</p>\n", Server_Port);
    strcat(html_response, HTML_FOOTER);
}

static void receive_TCP(void)
{
    int i = 0;                  /* general purpose counters */
    int numread = 0;
    char url[MAX_TCP_SIZE] = "";        /* URL of browser's request GET _____ */
    FILE *external_file = NULL; /* file handle for reading files */
    char file_to_open[100] = "";
    char content_type[20] = ""; /* html doc Content-Type field */
    int content_length = 0;     /* size of html content in bytes */

    numread = read(client_socket, &url[0], MAX_TCP_SIZE);
    if (numread > 0) {
        url[numread] = '\0';

        strcpy(content_type, "text/html");      /* default content type is html */

        /* GET method from browser */
        if (strncmp(url, "GET", 3) == 0) {
            for (i = 4; i < numread; i++) {     /* dump GET */
                /* space separating html file from protocol */
                if ((url[i] == ' ') || (i > MAX_TCP_SIZE))
                    break;
                url[i - 4] = url[i];
            }
            url[i - 4] = '\0';  /* terminate URL string */

            strcpy(html_response, "");
            content_length = 0; /* default to no content */

            ///////////////////// INDEX ////////////////////////////////////     
            if ((strcmp(url, "/index.html") == 0) || (strlen(url) == 1)) {
                construct_index_page(url, html_response);
                content_length = strlen(html_response);
                send_TCP(content_type, content_length);
            }
            ////////////////// EXTERNAL FILES (images, css) ////////////////
            if ((strncmp(&url[strlen(url) - 5], ".html", 5) != 0) && (content_length == 0)) {   /* not .html */
                /* png image going back to browser */
                if (strncmp(&url[strlen(url) - 4], ".png", 4) == 0)     /* ?.png */
                    strcpy(content_type, "image/png");
                /* css file going back to browser */
                if (strncmp(&url[strlen(url) - 4], ".css", 4) == 0)     /* ?.css */
                    strcpy(content_type, "text/html");

                snprintf(file_to_open, 99, "%s", &url[1]);
                /* file opened for reading */
                if ((external_file = fopen(file_to_open, "r")) != NULL) {
                    i = 0;
                    while (!feof(external_file)) {
                        fread((html_response + i), 1, 1, external_file);
                        i++;
                        /* don't overflow buffer */
                        if (i >= (MAX_TCP_SIZE - 1000))
                            break;
                    }
                    content_length = i;
                    fclose(external_file);
                }
                send_TCP(content_type, content_length);
            }
        }
    }
}

int main(int argc,              /* number of arguments entered at command line */
    char *argv[])
{                               /* arguments in string form. */
    server_socket = open_tcp_socket(Server_Port);

    // Establish a signal handler for ctrl-c, SIGHUP and SIGTERM 
    // Provides clean bailout 
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);

    // main task
    while (1) {
        fd_set read_fds;
        int max = 0;

        struct timeval select_timeout;
        select_timeout.tv_sec = 1;      /* wait a second if no activity */
        select_timeout.tv_usec = 0;

        /* prepare to accept a TCP connection to the open server socket */
        client_socket = open_tcp_client(server_socket);

        FD_ZERO(&read_fds);     /* clear the file handle set */

        if (client_socket > 0)
            FD_SET(client_socket, &read_fds);   /* TCP client socket, if open */

        if (select(max + 1, &read_fds, NULL, NULL, &select_timeout) > 0) {
            if (client_socket > 0) {    /* TCP client socket is open */
                if (FD_ISSET(client_socket, &read_fds)) {       /* character received on TCP socket */
                    receive_TCP();
                }
            }
        }
    }

    return 0;
}
