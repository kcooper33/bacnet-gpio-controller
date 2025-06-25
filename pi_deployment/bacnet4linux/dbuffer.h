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
#ifndef DBUFFER_H
#define DBUFFER_H

typedef struct DBuffer {
    size_t size;                // the strlen size of the string 
    char *data;                 // pointer to some data that is stored
} DBUFFER_TYPE;
typedef DBUFFER_TYPE *OS_DBuffer;

OS_DBuffer DBuffer_Create(void);
void DBuffer_Delete(OS_DBuffer DBuffer);

size_t DBuffer_Append(OS_DBuffer DBuffer, const char *data, size_t size);
size_t DBuffer_Init(OS_DBuffer DBuffer, const char *data, size_t size);
char *DBuffer_Data(OS_DBuffer DBuffer);
size_t DBuffer_Size(OS_DBuffer DBuffer);

#endif
