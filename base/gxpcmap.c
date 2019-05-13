/* Copyright (C) 2001-2019 Artifex Software, Inc.
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


/* Pattern color mapping for Ghostscript library */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gp.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsutil.h"             /* for gs_next_ids */
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gspath2.h"
#include "gxcspace.h"           /* for gscolor2.h */
#include "gxcolor2.h"
#include "gxdcolor.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gxpcolor.h"
#include "gxp1impl.h"
#include "gxclist.h"
#include "gxcldev.h"
#include "gzstate.h"
#include "gxdevsop.h"
#include "gdevmpla.h"
#include "gdevp14.h"
#include "gxgetbit.h"
#include "gscoord.h"

#if RAW_PATTERN_DUMP
unsigned int global_pat_index = 0;
#endif

/* Define the default size of the Pattern cache. */
#define max_cached_patterns_LARGE 50
#define max_pattern_bits_LARGE 100000
#define max_cached_patterns_SMALL 5
#define max_pattern_bits_SMALL 1000
uint
gx_pat_cache_default_tiles(void)
{
#if ARCH_SMALL_MEMORY
    return max_cached_patterns_SMALL;
#else
#ifdef DEBUG
    return (gs_debug_c('.') ? max_cached_patterns_SMALL :
            max_cached_patterns_LARGE);
#else
    return max_cached_patterns_LARGE;
#endif
#endif
}
ulong
gx_pat_cache_default_bits(void)
{
#if ARCH_SMALL_MEMORY
    return max_pattern_bits_SMALL;
#else
#ifdef DEBUG
    return (gs_debug_c('.') ? max_pattern_bits_SMALL :
            max_pattern_bits_LARGE);
#else
    return max_pattern_bits_LARGE;
#endif
#endif
}

/* Define the structures for Pattern rendering and caching. */
private_st_color_tile();
private_st_color_tile_element();
private_st_pattern_cache();
private_st_device_pattern_accum();
private_st_pattern_trans();

/* ------ Pattern rendering ------ */

/* Device procedures */
static dev_proc_open_device(pattern_accum_open);
static dev_proc_close_device(pattern_accum_close);
static dev_proc_fill_rectangle(pattern_accum_fill_rectangle);
static dev_proc_copy_mono(pattern_accum_copy_mono);
static dev_proc_copy_color(pattern_accum_copy_color);
static dev_proc_copy_planes(pattern_accum_copy_planes);
static dev_proc_get_bits_rectangle(pattern_accum_get_bits_rectangle);
static dev_proc_fill_rectangle_hl_color(pattern_accum_fill_rectangle_hl_color);
/* not static for use by clist_dev_spec_op with pattern-clist */
dev_proc_dev_spec_op(pattern_accum_dev_spec_op);

/* The device descriptor */
static const gx_device_pattern_accum gs_pattern_accum_device =
{std_device_std_body_type_open(gx_device_pattern_accum, 0,
                          "pattern accumulator", &st_device_pattern_accum,
                          0, 0, 72, 72),
 {
     /* NOTE: all drawing procedures must be defaulted, not forwarded. */
     pattern_accum_open,
     NULL,                              /* get_initial_matrix */
     NULL,                              /* sync_output */
     NULL,                              /* output_page */
     pattern_accum_close,
     NULL,                              /* map_rgb_color */
     NULL,                              /* map_color_rgb */
     pattern_accum_fill_rectangle,
     gx_default_tile_rectangle,
     pattern_accum_copy_mono,
     pattern_accum_copy_color,
     NULL,                              /* obselete_draw_line */
     gx_default_get_bits,
     NULL,                              /* get_params */
     NULL,                              /* put_params */
     NULL,                              /* map_cmyk_color */
     NULL,                              /* get_xfont_procs */
     NULL,                              /* get_xfont_device */
     NULL,                              /* map_rgb_alpha_color */
     NULL,                              /* get_page_device */
     NULL,                              /* get_alpha_bits */
     gx_default_copy_alpha,
     NULL,                              /* get_band */
     gx_default_copy_rop,
     gx_default_fill_path,
     gx_default_stroke_path,
     gx_default_fill_mask,
     gx_default_fill_trapezoid,
     gx_default_fill_parallelogram,
     gx_default_fill_triangle,
     gx_default_draw_thin_line,
     gx_default_begin_image,
     gx_default_image_data,
     gx_default_end_image,
     gx_default_strip_tile_rectangle,
     gx_default_strip_copy_rop,
     gx_get_largest_clipping_box,
     gx_default_begin_typed_image,
     pattern_accum_get_bits_rectangle,
     NULL,                              /* map_color_rgb_alpha */
     gx_default_create_compositor,
     NULL,                              /* create_compositor */
     gx_default_text_begin,
     gx_default_finish_copydevice,
     NULL,                              /* begin_transparency_group */
     NULL,                              /* end_transparency_group */
     NULL,                              /* begin_transparency_mask */
     NULL,                              /* end_transparency_mask */
     NULL,                              /* discard_transparency_layer */
     NULL,                              /* get_color_mapping_procs */
     NULL,                              /* get_color_comp_index */
     NULL,                              /* encode_color */
     NULL,                              /* decode_color */
     NULL,                              /* pattern_manage */
     pattern_accum_fill_rectangle_hl_color, /* fill_rectangle_hl_color */
     NULL,                              /* include_color_space */
     NULL,                              /* fill_linear_color_scanline */
     NULL,                              /* fill_linear_color_trapezoid */
     NULL,                              /* fill_linear_color_triangle */
     NULL,                              /* update_spot_equivalent_colors */
     NULL,                              /* ret_devn_params */
     NULL,                              /* fillpage */
     NULL,                              /* push_transparency_state */
     NULL,                              /* pop_transparency_state */
     NULL,                              /* put_image */
     pattern_accum_dev_spec_op,         /* dev_spec_op */
     pattern_accum_copy_planes,         /* copy_planes */
     NULL,                              /* get_profile */
     NULL,                              /* set_graphics_type_tag */
     gx_default_strip_copy_rop2,
     gx_default_strip_tile_rect_devn,
     NULL,                              /* alpha_hl_color */
     NULL,                              /* process_page */
     gx_default_transform_pixel_region  /* NOT the default forwarding one */
},
 0,                             /* target */
 0, 0, 0, 0                     /* bitmap_memory, bits, mask, instance */
};

int
pattern_clist_open_device(gx_device *dev)
{
    /* This function is defiled only for clist_init_bands. */
    return gs_clist_device_procs.open_device(dev);
}

static dev_proc_create_buf_device(dummy_create_buf_device)
{
    gx_device_memory *mdev = (gx_device_memory *)*pbdev;

    gs_make_mem_device(mdev, gdev_mem_device_for_bits(target->color_info.depth),
                mem, 0, target);
    return 0;
}
static dev_proc_size_buf_device(dummy_size_buf_device)
{
    return 0;
}
static dev_proc_setup_buf_device(dummy_setup_buf_device)
{
    return 0;
}
static dev_proc_destroy_buf_device(dummy_destroy_buf_device)
{
}
/* Attempt to determine the size of a pattern (the approximate amount that will */
/* be needed in the pattern cache). If we end up using the clist, this is only  */
/* a guess -- we use the tile size which will _probably_ be too large.          */
static int
gx_pattern_size_estimate(gs_pattern1_instance_t *pinst, bool has_tags)
{
    gx_device *tdev = pinst->saved->device;
    int depth = (pinst->templat.PaintType == 2 ? 1 : tdev->color_info.depth);
    int64_t raster;
    int64_t size;

    if (pinst->size.x == 0 || pinst->size.y == 0)
        return 0;

    if (pinst->templat.uses_transparency) {
        /* if the device has tags, add in an extra tag byte for the pdf14 compositor */
        raster = ((int64_t)pinst->size.x * ((depth/8) + 1 + (has_tags ? 1 : 0)));
    } else {
        raster = ((int64_t)pinst->size.x * depth + 7) / 8;
    }
    size = raster > max_int / pinst->size.y ? (max_int & ~0xFFFF) : raster * pinst->size.y;
    if (size > (int64_t)max_int)
        size = (max_int & ~0xFFFF);
    return (int)size;
}

