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
/* Imager state housekeeping */
#include "gx.h"
#include "gserrors.h"
#include "gscspace.h"
#include "gscie.h"
#include "gsstruct.h"
#include "gsutil.h"		/* for gs_next_ids */
#include "gxbitmap.h"
#include "gxcmap.h"
#include "gxdht.h"
#include "gxistate.h"
#include "gzht.h"
#include "gzline.h"

/******************************************************************************
 * See gsstate.c for a discussion of graphics/imager state memory management. *
 ******************************************************************************/

/* Imported values */
/* The following should include a 'const', but for some reason */
/* the Watcom compiler won't accept it, even though it happily accepts */
/* the same construct everywhere else. */
extern /*const*/ gx_color_map_procs *const cmap_procs_default;

/* GC procedures for gx_line_params */
private
ENUM_PTRS_WITH(line_params_enum_ptrs, gx_line_params *plp) return 0;
    case 0: return ENUM_OBJ((plp->dash.pattern_size == 0 ?
			     NULL : plp->dash.pattern));
ENUM_PTRS_END
private RELOC_PTRS_WITH(line_params_reloc_ptrs, gx_line_params *plp)
{
    if (plp->dash.pattern_size)
	RELOC_VAR(plp->dash.pattern);
} RELOC_PTRS_END
private_st_line_params();

/*
 * GC procedures for gs_imager_state
 *
 * See comments in gixstate.h before the definition of gs_cr_state_do_rc and
 * st_cr_state_num_ptrs for an explanation about why the effective_transfer
 * pointers are handled in this manner.
 */
public_st_imager_state();
private 
ENUM_PTRS_BEGIN(imager_state_enum_ptrs)
    ENUM_SUPER(gs_imager_state, st_line_params, line_params, st_imager_state_num_ptrs - st_line_params_num_ptrs);
    ENUM_PTR(0, gs_imager_state, client_data);
    ENUM_PTR(1, gs_imager_state, opacity.mask);
    ENUM_PTR(2, gs_imager_state, shape.mask);
    ENUM_PTR(3, gs_imager_state, transparency_stack);
#define E1(i,elt) ENUM_PTR(i+4,gs_imager_state,elt);
    gs_cr_state_do_ptrs(E1)
#undef E1
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(imager_state_reloc_ptrs)
{
    RELOC_SUPER(gs_imager_state, st_line_params, line_params);
    RELOC_PTR(gs_imager_state, client_data);
    RELOC_PTR(gs_imager_state, opacity.mask);
    RELOC_PTR(gs_imager_state, shape.mask);
    RELOC_PTR(gs_imager_state, transparency_stack);
#define R1(i,elt) RELOC_PTR(gs_imager_state,elt);
    gs_cr_state_do_ptrs(R1)
#undef R1
    {
        int i = GX_DEVICE_COLOR_MAX_COMPONENTS - 1;

        for (; i >= 0; i--)
            RELOC_PTR(gs_imager_state, effective_transfer[i]);
    }
} RELOC_PTRS_END


/* Initialize an imager state, other than the parts covered by */
/* gs_imager_state_initial. */
private float
imager_null_transfer(floatp gray, const gx_transfer_map * pmap)
{
    return gray;
}

int
gs_imager_state_initialize(gs_imager_state * pis, gs_memory_t * mem)
{
    int i;
    pis->memory = mem;
    pis->client_data = 0;
    pis->opacity.mask = 0;
    pis->shape.mask = 0;
    pis->transparency_stack = 0;
    /* Color rendering state */
    pis->halftone = 0;
    {
	int i;

	for (i = 0; i < gs_color_select_count; ++i)
	    pis->screen_phase[i].x = pis->screen_phase[i].y = 0;
    }
    pis->dev_ht = 0;
    pis->cie_render = 0;
    pis->black_generation = 0;
    pis->undercolor_removal = 0;
    /* Allocate an initial transfer map. */
    rc_alloc_struct_n(pis->set_transfer.gray,
		      gx_transfer_map, &st_transfer_map,
		      mem, return_error(mem, gs_error_VMerror),
		      "gs_imager_state_init(transfer)", 1);
    pis->set_transfer.gray->proc = imager_null_transfer;
    pis->set_transfer.gray->id = gs_next_ids(mem, 1);
    pis->set_transfer.gray->values[0] = frac_0;
    pis->set_transfer.red =
	pis->set_transfer.green =
	pis->set_transfer.blue = NULL; 
    for (i = 0; i < GX_DEVICE_COLOR_MAX_COMPONENTS; i++)
	pis->effective_transfer[i] = pis->set_transfer.gray;
    pis->cie_joint_caches = NULL;
    pis->cmap_procs = cmap_procs_default;
    pis->pattern_cache = NULL;
    pis->have_pattern_streams = false;
    return 0;
}

