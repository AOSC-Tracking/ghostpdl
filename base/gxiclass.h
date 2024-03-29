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


/* Define image rendering algorithm classes */

#ifndef gxiclass_INCLUDED
#  define gxiclass_INCLUDED

#include "stdpre.h"
#include "gsdevice.h"

/* Define the abstract type for the image enumerator state. */
typedef struct gx_image_enum_s gx_image_enum;

/*
 * Define the interface for routines used to render a (source) scan line.
 * If the buffer is the original client's input data, it may be unaligned;
 * otherwise, it will always be aligned.
 *
 * The image_render procedures work on fully expanded, complete rows.  These
 * take a height argument, which is an integer >= 0; they return a negative
 * code, or the number of rows actually processed (which may be less than
 * the height).  height = 0 is a special call to indicate that there is no
 * more input data; this is necessary because the last scan lines of the
 * source data may not produce any output.
 *
 * Note that the 'w' argument of the image_render procedure is the number
 * of samples, i.e., the number of pixels * the number of samples per pixel.
 * This is neither the width in pixels nor the width in bytes (in the case
 * of 12-bit samples, which expand to 2 bytes apiece).
 */
#define irender_proc(proc)\
  int proc(gx_image_enum *penum, const byte *buffer, int data_x,\
           uint w, int h, gx_device *dev)
typedef irender_proc((*irender_proc_t));

/*
 * Define procedures for selecting imaging methods according to the class of
 * the image.  Image class procedures are called in alphabetical order, so
 * their names begin with a digit that indicates their priority
 * (0_interpolate, etc.): each one may assume that all the previous ones
 * failed.  If a class procedure succeeds, it may update the enumerator
 * structure as well as returning the rendering procedure.
 */
#define iclass_proc(proc)\
  int proc(gx_image_enum *penum, irender_proc_t *render_fn)
typedef iclass_proc((*gx_image_class_t));

#endif /* gxiclass_INCLUDED */
