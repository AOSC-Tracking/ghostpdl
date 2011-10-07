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
/*$Id$ */

/* pldraw.c */
/* Common drawing routines for PCL5 and PCL XL */
#include "std.h"
#include "gstypes.h"
#include "gserrors.h"
#include "gsmemory.h"
#include "gxdevice.h"
#include "gzstate.h"
#include "gsimage.h"
#include "pldraw.h"

/* the next 3 procedures use an obsolete image api in the
   graphics library.  Clients should use the definition after these
   3. */

/* Begin an image with parameters derived from a graphics state. */
int
pl_begin_image(gs_state *pgs, const gs_image_t *pim,
  void **pinfo)
{	
    gx_device *dev = pgs->device;

    if ( pim->ImageMask | pim->CombineWithColor ) {
        int code = gx_set_dev_color(pgs);
        if (code != 0)
            return code;
    }
    return (*dev_proc(dev, begin_image))
        (dev, (const gs_imager_state *)pgs, pim,
         gs_image_format_chunky, (const gs_int_rect *)0,
         gs_currentdevicecolor_inline(pgs), pgs->clip_path, pgs->memory,
         (gx_image_enum_common_t **)pinfo);
}

int
pl_image_data(gs_state *pgs, void *info, const byte **planes,
              int data_x, uint raster, int height)
{
    gx_device *dev = pgs->device;
    return (*dev_proc(dev, image_data))
        (dev, info, planes, data_x, raster, height);
}

int
pl_end_image(gs_state *pgs, void *info, bool draw_last)
{
    gx_device *dev = pgs->device;
    return (*dev_proc(dev, end_image))(dev, info, draw_last);
}

int
pl_begin_image2(gs_image_enum **ppenum, gs_image_t *pimage, gs_state *pgs)
{
    *ppenum = gs_image_enum_alloc(gs_state_memory(pgs), "px_paint_pattern");
    if (*ppenum == 0)
        return_error(gs_error_VMerror);
    
    return gs_image_init(*ppenum, pimage, 0, pgs);
}

int
pl_image_data2(gs_image_enum *penum, const byte *row, uint size, uint *pused)
{
    return gs_image_next(penum, row, size, pused);
}

int
pl_end_image2(gs_image_enum *penum, gs_state *pgs)
{
    return gs_image_cleanup_and_free_enum(penum, pgs);
}
