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
/* Level 1 extended color operators */
#include "ghost.h"
#include "oper.h"
#include "estack.h"
#include "ialloc.h"
#include "igstate.h"
#include "iutil.h"
#include "store.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gzstate.h"
#include "gxdevice.h"
#include "gxcmap.h"
#include "gscolor1.h"
#include "gxcspace.h"
#include "icolor.h"
#include "iimage.h"

/* - currentblackgeneration <proc> */
private int
zcurrentblackgeneration(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(imemory, 1);
    *op = istate->black_generation;
    return 0;
}

/* - currentcolortransfer <redproc> <greenproc> <blueproc> <grayproc> */
private int
zcurrentcolortransfer(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(imemory, 4);
    op[-3] = istate->transfer_procs.red;
    op[-2] = istate->transfer_procs.green;
    op[-1] = istate->transfer_procs.blue;
    *op = istate->transfer_procs.gray;
    return 0;
}

/* - currentundercolorremoval <proc> */
private int
zcurrentundercolorremoval(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(imemory, 1);
    *op = istate->undercolor_removal;
    return 0;
}

/* <proc> setblackgeneration - */
private int
zsetblackgeneration(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;

    check_proc(imemory, *op);
    check_ostack(imemory, zcolor_remap_one_ostack - 1);
    check_estack(1 + zcolor_remap_one_estack);
    code = gs_setblackgeneration_remap(igs, gs_mapped_transfer, false);
    if (code < 0)
	return code;
    istate->black_generation = *op;
    pop(1);
    push_op_estack(zcolor_remap_color);
    return zcolor_remap_one(i_ctx_p, &istate->black_generation,
			    igs->black_generation, igs,
			    zcolor_remap_one_finish);
}

/* <redproc> <greenproc> <blueproc> <grayproc> setcolortransfer - */
private int
zsetcolortransfer(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;

    check_proc(imemory, op[-3]);
    check_proc(imemory, op[-2]);
    check_proc(imemory, op[-1]);
    check_proc(imemory, *op);
    check_ostack(imemory, zcolor_remap_one_ostack * 4 - 4);
    check_estack(1 + zcolor_remap_one_estack * 4);
    istate->transfer_procs.red = op[-3];
    istate->transfer_procs.green = op[-2];
    istate->transfer_procs.blue = op[-1];
    istate->transfer_procs.gray = *op;
    if ((code = gs_setcolortransfer_remap(igs,
				     gs_mapped_transfer, gs_mapped_transfer,
				     gs_mapped_transfer, gs_mapped_transfer,
					  false)) < 0
	)
	return code;
    /* Use osp rather than op here, because zcolor_remap_one pushes. */
    pop(4);
    push_op_estack(zcolor_reset_transfer);
    if ((code = zcolor_remap_one(i_ctx_p,
				 &istate->transfer_procs.red,
				 igs->set_transfer.red, igs,
				 zcolor_remap_one_finish)) < 0 ||
	(code = zcolor_remap_one(i_ctx_p,
				 &istate->transfer_procs.green,
				 igs->set_transfer.green, igs,
				 zcolor_remap_one_finish)) < 0 ||
	(code = zcolor_remap_one(i_ctx_p,
				 &istate->transfer_procs.blue,
				 igs->set_transfer.blue, igs,
				 zcolor_remap_one_finish)) < 0 ||
	(code = zcolor_remap_one(i_ctx_p, &istate->transfer_procs.gray,
				 igs->set_transfer.gray, igs,
				 zcolor_remap_one_finish)) < 0
	)
	return code;
    return o_push_estack;
}

/* <proc> setundercolorremoval - */
private int
zsetundercolorremoval(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;

    check_proc(imemory, *op);
    check_ostack(imemory, zcolor_remap_one_ostack - 1);
    check_estack(1 + zcolor_remap_one_estack);
    code = gs_setundercolorremoval_remap(igs, gs_mapped_transfer, false);
    if (code < 0)
	return code;
    istate->undercolor_removal = *op;
    pop(1);
    push_op_estack(zcolor_remap_color);
    return zcolor_remap_one(i_ctx_p, &istate->undercolor_removal,
			    igs->undercolor_removal, igs,
			    zcolor_remap_one_signed_finish);
}


/* ------ Initialization procedure ------ */

const op_def zcolor1_op_defs[] =
{
    {"0currentblackgeneration", zcurrentblackgeneration},
    {"0currentcolortransfer", zcurrentcolortransfer},
    {"0currentundercolorremoval", zcurrentundercolorremoval},
    {"1setblackgeneration", zsetblackgeneration},
    {"4setcolortransfer", zsetcolortransfer},
    {"1setundercolorremoval", zsetundercolorremoval},
    op_def_end(0)
};
