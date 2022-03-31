/* Copyright (C) 2001-2022 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied,
   modified or distributed except as expressly authorized under the terms
   of the license contained in the file LICENSE in this distribution.

   Refer to licensing information at http://www.artifex.com or contact
   Artifex Software, Inc.,  1305 Grant Avenue - Suite 200, Novato,
   CA 94945, U.S.A., +1(415)492-9861, for further information.
*/

/*  Handling of color spaces stored during replacement of all
 *  text with black.
*/

#include "gsmemory.h"
#include "gsstruct.h"
#include "gzstate.h"
#include "gsicc_blacktext.h"

/* gsicc_blacktextvec_state_t is going to be storing GCed items
   (color spaces and client colors) and so will need to be GCed */
gs_private_st_ptrs4(st_blacktextvec_state, gsicc_blacktextvec_state_t,
    "gsicc_blacktextvec_state", blacktextvec_state_enum_ptrs,
    blacktextvec_state_reloc_ptrs, pcs, pcs_alt, pcc, pcc_alt);

static void
rc_gsicc_blacktextvec_state_free(gs_memory_t *mem, void *ptr_in,
    client_name_t cname)
{
    gsicc_blacktextvec_state_t *state = (gsicc_blacktextvec_state_t*)ptr_in;

    rc_decrement_cs(state->pcs, "rc_gsicc_blacktextvec_state_free");
    rc_decrement_cs(state->pcs_alt, "rc_gsicc_blacktextvec_state_free");

    gs_free_object(state->memory, state,
        "rc_gsicc_blacktextvec_state_free");
}

gsicc_blacktextvec_state_t*
gsicc_blacktextvec_state_new(gs_memory_t *memory, bool is_text)
{
    gsicc_blacktextvec_state_t *result;

    result = gs_alloc_struct(memory->stable_memory, gsicc_blacktextvec_state_t,
        &st_blacktextvec_state, "gsicc_blacktextvec_state_new");
    if (result == NULL)
        return NULL;
    rc_init_free(result, memory->stable_memory, 1, rc_gsicc_blacktextvec_state_free);
    result->memory = memory->stable_memory;
    result->pcs = NULL;
    result->pcs_alt = NULL;
    result->pcc = NULL;
    result->pcc_alt = NULL;
    result->is_text = is_text;

    return result;
}

/* Crude white color check. Only valid for ICC based RGB, CMYK, Gray, and LAB CS.
  Makes some assumptions about profile.  Also may want some tolerance check. */
static inline bool is_white(gs_color_space* pcs, gs_client_color* pcc)
{
    if (gs_color_space_get_index(pcs) == gs_color_space_index_ICC) {
        switch (pcs->cmm_icc_profile_data->data_cs) {
            case gsGRAY:
                if (pcc->paint.values[0] == 1.0)
                    return true;
                else
                    return false;
                break;
            case gsRGB:
                if (pcc->paint.values[0] == 1.0 && pcc->paint.values[1] == 1.0 &&
                    pcc->paint.values[2] == 1.0)
                    return true;
                else
                    return false;
                break;
            case gsCMYK:
                if (pcc->paint.values[0] == 0.0 && pcc->paint.values[1] == 0.0 &&
                    pcc->paint.values[2] == 0.0 && pcc->paint.values[3] == 0.0)
                    return true;
                else
                    return false;
                break;
            case gsCIELAB:
                if (pcc->paint.values[0] == 100.0 && pcc->paint.values[1] == 0.0 &&
                    pcc->paint.values[2] == 0.0)
                    return true;
                else
                    return false;
                break;
            default:
                return false;
        }
    } else
        return false;
}

