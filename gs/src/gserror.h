/* Portions Copyright (C) 2001 artofcode LLC.
   Portions Copyright (C) 1996, 2001 Artifex Software Inc.
   Portions Copyright (C) 1988, 2000 Aladdin Enterprises.
   This software is based in part on the work of the Independent JPEG Group.
   All Rights Reserved.

   This software is distributed under license and may not be copied, modified
   or distributed except as expressly authorized under the terms of that
   license.  Refer to licensing information at http://www.artifex.com/ or
   contact Artifex Software, Inc., 101 Lucas Valley Road #110,
   San Rafael, CA  94903, (415)492-9861, for further information. */

/*$RCSfile$ $Revision$ */
/* Error return macros */

#ifndef gserror_INCLUDED
#  define gserror_INCLUDED

int gs_log_error(const gs_memory_t *mem, int, const char *, int);
#ifndef DEBUG
#  define gs_log_error(mem, err, file, line) (err)
#endif
#define gs_note_error(mem, err) gs_log_error(mem, err, __FILE__, __LINE__)
#define return_error(mem, err) return gs_note_error(mem, err)

#endif /* gserror_INCLUDED */
