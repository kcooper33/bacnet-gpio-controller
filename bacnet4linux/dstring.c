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

// Dynamic String Library

// we are using asprintf from GNU
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "dstring.h"

static struct DString *DStringCreate(void)
{
    return calloc(1, sizeof(struct DString));
}

OS_DString DString_Create(void)
{
    return DStringCreate();
}

size_t DString_Printf(OS_DString dstring, const char *fmt, ...)
{
    va_list ap;
    size_t size = 0;            // return value
    int num = 0;                // what vasprintf returns

    if (dstring) {
        // destroy old stuff
        if (dstring->data)
            free(dstring->data);
        dstring->size = 0;

        // create new stuff
        va_start(ap, fmt);
        num = vasprintf(&dstring->data, fmt, ap);
        va_end(ap);

        if (num == -1)
            dstring->data = NULL;
        else
            dstring->size = num;
        // configure the return value
        size = dstring->size;
    }

    return size;
}

size_t DString_Concat(OS_DString dstring, const char *data)
{
    size_t size = 0;
    char *concat_data;

    int num = 0;

    if (dstring) {
        if (data) {
            if (dstring->data) {
                num = asprintf(&concat_data, "%s%s", dstring->data, data);
                if (num != -1) {
                    free(dstring->data);
                    dstring->data = concat_data;
                    dstring->size = num;
                }
            } else {
                num = asprintf(&dstring->data, data);
                if (num == -1)
                    dstring->data = NULL;
                else
                    dstring->size = num;
            }
        }
        // configure the return value
        size = dstring->size;
    }

    return size;
}

size_t DString_Copy(OS_DString dstring, const char *data)
{
    size_t size = 0;            // return value
    int num = 0;

    if (dstring) {
        // destroy old stuff
        if (dstring->data)
            free(dstring->data);
        dstring->size = 0;

        if (data) {
            // create new stuff
            num = asprintf(&dstring->data, data);
            if (num == -1)
                dstring->data = NULL;
            else
                dstring->size = num;
        } else
            dstring->data = NULL;

        // configure the return value
        size = dstring->size;
    }

    return size;
}

size_t DString_Append_Printf(OS_DString dstring, const char *fmt, ...)
{
    va_list ap;
    size_t size = 0;            // return value
    int num = 0;                // what vasprintf returns
    char *data;                 // data added
    char *concat_data;          // both existing data and additional data

    if (dstring) {
        // create new stuff
        va_start(ap, fmt);
        num = vasprintf(&data, fmt, ap);
        va_end(ap);
        // success?
        if (num == -1)
            data = NULL;
        else
            size = num;
        // add it to the existing string
        if (data) {
            if (dstring->data) {
                num = asprintf(&concat_data, "%s%s", dstring->data, data);
                if (num != -1) {
                    free(dstring->data);
                    dstring->data = concat_data;
                    dstring->size = num;
                }
            } else {
                num = asprintf(&dstring->data, data);
                if (num == -1)
                    dstring->data = NULL;
                else
                    dstring->size = num;
            }
            free(data);
        }
        // configure the return value
        size = dstring->size;
    }

    return size;
}

size_t DString_Prefix_Printf(OS_DString dstring, const char *fmt, ...)
{
    va_list ap;
    size_t size = 0;            // return value
    int num = 0;                // what vasprintf returns
    char *data;                 // data added
    char *concat_data;          // both existing data and additional data

    if (dstring) {
        // create new stuff
        va_start(ap, fmt);
        num = vasprintf(&data, fmt, ap);
        va_end(ap);
        // success?
        if (num == -1)
            data = NULL;
        else
            size = num;
        // pre-pend it to the existing string
        if (data) {
            if (dstring->data) {
                num = asprintf(&concat_data, "%s%s", data, dstring->data);
                if (num != -1) {
                    free(dstring->data);
                    dstring->data = concat_data;
                    dstring->size = num;
                }
            } else {
                num = asprintf(&dstring->data, data);
                if (num == -1)
                    dstring->data = NULL;
                else
                    dstring->size = num;
            }
            free(data);
        }
        // configure the return value
        size = dstring->size;
    }

    return size;
}

char *DString_Data(OS_DString dstring)
{
    char *data = NULL;

    if (dstring)
        data = dstring->data;

    return data;
}

size_t DString_Length(OS_DString dstring)
{
    size_t size = 0;

    if (dstring)
        size = dstring->size;

    return size;
}

void DString_Delete(OS_DString dstring)
{
    if (dstring) {
        dstring->size = 0;
        if (dstring->data) {
            free(dstring->data);
            dstring->data = NULL;
        }
        free(dstring);
    }

    return;
}

#ifdef TEST
#include <assert.h>
#include <string.h>

#include "ctest.h"