/*
 * Make a temporary copy of a gs_imager_state.  Note that this does not
 * do all the necessary reference counting, etc.  However, it does
 * clear out the transparency stack in the destination.
 */
gs_imager_state *
gs_imager_state_copy(const gs_imager_state * pis, gs_memory_t * mem)
{
    gs_imager_state *pis_copy =
	gs_alloc_struct(mem, gs_imager_state, &st_imager_state,
			"gs_imager_state_copy");

    if (pis_copy) {
	*pis_copy = *pis;
	pis_copy->transparency_stack = 0;
    }
    return pis_copy;
}

/* Increment reference counts to note that an imager state has been copied. */
void
gs_imager_state_copied(gs_imager_state * pis)
{
    rc_increment(pis->memory, pis->opacity.mask);
    rc_increment(pis->memory, pis->shape.mask);
    rc_increment(pis->memory, pis->halftone);
    rc_increment(pis->memory, pis->dev_ht);
    rc_increment(pis->memory, pis->cie_render);
    rc_increment(pis->memory, pis->black_generation);
    rc_increment(pis->memory, pis->undercolor_removal);
    rc_increment(pis->memory, pis->set_transfer.gray);
    rc_increment(pis->memory, pis->set_transfer.red);
    rc_increment(pis->memory, pis->set_transfer.green);
    rc_increment(pis->memory, pis->set_transfer.blue);
    rc_increment(pis->memory, pis->cie_joint_caches);
}

/* Adjust reference counts before assigning one imager state to another. */
void
gs_imager_state_pre_assign(gs_imager_state *pto, const gs_imager_state *pfrom)
{
    const char *const cname = "gs_imager_state_pre_assign";

#define RCCOPY(mem, element)\
    rc_pre_assign(mem, pto->element, pfrom->element, cname)

    RCCOPY(pto->memory, cie_joint_caches);
    RCCOPY(pto->memory, set_transfer.blue);
    RCCOPY(pto->memory, set_transfer.green);
    RCCOPY(pto->memory, set_transfer.red);
    RCCOPY(pto->memory, set_transfer.gray);
    RCCOPY(pto->memory, undercolor_removal);
    RCCOPY(pto->memory, black_generation);
    RCCOPY(pto->memory, cie_render);
    RCCOPY(pto->memory, dev_ht);
    RCCOPY(pto->memory, halftone);
    RCCOPY(pto->memory, shape.mask);
    RCCOPY(pto->memory, opacity.mask);
#undef RCCOPY
}

/* Release an imager state. */
void
gs_imager_state_release(gs_imager_state * pis)
{
    const char *const cname = "gs_imager_state_release";
    gx_device_halftone *pdht = pis->dev_ht;

#define RCDECR(mem, element)\
    rc_decrement(mem, pis->element, cname)

    RCDECR(pis->memory, cie_joint_caches);
    RCDECR(pis->memory, set_transfer.gray);
    RCDECR(pis->memory, set_transfer.blue);
    RCDECR(pis->memory, set_transfer.green);
    RCDECR(pis->memory, set_transfer.red);
    RCDECR(pis->memory, undercolor_removal);
    RCDECR(pis->memory, black_generation);
    RCDECR(pis->memory, cie_render);
    /*
     * If we're going to free the device halftone, make sure we free the
     * dependent structures as well.
     */
    if (pdht != 0 && pdht->rc.ref_count == 1) {
	gx_device_halftone_release(pdht, pdht->rc.memory);
    }
    RCDECR(pis->memory, dev_ht);
    RCDECR(pis->memory, halftone);
    RCDECR(pis->memory, shape.mask);
    RCDECR(pis->memory, opacity.mask);
#undef RCDECR
}
