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
/* Functions for managing the device filter stack */

#include "ctype_.h"
#include "memory_.h"		/* for memchr, memcpy */
#include "string_.h"
#include "gx.h"
#include "gp.h"
#include "gscdefs.h"		/* for gs_lib_device_list */
#include "gserrors.h"
#include "gsfname.h"
#include "gsstruct.h"
#include "gspath.h"		/* gs_initclip prototype */
#include "gspaint.h"		/* gs_erasepage prototype */
#include "gsmatrix.h"		/* for gscoord.h */
#include "gscoord.h"		/* for gs_initmatrix */
#include "gzstate.h"
#include "gxcmap.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gxiodev.h"

#include "gsdfilt.h"

struct gs_device_filter_stack_s {
    gs_device_filter_stack_t *next;
    gs_device_filter_t *df;
    gx_device *next_device;
};

gs_private_st_ptrs3(st_gs_device_filter_stack, gs_device_filter_stack_t,
		    "gs_device_filter_stack",
		    gs_device_filter_stack_enum_ptrs,
		    gs_device_filter_stack_reloc_ptrs,
		    next, df, next_device);

gs_public_st_simple(st_gs_device_filter, gs_device_filter_t,
		    "gs_device_filter");

int
gs_push_device_filter(gs_memory_t *mem, gs_state *pgs, gs_device_filter_t *df)
{
    gs_device_filter_stack_t *dfs;
    gx_device *new_dev = NULL;
    int code;

    dfs = gs_alloc_struct(mem, gs_device_filter_stack_t,
			  &st_gs_device_filter_stack, "gs_push_device_filter");
    if (dfs == NULL)
	return_error(mem, gs_error_VMerror);
    rc_increment(pgs->memory, pgs->device);
    dfs->next_device = pgs->device;
    code = df->push(df, mem, pgs, &new_dev, pgs->device);
    if (code < 0) {
	return code;
	gs_free_object(mem, dfs, "gs_push_device_filter");
    }
    dfs->next = pgs->dfilter_stack;
    pgs->dfilter_stack = dfs;
    dfs->df = df;
    gs_setdevice_no_init(pgs, new_dev);
    rc_decrement_only(pgs->memory, new_dev, "gs_push_device_filter");
    return code;
}

int
gs_pop_device_filter(gs_memory_t *mem, gs_state *pgs)
{
    gs_device_filter_stack_t *dfs_tos = pgs->dfilter_stack;
    gx_device *tos_device = pgs->device;
    gs_device_filter_t *df;
    int code;

    if (dfs_tos == NULL)
	return_error(mem, gs_error_rangecheck);
    df = dfs_tos->df;
    pgs->dfilter_stack = dfs_tos->next;
    code = df->prepop(df, mem, pgs, tos_device);
    rc_increment(mem, tos_device);
    gs_setdevice_no_init(pgs, dfs_tos->next_device);
    rc_decrement_only(mem, dfs_tos->next_device, "gs_pop_device_filter");
    gs_free_object(mem, dfs_tos, "gs_pop_device_filter");
    code = df->postpop(df, mem, pgs, tos_device);
    rc_decrement_only(mem, tos_device, "gs_pop_device_filter");
    return code;
}

int
gs_clear_device_filters(gs_memory_t *mem, gs_state *pgs)
{
    int code;

    while (pgs->dfilter_stack != NULL) {
	if ((code = gs_pop_device_filter(mem, pgs)) < 0)
	    return code;
    }
    return 0;
}