static void gx_pattern_accum_finalize_cw(gx_device * dev)
{
    gx_device_clist_writer *cwdev = (gx_device_clist_writer *)dev;
    rc_decrement_only(cwdev->target, "gx_pattern_accum_finalize_cw");
}

bool gx_device_is_pattern_accum(gx_device *dev)
{
    return dev_proc(dev, open_device) == pattern_accum_open;
}

bool gx_device_is_pattern_clist(gx_device *dev)
{
    return dev_proc(dev, open_device) == pattern_clist_open_device;
}

/* Allocate a pattern accumulator, with an initial refct of 0. */
gx_device_forward *
gx_pattern_accum_alloc(gs_memory_t * mem, gs_memory_t * storage_memory,
                       gs_pattern1_instance_t *pinst, client_name_t cname)
{
    gx_device *tdev = pinst->saved->device;
    bool has_tags = device_encodes_tags(tdev);
    int size = gx_pattern_size_estimate(pinst, has_tags);
    gx_device_forward *fdev;
    int force_no_clist = 0;
    int max_pattern_bitmap = tdev->MaxPatternBitmap == 0 ? MaxPatternBitmap_DEFAULT :
                                tdev->MaxPatternBitmap;

    pinst->is_planar = tdev->is_planar;
    /*
     * If the target device can accumulate a pattern stream and the language
     * client supports high level patterns (ps and pdf only) we don't need a
     * raster or clist representation for the pattern, but the code goes
     * through the motions of creating the device anyway.  Later when the
     * pattern paint procedure is called  an error is returned and whatever
     * has been set up here is destroyed.  We try to make sure the same path
     * is taken in the code even though the device is never used because
     * there are pathological problems (see Bug689851.pdf) where the pattern
     * is so large we can't even allocate the memory for the device and the
     * dummy clist path must be used.  None of this discussion is relevant if
     * the client language does not support high level patterns or the device
     * cannot accumulate the pattern stream.
     */
    if (pinst->saved->have_pattern_streams == 0 && (*dev_proc(pinst->saved->device,
        dev_spec_op))((gx_device *)pinst->saved->device,
        gxdso_pattern_can_accum, pinst, 0) == 1)
        force_no_clist = 1; /* Set only for first time through */
    if (force_no_clist || (size < max_pattern_bitmap && !pinst->is_clist)
        || pinst->templat.PaintType != 1 ) {
        gx_device_pattern_accum *adev = gs_alloc_struct(mem, gx_device_pattern_accum,
                        &st_device_pattern_accum, cname);
        if (adev == 0)
            return 0;
#ifdef DEBUG
        if (pinst->is_clist)
            emprintf(mem, "not using clist even though clist is requested\n");
#endif
        pinst->is_clist = false;
        gx_device_init((gx_device *)adev,
                       (const gx_device *)&gs_pattern_accum_device,
                       mem, true);
        adev->instance = pinst;
        adev->bitmap_memory = storage_memory;
        fdev = (gx_device_forward *)adev;
    } else {
        gx_device_buf_procs_t buf_procs = {dummy_create_buf_device,
        dummy_size_buf_device, dummy_setup_buf_device, dummy_destroy_buf_device};
        gx_device_clist *cdev;
        gx_device_clist_writer *cwdev;
        const int data_size = 1024*128;
        gx_band_params_t band_params = { 0 };
        byte *data  = gs_alloc_bytes(tdev->memory->non_gc_memory, data_size, cname);

        if (data == NULL)
            return 0;
        pinst->is_clist = true;
        /* NB: band_params.page_uses_transparency is set in clist_make_accum_device */
        band_params.BandWidth = pinst->size.x;
        band_params.BandHeight = pinst->size.y;
        band_params.BandBufferSpace = 0;

        cdev = clist_make_accum_device(tdev, "pattern-clist", data, data_size,
                                       &buf_procs, &band_params, true, /* use_memory_clist */
                                       pinst->templat.uses_transparency, pinst);
        if (cdev == 0) {
            gs_free_object(tdev->memory->non_gc_memory, data, cname);
            return 0;
        }
        cwdev = (gx_device_clist_writer *)cdev;
        cwdev->finalize = gx_pattern_accum_finalize_cw;
        set_dev_proc(cwdev, open_device, pattern_clist_open_device);
        fdev = (gx_device_forward *)cdev;
    }
    fdev->log2_align_mod = tdev->log2_align_mod;
    fdev->pad = tdev->pad;
    fdev->is_planar = tdev->is_planar;
    fdev->graphics_type_tag = tdev->graphics_type_tag;
    fdev->interpolate_control = tdev->interpolate_control;
    gx_device_forward_fill_in_procs(fdev);
    return fdev;
}

/*
 * Initialize a pattern accumulator.
 * Client must already have set instance and bitmap_memory.
 *
 * Note that mask and bits accumulators are only created if necessary.
 */