bool gsicc_setup_black_textvec(gs_gstate *pgs, gx_device *dev, bool is_text)
{
    gs_color_space *pcs_curr = gs_currentcolorspace_inline(pgs);
    gs_color_space *pcs_alt = gs_swappedcolorspace_inline(pgs);

    /* If neither space is ICC then we are not doing anything */
    if (!gs_color_space_is_ICC(pcs_curr) && !gs_color_space_is_ICC(pcs_alt))
        return false;

    /* Create a new object to hold the cs details */
    pgs->black_textvec_state = gsicc_blacktextvec_state_new(pgs->memory, is_text);
    if (pgs->black_textvec_state == NULL)
        return false; /* No error just move on */

    /* If curr space is ICC then store it */
    if  (gs_color_space_is_ICC(pcs_curr)) {
        rc_increment_cs(pcs_curr);  /* We are storing the cs. Will decrement when structure is released */
        pgs->black_textvec_state->pcs = pcs_curr;
        pgs->black_textvec_state->pcc = pgs->color[0].ccolor;
        cs_adjust_color_count(pgs, 1); /* The set_gray will do a decrement, only need if pattern */
        pgs->black_textvec_state->value[0] = pgs->color[0].ccolor->paint.values[0];

        if (is_white(pcs_curr, pgs->color[0].ccolor))
            gs_setgray(pgs, 1.0);
        else
            gs_setgray(pgs, 0.0);
    }

    /* If alt space is ICC then store it */
    if (gs_color_space_is_ICC(pcs_alt)) {
        rc_increment_cs(pcs_alt);  /* We are storing the cs. Will decrement when structure is released */
        pgs->black_textvec_state->pcs_alt = pcs_alt;

        gs_swapcolors_quick(pgs);  /* Have to swap for set_gray and adjust color count */
        pgs->black_textvec_state->pcc_alt = pgs->color[0].ccolor;
        cs_adjust_color_count(pgs, 1); /* The set_gray will do a decrement, only need if pattern */
        pgs->black_textvec_state->value[1] = pgs->color[0].ccolor->paint.values[0];

        if (is_white(pcs_alt, pgs->color[0].ccolor))
            gs_setgray(pgs, 1.0);
        else
            gs_setgray(pgs, 0.0);
        gs_swapcolors_quick(pgs);
    }

    pgs->black_textvec_state->is_fill = pgs->is_fill_color;
    return true; /* Need to clean up */
}

void
gsicc_restore_blacktextvec(gs_gstate *pgs, bool is_text)
{
    gsicc_blacktextvec_state_t *state = pgs->black_textvec_state;
    int code;

    if (state == NULL)
        return;

    if (is_text != state->is_text)
        return;

    /* Make sure state and original are same fill_color condition */
    if (state->rc.ref_count == 1) {
        if ((state->is_fill && pgs->is_fill_color) || (!state->is_fill && !pgs->is_fill_color)) {
            if (pgs->black_textvec_state->pcs != NULL) {
                if ((code = gs_setcolorspace_only(pgs, pgs->black_textvec_state->pcs)) >= 0) {
                    /* current client color is gray.  no need to decrement */
                    pgs->color[0].ccolor = pgs->black_textvec_state->pcc;
                    pgs->color[0].ccolor->paint.values[0] = pgs->black_textvec_state->value[0];
                }
                gx_unset_dev_color(pgs);
            }
            if (pgs->black_textvec_state->pcs_alt != NULL) {
                gs_swapcolors_quick(pgs);
                if ((code = gs_setcolorspace_only(pgs, pgs->black_textvec_state->pcs_alt)) >= 0) {
                    pgs->color[0].ccolor = pgs->black_textvec_state->pcc_alt;
                    pgs->color[0].ccolor->paint.values[0] = pgs->black_textvec_state->value[1];
                }
                gs_swapcolors_quick(pgs);
                gx_unset_alt_dev_color(pgs);
            }
        } else {
            if (pgs->black_textvec_state->pcs_alt != NULL) {
                if ((code = gs_setcolorspace_only(pgs, pgs->black_textvec_state->pcs_alt)) >= 0) {
                    pgs->color[0].ccolor = pgs->black_textvec_state->pcc_alt;
                    pgs->color[0].ccolor->paint.values[0] = pgs->black_textvec_state->value[1];
                }
                gx_unset_dev_color(pgs);
            }
            if (pgs->black_textvec_state->pcs != NULL) {
                gs_swapcolors_quick(pgs);
                if ((code = gs_setcolorspace_only(pgs, pgs->black_textvec_state->pcs)) >= 0) {
                    pgs->color[0].ccolor = pgs->black_textvec_state->pcc;
                    pgs->color[0].ccolor->paint.values[0] = pgs->black_textvec_state->value[0];
                }
                gs_swapcolors_quick(pgs);
                gx_unset_alt_dev_color(pgs);
            }
        }
    }
    rc_decrement(state, "gsicc_restore_black_text");
    pgs->black_textvec_state = NULL;
}