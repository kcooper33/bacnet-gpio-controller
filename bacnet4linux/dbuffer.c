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
 along with this program; if not, write to:
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

// Dynamic Buffer Library

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dbuffer.h"

static struct DBuffer *DBufferCreate(void)
{
    return calloc(1, sizeof(struct DBuffer));
}

OS_DBuffer DBuffer_Create(void)
{
    return DBufferCreate();
}

size_t DBuffer_Append(OS_DBuffer DBuffer, const char *data, size_t size)
{
    size_t total_size = 0;      // return value
    char *concat_data;          // the new data from realloc

    if (DBuffer) {
        if (data) {
            if (DBuffer->data) {
                total_size = DBuffer->size + size;
                concat_data = realloc(DBuffer->data, total_size);
                if (concat_data) {
                    DBuffer->data = concat_data;
                    memmove(DBuffer->data + DBuffer->size, data, size);
                    DBuffer->size = total_size;
                }
            } else {
                DBuffer->data = malloc(size);
                if (DBuffer->data) {
                    memmove(DBuffer->data, data, size);
                    DBuffer->size = size;
                } else
                    DBuffer->size = 0;
            }
        }
        // configure the return value
        total_size = DBuffer->size;
    }

    return total_size;
}

size_t DBuffer_Init(OS_DBuffer DBuffer, const char *data, size_t size)
{
    size_t total_size = 0;      // return value

    if (DBuffer) {
        // destroy old stuff
        if (DBuffer->data)
            free(DBuffer->data);
        // create new stuff
        if (data && size) {
            DBuffer->data = malloc(size);
            if (DBuffer->data) {
                memmove(DBuffer->data, data, size);
                DBuffer->size = size;
            } else
                DBuffer->size = 0;

        } else {
            DBuffer->data = NULL;
            DBuffer->size = 0;
        }

        // configure the return value
        total_size = DBuffer->size;
    }

    return total_size;
}

char *DBuffer_Data(OS_DBuffer DBuffer)
{
    char *data = NULL;

    if (DBuffer)
        data = DBuffer->data;

    return data;
}

size_t DBuffer_Size(OS_DBuffer DBuffer)
{
    size_t size = 0;

    if (DBuffer)
        size = DBuffer->size;

    return size;
}

void DBuffer_Delete(OS_DBuffer DBuffer)
{
    if (DBuffer) {
        DBuffer->size = 0;
        if (DBuffer->data) {
            free(DBuffer->data);
            DBuffer->data = NULL;
        }
        free(DBuffer);
    }

    return;
}

#ifdef TEST
#include <assert.h>
#include <string.h>

#include "ctest.h"

void testDBuffer(Test * pTest)
{
    OS_DBuffer dbuffer;
    char *data1 = "Joshua";
    char *data2 = "Anna";
    char *data3 = "Rose";
    char *data;
    size_t size;
    size_t size_data;
    size_t size_data3;

    // null parameter function check
    data = DBuffer_Data(NULL);
    ct_test(pTest, data == NULL);
    size = DBuffer_Size(NULL);
    ct_test(pTest, size == 0);
    size = DBuffer_Init(NULL, NULL, 0);
    ct_test(pTest, size == 0);
    size = DBuffer_Append(NULL, NULL, 0);
    ct_test(pTest, size == 0);

    // create
    dbuffer = DBuffer_Create();
    // verify creation
    ct_test(pTest, dbuffer != NULL);
    // verify initialization
    data = DBuffer_Data(dbuffer);
    ct_test(pTest, data == NULL);
    size = DBuffer_Size(dbuffer);
    ct_test(pTest, size == 0);

    // init
    size_data = strlen(data1);
    size = DBuffer_Init(dbuffer, data1, size_data);
    data = DBuffer_Data(dbuffer);
    ct_test(pTest, memcmp(data, data1, size_data) == 0);
    ct_test(pTest, size == size_data);
    size = DBuffer_Size(dbuffer);
    ct_test(pTest, size == size_data);
    // append - null check
    size = DBuffer_Append(dbuffer, NULL, 0);    // nothing should change
    ct_test(pTest, size == size_data);
    data = DBuffer_Data(dbuffer);
    ct_test(pTest, memcmp(data, data1, size_data) == 0);
    size = DBuffer_Size(dbuffer);
    ct_test(pTest, size == size_data);
    // append
    size = DBuffer_Init(dbuffer, NULL, 0);      // start clean
    size_data = strlen(data2);
    size = DBuffer_Append(dbuffer, data2, size_data);   // concat onto blank string
    data = DBuffer_Data(dbuffer);
    ct_test(pTest, memcmp(data, data2, size_data) == 0);
    ct_test(pTest, size == size_data);
    size = DBuffer_Size(dbuffer);
    ct_test(pTest, size == size_data);
    size_data3 = strlen(data3);
    size = DBuffer_Append(dbuffer, data3, size_data3);  // append second string
    ct_test(pTest, size == (size_data + size_data3));
    data = DBuffer_Data(dbuffer);
    ct_test(pTest, memcmp(data, data2, size_data) == 0);
    ct_test(pTest, memcmp(data + size_data, data3, size_data3) == 0);
    size = DBuffer_Size(dbuffer);
    ct_test(pTest, size == (size_data + size_data3));

    // delete
    size_data = 0;
    size = DBuffer_Init(dbuffer, NULL, 0);
    ct_test(pTest, size == size_data);
    data = DBuffer_Data(dbuffer);
    ct_test(pTest, data == NULL);
    size = DBuffer_Size(dbuffer);
    ct_test(pTest, size == 0);

    // cleanup
    DBuffer_Delete(dbuffer);

    return;
}

void testDBufferExercise(Test * pTest)
{
    OS_DBuffer dbuffer;
    char *data2 = "Steve and Patricia";
    char *data3 = "Anna, Joshua, Mary, Christopher";
    char *data;
    size_t size;
    size_t size_data;
    size_t size_data2;
    size_t size_data3;
    int i, j;                   // counters in the for loop


    // create
    dbuffer = DBuffer_Create();
    // verify creation
    ct_test(pTest, dbuffer != NULL);

    for (i = 0; i < 1024; i++) {
        size_data2 = strlen(data2);
        size_data3 = strlen(data3);
        size_data = DBuffer_Append(dbuffer, data2, size_data2); // append second string
        for (j = 0; j < 1024; j++) {
            size = DBuffer_Append(dbuffer, data3, size_data3);  // append second string
            ct_test(pTest, size == (size_data + size_data3));
            data = DBuffer_Data(dbuffer);
            // check the beginning
            ct_test(pTest, memcmp(data, data2, size_data2) == 0);
            // check the end
            ct_test(pTest, memcmp(data + size_data, data3,
                    size_data3) == 0);
            size = DBuffer_Size(dbuffer);
            ct_test(pTest, size == (size_data + size_data3));
            size_data = size;
        }
        // delete
        size_data = 0;
        size = DBuffer_Init(dbuffer, NULL, 0);
        ct_test(pTest, size == size_data);
        data = DBuffer_Data(dbuffer);
        ct_test(pTest, data == NULL);
        size = DBuffer_Size(dbuffer);
        ct_test(pTest, size == 0);
    }

    // cleanup
    DBuffer_Delete(dbuffer);

    return;
}


#ifdef TEST_DBUFFER
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("dbuffer", NULL);

    /* individual tests */
    rc = ct_addTestFunction(pTest, testDBuffer);
    assert(rc);
    rc = ct_addTestFunction(pTest, testDBufferExercise);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);

    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_KEYLIST */
#endif                          /* TEST */