static int
pattern_accum_open(gx_device * dev)
{
    gx_device_pattern_accum *const padev = (gx_device_pattern_accum *) dev;
    const gs_pattern1_instance_t *pinst = padev->instance;
    gs_memory_t *mem = padev->bitmap_memory;
    gx_device_memory *mask = 0;
    gx_device_memory *bits = 0;
    /*
     * The client should preset the target, because the device for which the
     * pattern is being rendered may not (in general, will not) be the same
     * as the one that was current when the pattern was instantiated.
     */
    gx_device *target =
        (padev->target == 0 ? gs_currentdevice(pinst->saved) :
         padev->target);
    int width = pinst->size.x;
    int height = pinst->size.y;
    int code = 0;
    bool mask_open = false;

    /*
     * C's bizarre coercion rules force us to copy HWResolution in pieces
     * rather than using a single assignment.
     */
#define PDSET(dev)\
  ((dev)->width = width, (dev)->height = height,\
   /*(dev)->HWResolution = target->HWResolution*/\
   (dev)->HWResolution[0] = target->HWResolution[0],\
   (dev)->HWResolution[1] = target->HWResolution[1])

    PDSET(padev);
    padev->color_info = target->color_info;
    /* Bug 689737: If PaintType == 2 (Uncolored tiling pattern), pattern is
     * 1bpp bitmap. No antialiasing in this case! */
    if (pinst->templat.PaintType == 2) {
        padev->color_info.anti_alias.text_bits = 1;
        padev->color_info.anti_alias.graphics_bits = 1;
    }
    /* If we have transparency, then fix the color info
       now so that the mem device allocates the proper
       buffer space for the pattern template.  We can
       do this since the transparency code all */
    if (pinst->templat.uses_transparency) {
        /* Allocate structure that we will use for the trans pattern */
        padev->transbuff = gs_alloc_struct(mem,gx_pattern_trans_t,&st_pattern_trans,"pattern_accum_open(trans)");
        padev->transbuff->transbytes = NULL;
        padev->transbuff->mem = NULL;
        padev->transbuff->pdev14 = NULL;
        padev->transbuff->fill_trans_buffer = NULL;
        /* n_chan = 0 => padev->transbuff isn't inited. */
        padev->transbuff->n_chan = 0;
        padev->transbuff->buf = NULL;
    } else {
        padev->transbuff = NULL;
    }
    if (pinst->uses_mask) {
        mask = gs_alloc_struct( mem,
                                gx_device_memory,
                                &st_device_memory,
                                "pattern_accum_open(mask)"
                                );
        if (mask == 0)
            return_error(gs_error_VMerror);
        gs_make_mem_mono_device(mask, mem, 0);
        PDSET(mask);
        mask->bitmap_memory = mem;
        mask->base = 0;
        code = (*dev_proc(mask, open_device)) ((gx_device *) mask);
        if (code >= 0) {
            mask_open = true;
            memset(mask->base, 0, mask->raster * mask->height);
        }
    }

    if (code >= 0) {
        if (pinst->templat.uses_transparency) {
            /* In this case, we will grab the buffer created
               by the graphic state's device (which is pdf14) and
               we will be tiling that into a transparency group buffer
               to blend with the pattern accumulator's target.  Since
               all the transparency stuff is planar format, it is
               best just to keep the data in that form */
            gx_device_set_target((gx_device_forward *)padev, target);
        } else {
            switch (pinst->templat.PaintType) {
            case 2:             /* uncolored */
                gx_device_set_target((gx_device_forward *)padev, target);
                break;
            case 1:             /* colored */
                bits = gs_alloc_struct(mem, gx_device_memory,
                                       &st_device_memory,
                                       "pattern_accum_open(bits)");
                if (bits == 0)
                    code = gs_note_error(gs_error_VMerror);
                else {
                    gs_make_mem_device(bits,
                            gdev_mem_device_for_bits(padev->color_info.depth),
                                       mem, -1, target);
                    PDSET(bits);
#undef PDSET
                    bits->color_info = padev->color_info;
                    bits->bitmap_memory = mem;
                    if (target->is_planar > 0)
                    {
                        gx_render_plane_t planes[GX_DEVICE_COLOR_MAX_COMPONENTS];
                        uchar num_comp = padev->color_info.num_components;
                        uchar i;
                        int depth = target->color_info.depth / target->color_info.num_components;
                        for (i = 0; i < num_comp; i++)
                        {
                            planes[i].shift = depth * (num_comp - 1 - i);
                            planes[i].depth = depth;
                            planes[i].index = i;
                        }
                        code = gdev_mem_set_planar(bits, num_comp, planes);
                    }
                    if (code >= 0) {
                        code = (*dev_proc(bits, open_device)) ((gx_device *) bits);
                        gx_device_set_target((gx_device_forward *)padev,
                                             (gx_device *)bits);
                        /* The update_spot_equivalent_color proc for the bits device
                           should forward to the real target device.  This will ensure
                           that the target device can get equivalent CMYK values for
                           spot colors if we are using a separation device and the spot
                           color occurs only in patterns on the page. */
                        bits->procs.update_spot_equivalent_colors = gx_forward_update_spot_equivalent_colors;
                    }
                }
            }
        }
    }
    if (code < 0) {
        if (bits != 0)
            gs_free_object(mem, bits, "pattern_accum_open(bits)");
        if (mask != 0) {
            if (mask_open)
                (*dev_proc(mask, close_device)) ((gx_device *) mask);
            gs_free_object(mem, mask, "pattern_accum_open(mask)");
        }
        return code;
    }
    padev->mask = mask;
    padev->bits = bits;
    /* Retain the device, so it will survive anomalous grestores. */
    gx_device_retain(dev, true);
    return code;
}

gx_pattern_trans_t*
new_pattern_trans_buff(gs_memory_t *mem)
{
    gx_pattern_trans_t *result;

    /* Allocate structure that we will use for the trans pattern */
    result = gs_alloc_struct(mem, gx_pattern_trans_t, &st_pattern_trans, "new_pattern_trans_buff");
    result->transbytes = NULL;
    result->pdev14 = NULL;
    result->mem = NULL;
    result->fill_trans_buffer = NULL;

    return(result);
}

/* Close an accumulator and free the bits. */
static int
pattern_accum_close(gx_device * dev)
{
    gx_device_pattern_accum *const padev = (gx_device_pattern_accum *) dev;
    gs_memory_t *mem = padev->bitmap_memory;

    /*
     * If bits != 0, it is the target of the device; reference counting
     * will close and free it.
     */
    gx_device_set_target((gx_device_forward *)padev, NULL);
    padev->bits = 0;
    if (padev->mask != 0) {
        (*dev_proc(padev->mask, close_device)) ((gx_device *) padev->mask);
        gs_free_object(mem, padev->mask, "pattern_accum_close(mask)");
        padev->mask = 0;
    }

    if (padev->transbuff != 0) {
        gs_free_object(mem,padev->target,"pattern_accum_close(transbuff)");
        padev->transbuff = NULL;
    }

    /* Un-retain the device now, so reference counting will free it. */
    gx_device_retain(dev, false);
    return 0;
}

/* _hl_color */
static int
pattern_accum_fill_rectangle_hl_color(gx_device *dev, const gs_fixed_rect *rect,
                                      const gs_gstate *pgs,
                                      const gx_drawing_color *pdcolor,
                                      const gx_clip_path *pcpath)
{
    gx_device_pattern_accum *const padev = (gx_device_pattern_accum *) dev;
    int code;

    if (padev->bits) {
        code = (*dev_proc(padev->target, fill_rectangle_hl_color))
            (padev->target, rect, pgs, pdcolor, pcpath);
        if (code < 0)
            return code;
    }
    if (padev->mask) {
        int x, y, w, h;

        x = fixed2int(rect->p.x);
        y = fixed2int(rect->p.y);
        w = fixed2int(rect->q.x) - x;
        h = fixed2int(rect->q.y) - y;

        return (*dev_proc(padev->mask, fill_rectangle))
            ((gx_device *) padev->mask, x, y, w, h, (gx_color_index) 1);
    }
    return 0;
}

/* Fill a rectangle */
static int
pattern_accum_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
                             gx_color_index color)
{
    gx_device_pattern_accum *const padev = (gx_device_pattern_accum *) dev;

    if (padev->bits)
        (*dev_proc(padev->target, fill_rectangle))
            (padev->target, x, y, w, h, color);
    if (padev->mask)
        return (*dev_proc(padev->mask, fill_rectangle))
            ((gx_device *) padev->mask, x, y, w, h, (gx_color_index) 1);
     else
        return 0;
}

/* Copy a monochrome bitmap. */
static int
pattern_accum_copy_mono(gx_device * dev, const byte * data, int data_x,
                    int raster, gx_bitmap_id id, int x, int y, int w, int h,
                        gx_color_index color0, gx_color_index color1)
{
    gx_device_pattern_accum *const padev = (gx_device_pattern_accum *) dev;

    /* opt out early if nothing to render (some may think this a bug) */
    if (color0 == gx_no_color_index && color1 == gx_no_color_index)
        return 0;
    if (padev->bits)
        (*dev_proc(padev->target, copy_mono))
            (padev->target, data, data_x, raster, id, x, y, w, h,
             color0, color1);
    if (padev->mask) {
        if (color0 != gx_no_color_index)
            color0 = 1;
        if (color1 != gx_no_color_index)
            color1 = 1;
        if (color0 == 1 && color1 == 1)
            return (*dev_proc(padev->mask, fill_rectangle))
                ((gx_device *) padev->mask, x, y, w, h, (gx_color_index) 1);
        else
            return (*dev_proc(padev->mask, copy_mono))
                ((gx_device *) padev->mask, data, data_x, raster, id, x, y, w, h,
                 color0, color1);
    } else
        return 0;
}