void testDString(Test * pTest)
{
    OS_DString dstring;
    char *data1 = "Joshua";
    char *data2 = "Anna";
    char *data3 = "Rose";
    char *data_concat = "AnnaRose";
    char *data;
    int number = 42;
    char *data_number = "42";
    int number2 = 999;
    char *data_number2 = "999";
    char *data_number_concat = "42999";
    size_t len;
    size_t len_data;

    // null parameter function check
    data = DString_Data(NULL);
    ct_test(pTest, data == NULL);
    len = DString_Length(NULL);
    ct_test(pTest, len == 0);
    len = DString_Copy(NULL, NULL);
    ct_test(pTest, len == 0);
    len = DString_Printf(NULL, NULL);
    ct_test(pTest, len == 0);
    len = DString_Concat(NULL, NULL);
    ct_test(pTest, len == 0);

    // create
    dstring = DString_Create();
    // verify creation
    ct_test(pTest, dstring != NULL);
    // verify initialization
    data = DString_Data(dstring);
    ct_test(pTest, data == NULL);
    len = DString_Length(dstring);
    ct_test(pTest, len == 0);

    // copy
    len = DString_Copy(dstring, data1);
    len_data = strlen(data1);
    data = DString_Data(dstring);
    ct_test(pTest, strcmp(data, data1) == 0);
    ct_test(pTest, len == len_data);
    len = DString_Length(dstring);
    ct_test(pTest, len == len_data);

    // delete
    len_data = 0;
    len = DString_Copy(dstring, NULL);
    ct_test(pTest, len == len_data);
    data = DString_Data(dstring);
    ct_test(pTest, data == NULL);
    len = DString_Length(dstring);
    ct_test(pTest, len == 0);

    // printf
    len = DString_Printf(dstring, "%d", number);
    len_data = strlen(data_number);
    ct_test(pTest, len == len_data);
    data = DString_Data(dstring);
    ct_test(pTest, strcmp(data, data_number) == 0);
    len = DString_Length(dstring);
    ct_test(pTest, len == len_data);

    // strcat - null check
    len = DString_Concat(dstring, NULL);        // nothing should change
    ct_test(pTest, len == len_data);
    data = DString_Data(dstring);
    ct_test(pTest, strcmp(data, data_number) == 0);
    len = DString_Length(dstring);
    ct_test(pTest, len == len_data);
    // strcat
    len = DString_Copy(dstring, NULL);  // start clean
    len = DString_Concat(dstring, data2);       // concat onto blank string
    len_data = strlen(data2);
    data = DString_Data(dstring);
    ct_test(pTest, strcmp(data, data2) == 0);
    ct_test(pTest, len == len_data);
    len = DString_Length(dstring);
    ct_test(pTest, len == len_data);
    len = DString_Concat(dstring, data3);       // append second string
    len_data = strlen(data_concat);
    ct_test(pTest, len == len_data);
    data = DString_Data(dstring);
    ct_test(pTest, strcmp(data, data_concat) == 0);
    len = DString_Length(dstring);
    ct_test(pTest, len == len_data);

    len = DString_Copy(dstring, NULL);  // start clean
    len = DString_Append_Printf(dstring, "%d", number);
    len_data = strlen(data_number);
    ct_test(pTest, len == len_data);
    data = DString_Data(dstring);
    ct_test(pTest, strcmp(data, data_number) == 0);
    len = DString_Length(dstring);
    ct_test(pTest, len == len_data);
    // append a second number
    len = DString_Append_Printf(dstring, "%d", number2);
    len_data = strlen(data_number) + strlen(data_number2);
    ct_test(pTest, len == len_data);
    len = DString_Length(dstring);
    ct_test(pTest, len == len_data);
    data = DString_Data(dstring);
    ct_test(pTest, strcmp(data, data_number_concat) == 0);

    len = DString_Copy(dstring, NULL);  // start clean
    len = DString_Prefix_Printf(dstring, "%d", number2);
    len_data = strlen(data_number2);
    ct_test(pTest, len == len_data);
    data = DString_Data(dstring);
    ct_test(pTest, strcmp(data, data_number2) == 0);
    len = DString_Length(dstring);
    ct_test(pTest, len == len_data);
    // pre-pend a second number
    len = DString_Prefix_Printf(dstring, "%d", number);
    len_data = strlen(data_number2) + strlen(data_number);
    ct_test(pTest, len == len_data);
    len = DString_Length(dstring);
    ct_test(pTest, len == len_data);
    data = DString_Data(dstring);
    ct_test(pTest, strcmp(data, data_number_concat) == 0);

    // cleanup
    DString_Delete(dstring);

    return;
}

#ifdef TEST_DSTRING
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("dstring", NULL);

    /* individual tests */
    rc = ct_addTestFunction(pTest, testDString);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);

    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_KEYLIST */
#endif                          /* TEST */
