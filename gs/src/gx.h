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
/* Common internal definitions for Ghostscript library */

#ifndef gx_INCLUDED
#  define gx_INCLUDED

#include "stdio_.h"		/* includes std.h */
#include "gserror.h"
#include "gsio.h"
#include "gstypes.h"
#include "gsmemory.h"
#include "gdebug.h"

/* Configuration swtches - development needs only. */
/* Users should not modify them. */
#define NEW_TT_INTERPRETER 0
#define TT_GRID_FITTING (NEW_TT_INTERPRETER && 0) /* old code = 0, new code = 1. */
#define CURVED_TRAPEZOID_FILL 1 /* old code = 0, new code = 1. */
/* The following switches are only important with CURVED_TRAPEZOID_FILL 1. */
#   define CURVED_TRAPEZOID_FILL_HEAVY_TEST 0  /* Temporarily used for a debug purpose. */
#   define CURVED_TRAPEZOID_FILL0_COMPATIBLE 1 /* Temporarily used for a backward compatibility. 
                                               The implementation of 0 is not completed yet. */
#   define FLATTENED_ITERATOR_BACKSCAN 1 /* 0 - simpler code, 1 - faster and smaller RAM. */
#   define FLATTENED_ITERATOR_SELFTEST 0 /* Temporarily used for a debug purpose. */
#   define FLATTENED_ITERATOR_HEAVY_SELFTEST 0 /* Temporarily used for a debug purpose. */


/* Define opaque types for the graphics state. */
/* This is used so pervasively that we define it here, */
/* rather than at a higher level as perhaps would be more appropriate. */
#ifndef gs_imager_state_DEFINED
#  define gs_imager_state_DEFINED
typedef struct gs_imager_state_s gs_imager_state;

#endif
#ifndef gs_state_DEFINED
#  define gs_state_DEFINED
typedef struct gs_state_s gs_state;

#endif

#endif /* gx_INCLUDED */