/* Copy a color bitmap. */
static int
pattern_accum_copy_color(gx_device * dev, const byte * data, int data_x,
                    int raster, gx_bitmap_id id, int x, int y, int w, int h)
{
    gx_device_pattern_accum *const padev = (gx_device_pattern_accum *) dev;

    if (padev->bits)
        (*dev_proc(padev->target, copy_color))
            (padev->target, data, data_x, raster, id, x, y, w, h);
    if (padev->mask)
        return (*dev_proc(padev->mask, fill_rectangle))
            ((gx_device *) padev->mask, x, y, w, h, (gx_color_index) 1);
    else
        return 0;
}

/* Copy a color plane. */
static int
pattern_accum_copy_planes(gx_device * dev, const byte * data, int data_x,
                          int raster, gx_bitmap_id id,
                          int x, int y, int w, int h, int plane_height)
{
    gx_device_pattern_accum *const padev = (gx_device_pattern_accum *) dev;

    if (padev->bits)
        (*dev_proc(padev->target, copy_planes))
            (padev->target, data, data_x, raster, id, x, y, w, h, plane_height);
    if (padev->mask)
        return (*dev_proc(padev->mask, fill_rectangle))
            ((gx_device *) padev->mask, x, y, w, h, (gx_color_index) 1);
    else
        return 0;
}

static int
blank_unmasked_bits(gx_device * mask,
                    int polarity,
                    int num_comps,
                    int depth,
                    const gs_int_rect *prect,
                    gs_get_bits_params_t *p)
{
    static const int required_options = GB_COLORS_NATIVE
                       | GB_ALPHA_NONE
                       | GB_RETURN_COPY
                       | GB_ALIGN_STANDARD
                       | GB_OFFSET_0
                       | GB_RASTER_STANDARD;
    int raster = p->raster;
    byte *min;
    int x0 = prect->p.x;
    int y0 = prect->p.y;
    int x, y;
    int w = prect->q.x - x0;
    int h = prect->q.y - y0;
    int code = 0;
    byte *ptr;
    int blank = (polarity == GX_CINFO_POLARITY_ADDITIVE ? 255 : 0);

    if ((p->options & required_options) != required_options)
        return_error(gs_error_rangecheck);

    min = gs_alloc_bytes(mask->memory, (w+7)>>3, "blank_unmasked_bits");
    if (min == NULL)
        return_error(gs_error_VMerror);

    if (p->options & GB_PACKING_CHUNKY)
    {
        if ((depth & 7) != 0 || depth > 64)
            return_error(gs_error_rangecheck);
        ptr = p->data[0];
        depth >>= 3;
        raster -= w*depth;
        for (y = 0; y < h; y++)
        {
            byte *mine;
            code = dev_proc(mask, get_bits)(mask, y+y0, min, &mine);
            if (code < 0)
                goto fail;
            for (x = 0; x < w; x++)
            {
                int xx = x+x0;
                if (((mine[xx>>3]>>(x&7)) & 1) == 0) {
                    switch (depth)
                    {
                    case 8:
                        *ptr++ = blank;
                    case 7:
                        *ptr++ = blank;
                    case 6:
                        *ptr++ = blank;
                    case 5:
                        *ptr++ = blank;
                    case 4:
                        *ptr++ = blank;
                    case 3:
                        *ptr++ = blank;
                    case 2:
                        *ptr++ = blank;
                    case 1:
                        *ptr++ = blank;
                        break;
                    }
                } else {
                    ptr += depth;
                }
            }
            ptr += raster;
        }
    } else {
        if (depth/num_comps != 8)
            return_error(gs_error_rangecheck);
        for (y = 0; y < h; y++)
        {
            int c;
            byte *mine;
            code = dev_proc(mask, get_bits)(mask, y+y0, min, &mine);
            if (code < 0)
                goto fail;
            for (c = 0; c < num_comps; c++)
            {
                if (p->data[c] == NULL)
                    continue;
                ptr = p->data[c] + raster * y;
                for (x = 0; x < w; x++)
                {
                    int xx = x+x0;
                    if (((mine[xx>>3]>>(x&7)) & 1) == 0) {
                        *ptr++ = blank;
                    } else {
                        ptr++;
                    }
                }
            }
        }
    }

fail:
    gs_free_object(mask->memory, min, "blank_unmasked_bits");

    return code;
}

/* Read back a rectangle of bits. */
/****** SHOULD USE MASK TO DEFINE UNREAD AREA *****/
static int
pattern_accum_get_bits_rectangle(gx_device * dev, const gs_int_rect * prect,
                       gs_get_bits_params_t * params, gs_int_rect ** unread)
{
    gx_device_pattern_accum *const padev = (gx_device_pattern_accum *) dev;
    int code;
    gs_get_bits_params_t params2 = *params;

    if (padev->bits) {
        if (padev->mask)
            params2.options &= ~GB_RETURN_POINTER;
        code = (*dev_proc(padev->target, get_bits_rectangle))
            (padev->target, prect, &params2, unread);
        /* If we have a mask, then unmarked pixels of the bits
         * will be undefined. Strictly speaking it makes no
         * sense for us to return any value here, but the only
         * caller of this currently is the overprint code, which
         * uses the the values to parrot back to us. Let's
         * make sure they are set to the default 'empty' values.
         */
        if (code >= 0 && padev->mask)
            code = blank_unmasked_bits((gx_device *)padev->mask,
                                       padev->target->color_info.polarity,
                                       padev->target->color_info.num_components,
                                       padev->target->color_info.depth,
                                       prect, &params2);
        return code;
    }

    return_error(gs_error_Fatal); /* shouldn't happen */
}

/* ------ Color space implementation ------ */

/* Free all entries in a pattern cache. */
static bool
pattern_cache_choose_all(gx_color_tile * ctile, void *proc_data)
{
    return true;
}
static void
pattern_cache_free_all(gx_pattern_cache * pcache)
{
    gx_pattern_cache_winnow(pcache, pattern_cache_choose_all, NULL);
}

/* Allocate a Pattern cache. */
gx_pattern_cache *
gx_pattern_alloc_cache(gs_memory_t * mem, uint num_tiles, ulong max_bits)
{
    gx_pattern_cache *pcache =
    gs_alloc_struct(mem, gx_pattern_cache, &st_pattern_cache,
                    "gx_pattern_alloc_cache(struct)");
    gx_color_tile *tiles =
    gs_alloc_struct_array(mem, num_tiles, gx_color_tile,
                          &st_color_tile_element,
                          "gx_pattern_alloc_cache(tiles)");
    uint i;

    if (pcache == 0 || tiles == 0) {
        gs_free_object(mem, tiles, "gx_pattern_alloc_cache(tiles)");
        gs_free_object(mem, pcache, "gx_pattern_alloc_cache(struct)");
        return 0;
    }
    pcache->memory = mem;
    pcache->tiles = tiles;
    pcache->num_tiles = num_tiles;
    pcache->tiles_used = 0;
    pcache->next = 0;
    pcache->bits_used = 0;
    pcache->max_bits = max_bits;
    pcache->free_all = pattern_cache_free_all;
    for (i = 0; i < num_tiles; tiles++, i++) {
        tiles->id = gx_no_bitmap_id;
        /* Clear the pointers to pacify the GC. */
        uid_set_invalid(&tiles->uid);
        tiles->bits_used = 0;
#ifdef PACIFY_VALGRIND
        /* The following memsets are required to avoid a valgrind warning
         * in:
         *   gs -I./gs/lib -sOutputFile=out.pgm -dMaxBitmap=10000
         *      -sDEVICE=pgmraw -r300 -Z: -sDEFAULTPAPERSIZE=letter
         *      -dNOPAUSE -dBATCH -K2000000 -dClusterJob -dJOBSERVER
         *      tests_private/ps/ps3cet/11-14.PS
         * Setting the individual elements of the structures directly is
         * not enough, which leads me to believe that we are writing the
         * entire structs out, padding and all.
         */
        memset(&tiles->tbits, 0, sizeof(tiles->tbits));
        memset(&tiles->tmask, 0, sizeof(tiles->tmask));
#else
        tiles->tbits.data = 0;
        tiles->tmask.data = 0;
#endif
        tiles->index = i;
        tiles->cdev = NULL;
        tiles->ttrans = NULL;
        tiles->is_planar = false;
    }
    return pcache;
}
/* Ensure that an imager has a Pattern cache. */
static int
ensure_pattern_cache(gs_gstate * pgs)
{
    if (pgs->pattern_cache == 0) {
        gx_pattern_cache *pcache =
        gx_pattern_alloc_cache(pgs->memory,
                               gx_pat_cache_default_tiles(),
                               gx_pat_cache_default_bits());

        if (pcache == 0)
            return_error(gs_error_VMerror);
        pgs->pattern_cache = pcache;
    }
    return 0;
}

