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
 along with this program; if not, write to 
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330 
 Boston, MA  02111-1307, USA.

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
#include "bacnet_device.h"
#include "bacnet_api.h"
#include "invoke_id.h"
#include "debug.h"
#include "main.h"


void bacnet_print_error(char *apdu, int len)
{
    debug_printf(3, "Error!\n");
}

void bacnet_print_reject(char *apdu, int len)
{
    // Reject-PDU
    // pdu type [0] Unsigned -- 6 for this PDU type
    // reserved [1] Unsigned -- must be 0
    // original-invokeID [2] Unsigned 0-255
    // reject reason [3] enumeration

    debug_printf(3, "%s\n", enum_to_text_reject_reason(apdu[3]));
}

void bacnet_print_abort(char *apdu, int len)
{
    debug_printf(3, "Abort!\n");
}
