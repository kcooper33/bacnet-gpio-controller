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
#ifndef DEBUG_H
#define DEBUG_H

#include <time.h>

void debug_init(void);
void debug_printf(int level, char *fmt, ...);
void debug_error_printf(int line, char *file, char *fmt, ...);
void debug_set_level(int level);
int debug_get_level(void);
time_t debug_uptime(void);
void debug_dump_data(int level, unsigned char *dataBuff,
    unsigned int dataSize);

#define error_printf(format, arg...) \
  debug_error_printf(__LINE__, __FILE__, format, ## arg)

#define debug_where(x) debug_print_where(x,__LINE__, __FILE__)

#endif