/* Free pattern cache and its components. */
void
gx_pattern_cache_free(gx_pattern_cache *pcache)
{
    pattern_cache_free_all(pcache);
    gs_free_object(pcache->memory, pcache->tiles, "gx_pattern_cache_free");
    pcache->tiles = NULL;
    gs_free_object(pcache->memory, pcache, "gx_pattern_cache_free");
}

/* Get and set the Pattern cache in a gstate. */
gx_pattern_cache *
gstate_pattern_cache(gs_gstate * pgs)
{
    return pgs->pattern_cache;
}
void
gstate_set_pattern_cache(gs_gstate * pgs, gx_pattern_cache * pcache)
{
    pgs->pattern_cache = pcache;
}

/* Free a Pattern cache entry. */
static void
gx_pattern_cache_free_entry(gx_pattern_cache * pcache, gx_color_tile * ctile)
{
    gx_device *temp_device;

    if ((ctile->id != gx_no_bitmap_id) && !ctile->is_dummy) {
        gs_memory_t *mem = pcache->memory;

        /*
         * We must initialize the memory device properly, even though
         * we aren't using it for drawing.
         */
        if (ctile->tmask.data != 0) {
            gs_free_object(mem, ctile->tmask.data,
                           "free_pattern_cache_entry(mask data)");
            ctile->tmask.data = 0;      /* for GC */
        }
        if (ctile->tbits.data != 0) {
            gs_free_object(mem, ctile->tbits.data,
                           "free_pattern_cache_entry(bits data)");
            ctile->tbits.data = 0;      /* for GC */
        }
        if (ctile->cdev != NULL) {
            ctile->cdev->common.do_not_open_or_close_bandfiles = false;  /* make sure memfile gets freed/closed */
            dev_proc(&ctile->cdev->common, close_device)((gx_device *)&ctile->cdev->common);
            /* Free up the icc based stuff in the clist device.  I am puzzled
               why the other objects are not released */
            clist_free_icc_table(ctile->cdev->common.icc_table,
                            ctile->cdev->common.memory);
            ctile->cdev->common.icc_table = NULL;
            rc_decrement(ctile->cdev->common.icc_cache_cl,
                            "gx_pattern_cache_free_entry");
            ctile->cdev->common.icc_cache_cl = NULL;
            ctile->cdev->writer.pinst = NULL;
            gs_free_object(ctile->cdev->common.memory->non_gc_memory, ctile->cdev->common.cache_chunk, "free tile cache for clist");
            ctile->cdev->common.cache_chunk = 0;
            temp_device = (gx_device *)ctile->cdev;
            gx_device_retain(temp_device, false);
            ctile->cdev = NULL;
        }

        if (ctile->ttrans != NULL) {
            if_debug2m('v', mem,
                       "[v*] Freeing trans pattern from cache, uid = %ld id = %ld \n",
                       ctile->uid.id, ctile->id);
            if ( ctile->ttrans->pdev14 == NULL) {
                /* This can happen if we came from the clist */
                if (ctile->ttrans->mem != NULL)
                    gs_free_object(ctile->ttrans->mem ,ctile->ttrans->transbytes,
                                   "free_pattern_cache_entry(transbytes)");
                gs_free_object(mem,ctile->ttrans->fill_trans_buffer,
                                "free_pattern_cache_entry(fill_trans_buffer)");
                ctile->ttrans->transbytes = NULL;
                ctile->ttrans->fill_trans_buffer = NULL;
            } else {
                dev_proc(ctile->ttrans->pdev14, close_device)((gx_device *)ctile->ttrans->pdev14);
                temp_device = (gx_device *)(ctile->ttrans->pdev14);
                gx_device_retain(temp_device, false);
                rc_decrement(temp_device,"gx_pattern_cache_free_entry");
                ctile->ttrans->pdev14 = NULL;
                ctile->ttrans->transbytes = NULL;  /* should be ok due to pdf14_close */
                ctile->ttrans->fill_trans_buffer = NULL; /* This is always freed */
            }

            gs_free_object(mem, ctile->ttrans,
                           "free_pattern_cache_entry(ttrans)");
            ctile->ttrans = NULL;

        }

        pcache->tiles_used--;
        pcache->bits_used -= ctile->bits_used;
        ctile->id = gx_no_bitmap_id;
    }
}

/* Given the size of a new pattern tile, free entries from the cache until  */
/* enough space is available (or nothing left to free).                     */
/* This will allow 1 oversized entry                                        */
void
gx_pattern_cache_ensure_space(gs_gstate * pgs, int needed)
{
    int code = ensure_pattern_cache(pgs);
    gx_pattern_cache *pcache;

    if (code < 0)
        return;                 /* no cache -- just exit */

    pcache = pgs->pattern_cache;

    /* If too large then start freeing entries */
    /* By starting at 'next', we attempt to first free the oldest entries */
    while (pcache->bits_used + needed > pcache->max_bits &&
           pcache->bits_used != 0) {
        pcache->next = (pcache->next + 1) % pcache->num_tiles;
        gx_pattern_cache_free_entry(pcache, &pcache->tiles[pcache->next]);
    }
}

/* Export updating the pattern_cache bits_used and tiles_used for clist reading */
void
gx_pattern_cache_update_used(gs_gstate *pgs, ulong used)
{
    gx_pattern_cache *pcache = pgs->pattern_cache;

    pcache->bits_used += used;
    pcache->tiles_used++;
}

/*
 * Add a Pattern cache entry.  This is exported for the interpreter.
 * Note that this does not free any of the data in the accumulator
 * device, but it may zero out the bitmap_memory pointers to prevent
 * the accumulated bitmaps from being freed when the device is closed.
 */
