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

/* Declaration of the O_* flags for open */

#ifndef fcntl__INCLUDED
#  define fcntl__INCLUDED

/*
 * This absurd little file is needed because Microsoft C, in defiance
 * of multiple standards, does not define the O_ modes for 'open'.
 */

/*
 * We must include std.h before any file that includes (or might include)
 * sys/types.h.
 */
#include "std.h"
#include <fcntl.h>

#if !defined(O_APPEND) && defined(_O_APPEND)
#  define O_APPEND _O_APPEND
#endif
#if !defined(O_BINARY) && defined(_O_BINARY)
#  define O_BINARY _O_BINARY
#endif
#if !defined(O_CREAT) && defined(_O_CREAT)
#  define O_CREAT _O_CREAT
#endif
#if !defined(O_EXCL) && defined(_O_EXCL)
#  define O_EXCL _O_EXCL
#endif
#if !defined(O_RDONLY) && defined(_O_RDONLY)
#  define O_RDONLY _O_RDONLY
#endif
#if !defined(O_RDWR) && defined(_O_RDWR)
#  define O_RDWR _O_RDWR
#endif
#if !defined(O_TRUNC) && defined(_O_TRUNC)
#  define O_TRUNC _O_TRUNC
#endif
#if !defined(O_WRONLY) && defined(_O_WRONLY)
#  define O_WRONLY _O_WRONLY
#endif

#endif /* fcntl__INCLUDED */
