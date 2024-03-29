/* Copyright (C) 2001-2023 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied,
   modified or distributed except as expressly authorized under the terms
   of the license contained in the file LICENSE in this distribution.

   Refer to licensing information at http://www.artifex.com or contact
   Artifex Software, Inc.,  39 Mesa Street, Suite 108A, San Francisco,
   CA 94129, USA, for further information.
*/


/* plvalue.c */
/* Accessors for big-endian multi-byte values */

#include "std.h"
#include "plvalue.h"

#define get_uint16(bptr)\
  (((bptr)[0] << 8) | (bptr)[1])
#define get_int16(bptr)\
  (((int)get_uint16(bptr) ^ 0x8000) - 0x8000)

/* coverity[ -tainted_data_return ] */
/* coverity[ -tainted_data_argument : arg-0 ] */
int
pl_get_int16(const byte * bptr)
{
    return get_int16(bptr);
}

/* coverity[ -tainted_data_return ] */
/* coverity[ -tainted_data_argument : arg-0 ] */
uint
pl_get_uint16(const byte * bptr)
{
    return get_uint16(bptr);
}

/* coverity[ -tainted_data_return ] */
/* coverity[ -tainted_data_argument : arg-0 ] */
long
pl_get_int32(const byte * bptr)
{
    return ((long)get_int16(bptr) << 16) | get_uint16(bptr + 2);
}

/* coverity[ -tainted_data_return ] */
/* coverity[ -tainted_data_argument : arg-0 ] */
ulong
pl_get_uint32(const byte * bptr)
{
    return ((ulong) get_uint16(bptr) << 16) | get_uint16(bptr + 2);
}