static void make_bitmap(gx_strip_bitmap *, const gx_device_memory *, gx_bitmap_id);
int
gx_pattern_cache_add_entry(gs_gstate * pgs,
                   gx_device_forward * fdev, gx_color_tile ** pctile)
{
    gx_pattern_cache *pcache;
    const gs_pattern1_instance_t *pinst;
    ulong used = 0, mask_used = 0, trans_used = 0;
    gx_bitmap_id id;
    gx_color_tile *ctile;
    int code = ensure_pattern_cache(pgs);
    extern dev_proc_open_device(pattern_clist_open_device);
    gx_device_memory *mmask = NULL;
    gx_device_memory *mbits = NULL;
    gx_pattern_trans_t *trans = NULL;
    int size_b, size_c;

    if (code < 0)
        return code;
    pcache = pgs->pattern_cache;

    if (dev_proc(fdev, open_device) != pattern_clist_open_device) {
        gx_device_pattern_accum *padev = (gx_device_pattern_accum *)fdev;

        mbits = padev->bits;
        mmask = padev->mask;
        pinst = padev->instance;
        trans = padev->transbuff;

        /*
         * Check whether the pattern completely fills its box.
         * If so, we can avoid the expensive masking operations
         * when using the pattern.
         */
        /* Bug 700624: In cases where the mask is completely full,
         * but the pattern cells are separated from one another,
         * we need to leave gaps between the cells when rendering
         * them. Sadly, the graphics library can't cope with this
         * in the no-mask case. Therefore, only do the optimisation
         * of not sending the mask if the step matrix is suitable.
         *
         * To do this, we compare the step matrix to the size. My
         * belief is that the mask will only ever be full if it's
         * orthogonal, cos otherwise the edges will be clipped,
         * hence we lose no generality by checking for .xy and .yx
         * being 0.
         */
        if (mmask != 0 &&
            fabsf(pinst->step_matrix.tx) <= pinst->size.x &&
            fabsf(pinst->step_matrix.ty) <= pinst->size.y &&
            pinst->step_matrix.xy == 0 &&
            pinst->step_matrix.yx == 0) {
            int y;
            int w_less_8 = mmask->width-8;

            for (y = 0; y < mmask->height; y++) {
                const byte *row = scan_line_base(mmask, y);
                int w;

                for (w = w_less_8; w > 0; w -= 8)
                    if (*row++ != 0xff)
                        goto keep;
                w += 8;
                if ((*row | (0xff >> w)) != 0xff)
                    goto keep;
            }
            /* We don't need a mask. */
            mmask = 0;
          keep:;
        }
        /* Need to get size of buffers that are being added to the cache */
        if (mbits != 0)
            gdev_mem_bitmap_size(mbits, &used);
        if (mmask != 0) {
            gdev_mem_bitmap_size(mmask, &mask_used);
            used += mask_used;
        }
        if (trans != 0) {
            trans_used = trans->planestride*trans->n_chan;
            used += trans_used;
        }
    } else {
        gx_device_clist *cdev = (gx_device_clist *)fdev;
        gx_device_clist_writer * cldev = (gx_device_clist_writer *)cdev;

        code = clist_end_page(cldev);
        if (code < 0)
            return code;
        pinst = cdev->writer.pinst;
        size_b = clist_data_size(cdev, 0);
        if (size_b < 0)
            return_error(gs_error_unregistered);
        size_c = clist_data_size(cdev, 1);
        if (size_c < 0)
            return_error(gs_error_unregistered);
        /* The memfile size is the size, not the size determined by the depth*width*height */
        used = size_b + size_c;
    }
    id = pinst->id;
    ctile = &pcache->tiles[id % pcache->num_tiles];
    gx_pattern_cache_free_entry(pcache, ctile);         /* ensure that this cache slot is empty */
    ctile->id = id;
    ctile->is_planar = pinst->is_planar;
    ctile->depth = fdev->color_info.depth;
    ctile->uid = pinst->templat.uid;
    ctile->tiling_type = pinst->templat.TilingType;
    ctile->step_matrix = pinst->step_matrix;
    ctile->bbox = pinst->bbox;
    ctile->is_simple = pinst->is_simple;
    ctile->has_overlap = pinst->has_overlap;
    ctile->is_dummy = false;
    if (pinst->templat.uses_transparency)
        ctile->blending_mode = ((pdf14_device *)(pgs->device))->blend_mode;
    else
        ctile->blending_mode = 0;
    if (dev_proc(fdev, open_device) != pattern_clist_open_device) {
        if (mbits != 0) {
            make_bitmap(&ctile->tbits, mbits, gs_next_ids(pgs->memory, 1));
            mbits->bitmap_memory = 0;   /* don't free the bits */
        } else
            ctile->tbits.data = 0;
        if (mmask != 0) {
            make_bitmap(&ctile->tmask, mmask, id);
            mmask->bitmap_memory = 0;   /* don't free the bits */
        } else
            ctile->tmask.data = 0;
        if (trans != 0) {
            if_debug2m('v', pgs->memory,
                       "[v*] Adding trans pattern to cache, uid = %ld id = %ld \n",
                       ctile->uid.id, ctile->id);
            ctile->ttrans = trans;
        }

        ctile->cdev = NULL;
    } else {
        gx_device_clist *cdev = (gx_device_clist *)fdev;
        gx_device_clist_writer *cwdev = (gx_device_clist_writer *)fdev;

        ctile->tbits.data = 0;
        ctile->tbits.size.x = 0;
        ctile->tbits.size.y = 0;
        ctile->tmask.data = 0;
        ctile->tmask.size.x = 0;
        ctile->tmask.size.y = 0;
        ctile->cdev = cdev;
        /* Prevent freeing files on pattern_paint_cleanup : */
        cwdev->do_not_open_or_close_bandfiles = true;
    }
    /* In the clist case, used is accurate. In the non-clist case, it may
     * not be. The important thing is that we account the same for tiles
     * going in and coming out of the cache. Therefore we store the used
     * figure in the tile so we always remove the same amount. */
    ctile->bits_used = used;
    gx_pattern_cache_update_used(pgs, used);

    *pctile = ctile;
    return 0;
}

/* Get entry for reading a pattern from clist. */
int
gx_pattern_cache_get_entry(gs_gstate * pgs, gs_id id, gx_color_tile ** pctile)
{
    gx_pattern_cache *pcache;
    gx_color_tile *ctile;
    int code = ensure_pattern_cache(pgs);

    if (code < 0)
        return code;
    pcache = pgs->pattern_cache;
    ctile = &pcache->tiles[id % pcache->num_tiles];
    gx_pattern_cache_free_entry(pgs->pattern_cache, ctile);
    ctile->id = id;
    *pctile = ctile;
    return 0;
}

bool
gx_pattern_tile_is_clist(gx_color_tile *ptile)
{
    return ptile != NULL && ptile->cdev != NULL;
}

/* Add a dummy Pattern cache entry.  Stubs a pattern tile for interpreter when
   device handles high level patterns. */
int
gx_pattern_cache_add_dummy_entry(gs_gstate *pgs,
            gs_pattern1_instance_t *pinst, int depth)
{
    gx_color_tile *ctile;
    gx_pattern_cache *pcache;
    gx_bitmap_id id = pinst->id;
    int code = ensure_pattern_cache(pgs);

    if (code < 0)
        return code;
    pcache = pgs->pattern_cache;
    ctile = &pcache->tiles[id % pcache->num_tiles];
    gx_pattern_cache_free_entry(pcache, ctile);
    ctile->id = id;
    ctile->depth = depth;
    ctile->uid = pinst->templat.uid;
    ctile->tiling_type = pinst->templat.TilingType;
    ctile->step_matrix = pinst->step_matrix;
    ctile->bbox = pinst->bbox;
    ctile->is_simple = pinst->is_simple;
    ctile->has_overlap = pinst->has_overlap;
    ctile->is_dummy = true;
    memset(&ctile->tbits, 0 , sizeof(ctile->tbits));
    ctile->tbits.size = pinst->size;
    ctile->tbits.id = gs_no_bitmap_id;
    memset(&ctile->tmask, 0 , sizeof(ctile->tmask));
    ctile->cdev = NULL;
    ctile->ttrans = NULL;
    ctile->bits_used = 0;
    pcache->tiles_used++;
    return 0;
}

#if RAW_PATTERN_DUMP
/* Debug dump of pattern image data. Saved in
   interleaved form with global indexing in
   file name */
