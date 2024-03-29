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


/* Composite font operations for Ghostscript library */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxfixed.h"
#include "gsmatrix.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gxfcache.h"		/* gs_font_dir */
#include "gxfont.h"
#include "gxfont0.h"

/* Structure descriptor */
static struct_proc_enum_ptrs(font_type0_enum_ptrs);
static struct_proc_reloc_ptrs(font_type0_reloc_ptrs);

public_st_gs_font_type0();
static
ENUM_PTRS_WITH(font_type0_enum_ptrs, gs_font_type0 *pfont)
ENUM_PREFIX(st_gs_font, gs_type0_data_max_ptrs);
ENUM_PTR(0, gs_font_type0, data.Encoding);
ENUM_PTR(1, gs_font_type0, data.FDepVector);
case 2:
switch (pfont->data.FMapType)
{
    case fmap_SubsVector:
        ENUM_RETURN_CONST_STRING_PTR(gs_font_type0,
                                     data.SubsVector);
    case fmap_CMap:
        ENUM_RETURN_PTR(gs_font_type0, data.CMap);
    default:
        ENUM_RETURN(0);
}
ENUM_PTRS_END
static RELOC_PTRS_WITH(font_type0_reloc_ptrs, gs_font_type0 *pfont)
RELOC_PREFIX(st_gs_font);
RELOC_PTR(gs_font_type0, data.Encoding);
RELOC_PTR(gs_font_type0, data.FDepVector);
switch (pfont->data.FMapType)
{
    case fmap_SubsVector:
RELOC_CONST_STRING_PTR(gs_font_type0, data.SubsVector);
break;
    case fmap_CMap:
RELOC_PTR(gs_font_type0, data.CMap);
break;
    default:
;
}
RELOC_PTRS_END

/* Adjust a composite font by concatenating a given matrix */
/* to the FontMatrix of all descendant composite fonts. */
static int
gs_type0_adjust_matrix(gs_font_dir * pdir, gs_font_type0 * pfont,
                       const gs_matrix * pmat)
{
    gs_font **pdep = pfont->data.FDepVector;
    uint fdep_size = pfont->data.fdep_size;
    gs_font **ptdep;
    uint i;

    /* Check for any descendant composite fonts. */
    for (i = 0; i < fdep_size; i++)
        if (pdep[i]->FontType == ft_composite)
            break;
    if (i == fdep_size)
        return 0;
    ptdep = gs_alloc_struct_array(pfont->memory, fdep_size, gs_font *,
                                  &st_gs_font_ptr_element,
                                  "gs_type0_adjust_font(FDepVector)");
    if (ptdep == 0)
        return_error(gs_error_VMerror);
    memcpy(ptdep, pdep, sizeof(gs_font *) * fdep_size);
    for (; i < fdep_size; i++)
        if (pdep[i]->FontType == ft_composite) {
            int code = gs_makefont(pdir, pdep[i], pmat, &ptdep[i]);

            if (code < 0)
                return code;
        }
    pfont->data.FDepVector = ptdep;
    return 0;
}

/* Finish defining a composite font, */
/* by adjusting its descendants' FontMatrices. */
int
gs_type0_define_font(gs_font_dir * pdir, gs_font * pfont)
{
    const gs_matrix *pmat = &pfont->FontMatrix;

    /* Check for the identity matrix, which is common in root fonts. */
    if (pmat->xx == 1.0 && pmat->yy == 1.0 &&
        pmat->xy == 0.0 && pmat->yx == 0.0 &&
        pmat->tx == 0.0 && pmat->ty == 0.0
        )
        return 0;
    return gs_type0_adjust_matrix(pdir, (gs_font_type0 *) pfont, pmat);
}

/* Finish scaling a composite font similarly. */
int
gs_type0_make_font(gs_font_dir * pdir, const gs_font * pfont,
                   const gs_matrix * pmat, gs_font ** ppfont)
{
    return gs_type0_adjust_matrix(pdir, (gs_font_type0 *) * ppfont, pmat);
}
