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
#include <stdio.h>              /* Standard I/O */
#include <stdlib.h>             /* Standard Library */
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>

// default debug verbose level
static int Debug_Level = 1;
static time_t start_time;       /* the time execution started */
static struct timeval start_tv;

//#define DEBUG

void debug_set_level(int level)
{
    Debug_Level = level;

    return;
}

int debug_get_level(void)
{
    return Debug_Level;
}

void debug_init(void)
{
    static int initialized = 0;

    if (!initialized) {
        initialized = 1;
        start_time = time(NULL);        /* record time execution started */
        gettimeofday(&start_tv, NULL);
    }
}

time_t debug_uptime(void)
{
    time_t t;

    debug_init();
    t = time(NULL);

    return t - start_time;
}

/* allows output to be time stamped prior to output */
void debug_printf(int level, char *fmt, ...)
{
    va_list ap;
    struct timeval tv;

    if (level <= Debug_Level) {
        debug_init();
        gettimeofday(&tv, NULL);
        fprintf(stdout, "%lu.%03lu: ",
            tv.tv_sec - start_tv.tv_sec, tv.tv_usec / 1000);
        va_start(ap, fmt);
        vfprintf(stdout, fmt, ap);
        va_end(ap);
        fflush(stdout);
    }

    return;
}

/* allows stderr to be time stamped prior to output */
// debug_error_printf(__LINE__, __FILE__,"unable to alloc");
void debug_error_printf(int line, char *file, char *fmt, ...)
{
    va_list ap;
    struct timeval tv;

    debug_init();
    gettimeofday(&tv, NULL);
    fprintf(stderr, "%lu.%03lu: ",
        tv.tv_sec - start_tv.tv_sec, tv.tv_usec / 1000);
    if (file) {
        fprintf(stderr, "%s:%d: ", file, line);
    }
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stderr);

    return;
}

/* dumps line and file location when called with __LINE__ __FILE__ */
/* useful for debugging */
void debug_print_where(int level, int line, char *file)
{
    debug_printf(level, "debug: at line %4d in file %s\n", line, file);
}

/* Dumps an arbitrary block of memory in 'debug' fashion
*/
void debug_dump_data(int level, unsigned char *pBlock, unsigned int num)
{
    size_t lines = 0;           /* number of lines to print */
    size_t line = 0;            /* line of text counter */
    size_t last_line = 0;       /* line on which the last text resided */
    unsigned long count = 0;    // address to print
    unsigned int i = 0;         // counter

    if (level <= Debug_Level) {
        if (pBlock && num) {
            /* how many lines to print? */
            num--;              /* adjust */
            lines = (num / 16) + 1;
            last_line = num % 16;

            /* create the line */
            for (line = 0; line < lines; line++) {
                /* start with the address */
                printf("%08lX: ", count);
                /* hex representation */
                for (i = 0; i < 16; i++) {
                    if (((line == (lines - 1)) && (i <= last_line)) ||
                        (line != (lines - 1))) {
                        printf("%02X ", (unsigned) (0x00FF & pBlock[i]));
                    } else
                        printf("-- ");
                }
                printf(" ");
                /* print the characters if valid */
                for (i = 0; i < 16; i++) {
                    if (((line == (lines - 1)) && (i <= last_line)) ||
                        (line != (lines - 1))) {
                        if (isalnum(pBlock[i])) {
                            printf("%c", pBlock[i]);
                        } else
                            printf(".");
                    } else
                        printf(".");
                }
                printf("\r\n");
                pBlock += 16;
                count += 16;
            }
        }
    }

    return;
}