static void
dump_raw_pattern(int height, int width, int n_chan, int depth,
                byte *Buffer, int raster, const gx_device_memory * mdev)
{
    char full_file_name[50];
    FILE *fid;
    int max_bands;
    int j, k, m;
    int byte_number, bit_position;
    unsigned char current_byte;
    unsigned char output_val;
    bool is_planar;
    byte *curr_ptr = Buffer;
    int plane_offset;

    is_planar = mdev->is_planar;
    max_bands = ( n_chan < 57 ? n_chan : 56);   /* Photoshop handles at most 56 bands */
    if (is_planar) {
        gs_sprintf(full_file_name, "%d)PATTERN_PLANE_%dx%dx%d.raw", global_pat_index,
                mdev->raster, height, max_bands);
    } else {
        gs_sprintf(full_file_name, "%d)PATTERN_CHUNK_%dx%dx%d.raw", global_pat_index,
                width, height, max_bands);
    }
    fid = gp_fopen(full_file_name,"wb");
    if (depth >= 8) {
        /* Contone data. */
        if (is_planar) {
            for (m = 0; m < max_bands; m++) {
                curr_ptr = mdev->line_ptrs[m*mdev->height];
                fwrite(curr_ptr, 1, mdev->height * mdev->raster, fid);
            }
        } else {
            /* Just dump it like it is */
            fwrite(Buffer, 1, max_bands * height * width, fid);
        }
    } else {
        /* Binary Data. Lets get to 8 bit for debugging.  We have to
           worry about planar vs. chunky.  Note this assumes 1 bit data
           only. */
        if (is_planar) {
            plane_offset = mdev->raster * mdev->height;
            for (m = 0; m < max_bands; m++) {
                curr_ptr = mdev->line_ptrs[m*mdev->height];
                for (j = 0; j < height; j++) {
                    for (k = 0; k < width; k++) {
                        byte_number = (int) ceil((( (float) k + 1.0) / 8.0)) - 1;
                        current_byte = curr_ptr[j*(mdev->raster) + byte_number];
                        bit_position = 7 - (k -  byte_number*8);
                        output_val = ((current_byte >> bit_position) & 0x1) * 255;
                        fwrite(&output_val,1,1,fid);
                    }
                }
            }
        } else {
            for (j = 0; j < height; j++) {
                for (k = 0; k < width; k++) {
                    for (m = 0; m < max_bands; m++) {
                        /* index current byte */
                        byte_number =
                            (int) ceil((( (float) k * (float) max_bands +
                                          (float) m + 1.0) / 8.0)) - 1;
                        /* get byte of interest */
                        current_byte =
                                curr_ptr[j*(mdev->raster) + byte_number];
                        /* get bit position */
                        bit_position =
                                7 - (k * max_bands + m -  byte_number * 8);
                        /* extract and create byte */
                        output_val =
                                ((current_byte >> bit_position) & 0x1) * 255;
                        fwrite(&output_val,1,1,fid);
                    }
                }
            }
        }
    }
    fclose(fid);
}
#endif

static void
make_bitmap(register gx_strip_bitmap * pbm, const gx_device_memory * mdev,
            gx_bitmap_id id)
{
    pbm->data = mdev->base;
    pbm->raster = mdev->raster;
    pbm->rep_width = pbm->size.x = mdev->width;
    pbm->rep_height = pbm->size.y = mdev->height;
    pbm->id = id;
    pbm->rep_shift = pbm->shift = 0;
    pbm->num_planes = (mdev->is_planar ? mdev->color_info.num_components : 1);

        /* Lets dump this for debug purposes */

#if RAW_PATTERN_DUMP
    dump_raw_pattern(pbm->rep_height, pbm->rep_width,
                        mdev->color_info.num_components,
                        mdev->color_info.depth,
                        (unsigned char*) mdev->base,
                        pbm->raster, mdev);

        global_pat_index++;

#endif

}

/* Purge selected entries from the pattern cache. */
void
gx_pattern_cache_winnow(gx_pattern_cache * pcache,
  bool(*proc) (gx_color_tile * ctile, void *proc_data), void *proc_data)
{
    uint i;

    if (pcache == 0)            /* no cache created yet */
        return;
    for (i = 0; i < pcache->num_tiles; ++i) {
        gx_color_tile *ctile = &pcache->tiles[i];

        if (ctile->id != gx_no_bitmap_id && (*proc) (ctile, proc_data))
            gx_pattern_cache_free_entry(pcache, ctile);
    }
}

/* blank the pattern accumulator device assumed to be in the graphics
   state */
int
gx_erase_colored_pattern(gs_gstate *pgs)
{
    int code;
    gx_device_pattern_accum *pdev = (gx_device_pattern_accum *)gs_currentdevice(pgs);

    if ((code = gs_gsave(pgs)) < 0)
        return code;
    if ((code = gs_setgray(pgs, 1.0)) >= 0) {
        gs_rect rect;
        gx_device_memory *mask;
        static const gs_matrix identity = { 1, 0, 0, 1, 0, 0 };

        pgs->log_op = lop_default;
        rect.p.x = 0.0;
        rect.p.y = 0.0;
        rect.q.x = (double)pdev->width;
        rect.q.y = (double)pdev->height;

        code = gs_setmatrix(pgs, &identity);
        if (code < 0) {
            gs_grestore_only(pgs);
            return code;
        }
        /* we don't want the fill rectangle device call to use the
           mask */
        mask = pdev->mask;
        pdev->mask = NULL;
        code = gs_rectfill(pgs, &rect, 1);
        /* restore the mask */
        pdev->mask = mask;
        if (code < 0) {
            gs_grestore_only(pgs);
            return code;
        }
    }
    /* we don't need wraparound here */
    gs_grestore_only(pgs);
    return code;
}

