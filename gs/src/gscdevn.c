/* Copyright (C) 1997, 1998 Aladdin Enterprises.  All rights reserved.

   This file is part of Aladdin Ghostscript.

   Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
   or distributor accepts any responsibility for the consequences of using it,
   or for whether it serves any particular purpose or works at all, unless he
   or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
   License (the "License") for full details.

   Every copy of Aladdin Ghostscript must include a copy of the License,
   normally in a plain ASCII text file named PUBLIC.  The License grants you
   the right to copy, modify and redistribute Aladdin Ghostscript, but only
   under certain conditions described in the License.  Among other things, the
   License requires that the copyright notice and this notice be preserved on
   all copies.
 */

/*Id: gscdevn.c  */
/* DeviceN color space and operation definition */
#include "gx.h"
#include "gserrors.h"
#include "gsrefct.h"
#include "gsmatrix.h"		/* for gscolor2.h */
#include "gxcspace.h"

gs_private_st_composite(st_color_space_DeviceN, gs_paint_color_space,
     "gs_color_space_DeviceN", cs_DeviceN_enum_ptrs, cs_DeviceN_reloc_ptrs);

/* Define the DeviceN color space type. */
cs_declare_procs(private, gx_concretize_DeviceN, gx_install_DeviceN,
		 gx_adjust_cspace_DeviceN);
private cs_proc_num_components(gx_num_components_DeviceN);
private cs_proc_base_space(gx_alt_space_DeviceN);
private cs_proc_restrict_color(gx_restrict_DeviceN);
private cs_proc_concrete_space(gx_concrete_space_DeviceN);
private cs_proc_remap_concrete_color(gx_remap_concrete_DeviceN);
private cs_proc_init_color(gx_init_DeviceN);
const gs_color_space_type gs_color_space_type_DeviceN = {
    gs_color_space_index_DeviceN, true, false,
    &st_color_space_DeviceN, gx_num_components_DeviceN,
    gx_alt_space_DeviceN,
    gx_init_DeviceN, gx_restrict_DeviceN,
    gx_concrete_space_DeviceN,
    gx_concretize_DeviceN, gx_remap_concrete_DeviceN,
    gx_default_remap_color, gx_install_DeviceN,
    gx_adjust_cspace_DeviceN, gx_no_adjust_color_count
};

/* ------ Internal routines ------ */

/* Return the number of components of a DeviceN space. */
private int
gx_num_components_DeviceN(const gs_color_space * pcs)
{
    return pcs->params.device_n.num_components;
}

/* Return the alternate space of a DeviceN space. */
private const gs_color_space *
gx_alt_space_DeviceN(const gs_color_space * pcs)
{
    return (const gs_color_space *)&(pcs->params.device_n.alt_space);
}

/* Initialize a DeviceN color. */
/****** DOESN'T WORK IF num_components > 4 ******/
private void
gx_init_DeviceN(gs_client_color * pcc, const gs_color_space * pcs)
{
    int i;

    for (i = 0; i < pcs->params.device_n.num_components; ++i)
	pcc->paint.values[i] = 1.0;
}

/* Force a DeviceN color into legal range. */
private void
gx_restrict_DeviceN(gs_client_color * pcc, const gs_color_space * pcs)
{
    int i;

    for (i = 0; i < pcs->params.device_n.num_components; ++i) {
	floatp value = pcc->paint.values[i];

	pcc->paint.values[i] = (value <= 0 ? 0 : value >= 1 ? 1 : value);
    }
}

/* Remap a DeviceN color. */
private const gs_color_space *
gx_concrete_space_DeviceN(const gs_color_space * pcs,
			  const gs_imager_state * pis)
{				/* We don't support concrete DeviceN spaces yet. */
    const gs_color_space *pacs =
    (const gs_color_space *)&pcs->params.device_n.alt_space;

    return cs_concrete_space(pacs, pis);
}

private int
gx_concretize_DeviceN(const gs_client_color * pc, const gs_color_space * pcs,
		      frac * pconc, const gs_imager_state * pis)
{
    int code;
    gs_client_color cc;
    const gs_color_space *pacs =
    (const gs_color_space *)&pcs->params.device_n.alt_space;

    /* We always map into the alternate color space. */
    code = (*pcs->params.device_n.tint_transform)
	(&pcs->params.device_n, pc->paint.values, &cc.paint.values[0],
	 pcs->params.device_n.tint_transform_data);
    if (code < 0)
	return code;
    return (*pacs->type->concretize_color) (&cc, pacs, pconc, pis);
}

private int
gx_remap_concrete_DeviceN(const frac * pconc,
	gx_device_color * pdc, const gs_imager_state * pis, gx_device * dev,
			  gs_color_select_t select)
{				/* We don't support concrete DeviceN colors yet. */
    return_error(gs_error_rangecheck);
}

/* Install a DeviceN color space. */
private int
gx_install_DeviceN(gs_color_space * pcs, gs_state * pgs)
{				/*
				 * Give an error if any of the separation names are duplicated.
				 * We can't check this any earlier.
				 */
    const gs_separation_name *names = pcs->params.device_n.names;
    uint i, j;

    for (i = 1; i < pcs->params.device_n.num_components; ++i)
	for (j = 0; j < i; ++j)
	    if (names[i] == names[j])
		return_error(gs_error_rangecheck);
    return (*pcs->params.device_n.alt_space.type->install_cspace)
	((gs_color_space *) & pcs->params.device_n.alt_space, pgs);
}

/* Adjust the reference count of a DeviceN color space. */
private void
gx_adjust_cspace_DeviceN(const gs_color_space * pcs, gs_memory_t * mem,
			 int delta)
{
    (*pcs->params.device_n.alt_space.type->adjust_cspace_count)
	((const gs_color_space *)&pcs->params.device_n.alt_space, mem, delta);
}

/* GC procedures */

#define pcs ((gs_color_space *)vptr)

private 
ENUM_PTRS_BEGIN(cs_DeviceN_enum_ptrs)
{
    return ENUM_USING(*pcs->params.device_n.alt_space.type->stype,
		      &pcs->params.device_n.alt_space,
		      sizeof(pcs->params.device_n.alt_space), index - 2);
}
ENUM_PTR(0, gs_color_space, params.device_n.names);
ENUM_PTR(1, gs_color_space, params.device_n.tint_transform_data);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(cs_DeviceN_reloc_ptrs)
{
    RELOC_PTR(gs_color_space, params.device_n.names);
    RELOC_PTR(gs_color_space, params.device_n.tint_transform_data);
    RELOC_USING(*pcs->params.device_n.alt_space.type->stype,
		&pcs->params.device_n.alt_space,
		sizeof(gs_base_color_space));
}
RELOC_PTRS_END

#undef pcs