/* Reload a (non-null) Pattern color into the cache. */
/* *pdc is already set, except for colors.pattern.p_tile and mask.m_tile. */
int
gx_pattern_load(gx_device_color * pdc, const gs_gstate * pgs,
                gx_device * dev, gs_color_select_t select)
{
    gx_device_forward *adev = NULL;
    gs_pattern1_instance_t *pinst =
        (gs_pattern1_instance_t *)pdc->ccolor.pattern;
    gs_gstate *saved;
    gx_color_tile *ctile;
    gs_memory_t *mem = pgs->memory;
    bool has_tags = device_encodes_tags(dev);
    int code;

    if (pgs->pattern_cache == NULL)
        if ((code = ensure_pattern_cache((gs_gstate *) pgs))< 0)      /* break const for call */
            return code;

    if (gx_pattern_cache_lookup(pdc, pgs, dev, select))
        return 0;

    /* Get enough space in the cache for this pattern (estimated if it is a clist) */
    gx_pattern_cache_ensure_space((gs_gstate *)pgs, gx_pattern_size_estimate(pinst, has_tags));
    /*
     * Note that adev is an internal device, so it will be freed when the
     * last reference to it from a graphics state is deleted.
     */
    adev = gx_pattern_accum_alloc(mem, pgs->pattern_cache->memory, pinst, "gx_pattern_load");
    if (adev == 0)
        return_error(gs_error_VMerror);
    gx_device_set_target((gx_device_forward *)adev, dev);
    code = dev_proc(adev, open_device)((gx_device *)adev);
    if (code < 0) {
        gs_free_object(mem, adev, "gx_pattern_load");
        return code;
    }
    saved = gs_gstate_copy(pinst->saved, pinst->saved->memory);
    if (saved == 0) {
        code = gs_note_error(gs_error_VMerror);
        goto fail;
    }
    if (saved->pattern_cache == 0)
        saved->pattern_cache = pgs->pattern_cache;
    code = gs_setdevice_no_init(saved, (gx_device *)adev);
    if (code < 0)
        goto fail;
    if (pinst->templat.uses_transparency) {
        if_debug0m('v', mem, "gx_pattern_load: pushing the pdf14 compositor device into this graphics state\n");
        if ((code = gs_push_pdf14trans_device(saved, true)) < 0)
            return code;
        saved->device->is_open = true;
    } else {
        /* For colored patterns we clear the pattern device's
           background.  This is necessary for the anti aliasing code
           and (unfortunately) it masks a difficult to fix UMR
           affecting pcl patterns, see bug #690487.  Note we have to
           make a similar change in zpcolor.c where much of this
           pattern code is duplicated to support high level stream
           patterns. */
        if (pinst->templat.PaintType == 1 && !(pinst->is_clist)
            && dev_proc(pinst->saved->device, dev_spec_op)(pinst->saved->device, gxdso_pattern_can_accum, NULL, 0) == 0)
            if ((code = gx_erase_colored_pattern(saved)) < 0)
                goto fail;
    }

    code = (*pinst->templat.PaintProc)(&pdc->ccolor, saved);
    if (code < 0) {
        if (dev_proc(adev, open_device) == pattern_accum_open) {
            // free pattern cache data that never got added to the dictionary
            gx_device_pattern_accum *padev = (gx_device_pattern_accum *) adev;
            if ((padev->bits != NULL) && (padev->bits->base != NULL)) {
                gs_free_object(padev->bits->memory, padev->bits->base, "mem_open");
            }
        }
        /* RJW: At this point, in the non transparency case,
         * saved->device == adev. So unretain it, close it, and the
         * gs_gstate_free(saved) will remove it. In the transparency case,
         * saved->device = the pdf14 device. So we need to unretain it,
         * close adev, and finally close saved->device.
         */
        gx_device_retain(saved->device, false);         /* device no longer retained */
        if (pinst->templat.uses_transparency) {
            if (pinst->is_clist == 0) {
                gs_free_object(((gx_device_pattern_accum *)adev)->bitmap_memory,
                               ((gx_device_pattern_accum *)adev)->transbuff,
                               "gx_pattern_load");
            }
            dev_proc(adev, close_device)((gx_device *)adev);
            /* adev was the target of the pdf14 device, so also is no longer retained */
            gx_device_retain((gx_device *)adev, false);         /* device no longer retained */
        }
        dev_proc(saved->device, close_device)((gx_device *)saved->device);
        /* Freeing the state should now free the device which may be the pdf14 compositor. */
        gs_gstate_free_chain(saved);
        if (code == gs_error_handled)
            code = 0;
        return code;
    }
    if (pinst->templat.uses_transparency) {
        /* if_debug0m('v', saved->memory, "gx_pattern_load: popping the pdf14 compositor device from this graphics state\n");
        if ((code = gs_pop_pdf14trans_device(saved, true)) < 0)
            return code; */
            if (pinst->is_clist) {
                /* Send the compositor command to close the PDF14 device */
                code = gs_pop_pdf14trans_device(saved, true);
                if (code < 0)
                    return code;
            } else {
                /* Not a clist, get PDF14 buffer information */
                code =
                    pdf14_get_buffer_information(saved->device,
                                                ((gx_device_pattern_accum*)adev)->transbuff,
                                                 saved->memory,
                                                 true);
                /* PDF14 device (and buffer) is destroyed when pattern cache
                   entry is removed */
                if (code < 0)
                    return code;
            }
    }
    /* We REALLY don't like the following cast.... */
    code = gx_pattern_cache_add_entry((gs_gstate *)pgs,
                adev, &ctile);
    if (code >= 0) {
        if (!gx_pattern_cache_lookup(pdc, pgs, dev, select)) {
            mlprintf(mem, "Pattern cache lookup failed after insertion!\n");
            code = gs_note_error(gs_error_Fatal);
        }
    }
#ifdef DEBUG
    if (gs_debug_c('B') && dev_proc(adev, open_device) == pattern_accum_open) {
        gx_device_pattern_accum *pdev = (gx_device_pattern_accum *)adev;

        if (pdev->mask)
            debug_dump_bitmap(pdev->memory,
                              pdev->mask->base, pdev->mask->raster,
                              pdev->mask->height, "[B]Pattern mask");
        if (pdev->bits)
            debug_dump_bitmap(pdev->memory,
                              ((gx_device_memory *) pdev->target)->base,
                              ((gx_device_memory *) pdev->target)->raster,
                              pdev->target->height, "[B]Pattern bits");
    }
#endif
    /* Free the bookkeeping structures, except for the bits and mask */
    /* data iff they are still needed. */
    dev_proc(adev, close_device)((gx_device *)adev);
    /* Free the chain of gstates. Freeing the state will free the device. */
    gs_gstate_free_chain(saved);
    return code;

fail:
    if (dev_proc(adev, open_device) == pattern_accum_open) {
        // free pattern cache data that never got added to the dictionary
        gx_device_pattern_accum *padev = (gx_device_pattern_accum *) adev;
        if ((padev->bits != NULL) && (padev->bits->base != NULL)) {
            gs_free_object(padev->bits->memory, padev->bits->base, "mem_open");
        }
    }
    if (dev_proc(adev, open_device) == pattern_clist_open_device) {
        gx_device_clist *cdev = (gx_device_clist *)adev;

        gs_free_object(cdev->writer.bandlist_memory, cdev->common.data, "gx_pattern_load");
        cdev->common.data = 0;
    }
    dev_proc(adev, close_device)((gx_device *)adev);
    gs_gstate_free_chain(saved);
    return code;
}

/* Remap a PatternType 1 color. */
cs_proc_remap_color(gx_remap_Pattern);  /* check the prototype */
int
gs_pattern1_remap_color(const gs_client_color * pc, const gs_color_space * pcs,
                        gx_device_color * pdc, const gs_gstate * pgs,
                        gx_device * dev, gs_color_select_t select)
{
    gs_pattern1_instance_t *pinst = (gs_pattern1_instance_t *)pc->pattern;
    int code;

    /* Save original color space and color info into dev color */
    pdc->ccolor = *pc;
    pdc->ccolor_valid = true;
    if (pinst == 0) {
        /* Null pattern */
        color_set_null_pattern(pdc);
        return 0;
    }
    if (pinst->templat.PaintType == 2) {       /* uncolored */
        if (pcs->base_space)
            code = (pcs->base_space->type->remap_color)
                (pc, pcs->base_space, pdc, pgs, dev, select);
        else
            code = gs_note_error(gs_error_unregistered);
        if (code < 0)
            return code;
        if (pdc->type == gx_dc_type_pure)
            pdc->type = &gx_dc_pure_masked;
        else if (pdc->type == gx_dc_type_ht_binary)
            pdc->type = &gx_dc_binary_masked;
        else if (pdc->type == gx_dc_type_ht_colored)
            pdc->type = &gx_dc_colored_masked;
        else if (pdc->type == gx_dc_type_devn)
            pdc->type = &gx_dc_devn_masked;
        else
            return_error(gs_error_unregistered);
    } else
        color_set_null_pattern(pdc);
    pdc->mask.id = pinst->id;
    pdc->mask.m_tile = 0;
    return gx_pattern_load(pdc, pgs, dev, select);
}

int
pattern_accum_dev_spec_op(gx_device *dev, int dso, void *data, int size)
{
    gx_device_pattern_accum *const padev = (gx_device_pattern_accum *)dev;
    const gs_pattern1_instance_t *pinst = padev->instance;
    gx_device *target =
        (padev->target == 0 ? gs_currentdevice(pinst->saved) :
         padev->target);

    if (dso == gxdso_in_pattern_accumulator)
        return (pinst->templat.PaintType == 2 ? 2 : 1);
    if (dso == gxdso_get_dev_param) {
        dev_param_req_t *request = (dev_param_req_t *)data;
        gs_param_list * plist = (gs_param_list *)request->list;
        bool bool_true = 1;

        if (strcmp(request->Param, "NoInterpolateImagemasks") == 0) {
            return param_write_bool(plist, "NoInterpolateImagemasks", &bool_true);
        }
    }

    return dev_proc(target, dev_spec_op)(target, dso, data, size);
}
