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


/* Halftone bit updating for imaging library */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsbitops.h"
#include "gscdefs.h"
#include "gxbitmap.h"
#include "gxhttile.h"
#include "gxtmap.h"
#include "gxdht.h"
#include "gxdhtres.h"
#include "gp.h"

#define DUMP_TOS 0

extern_gx_device_halftone_list();

/*
 * Construct a standard-representation order from a threshold array.
 */
static int
construct_ht_order_default(gx_ht_order *porder, const byte *thresholds)
{
    gx_ht_bit *bits = (gx_ht_bit *)porder->bit_data;
    uint i;

    for (i = 0; i < porder->num_bits; i++)
        bits[i].mask = max(1, thresholds[i]);
    gx_ht_complete_threshold_order(porder);
    return 0;
}

/*
 * Construct a short-representation order from a threshold array.
 * Uses porder->width, num_levels, num_bits, levels, bit_data;
 * sets porder->levels[], bit_data[].
 */
static int
construct_ht_order_short(gx_ht_order *porder, const byte *thresholds)
{
    uint size = porder->num_bits;
    uint i;
    ushort *bits = (ushort *)porder->bit_data;
    uint *levels = porder->levels;
    uint num_levels = porder->num_levels;

    memset(levels, 0, num_levels * sizeof(*levels));
    /* Count the number of threshold elements with each value. */
    for (i = 0; i < size; i++) {
        uint value = max(1, thresholds[i]);

        if (value + 1 < num_levels)
            levels[value + 1]++;
    }
    for (i = 2; i < num_levels; ++i)
        levels[i] += levels[i - 1];
    /* Now construct the actual order. */
    {
        uint width = porder->width;
        uint padding = bitmap_raster(width) * 8 - width;

        for (i = 0; i < size; i++) {
            uint value = max(1, thresholds[i]);

            bits[levels[value]++] = i + (i / width * padding);
        }
    }

    /* Check whether this is a predefined halftone. */
    {
        const gx_dht_proc *phtrp = gx_device_halftone_list;

        for (; *phtrp; ++phtrp) {
            const gx_device_halftone_resource_t *const *pphtr = (*phtrp)();
            const gx_device_halftone_resource_t *phtr;

            while ((phtr = *pphtr++) != 0) {
                if (phtr->Width == porder->width &&
                    phtr->Height == porder->height &&
                    phtr->elt_size == sizeof(ushort) &&
                    !memcmp(phtr->levels, levels, num_levels * sizeof(*levels)) &&
                    !memcmp(phtr->bit_data, porder->bit_data,
                            (size_t)size * phtr->elt_size)
                    ) {
                    /*
                     * This is a predefined halftone.  Free the levels and
                     * bit_data arrays, replacing them with the built-in ones.
                     */
                    if (porder->data_memory) {
                        gs_free_object(porder->data_memory, porder->bit_data,
                                       "construct_ht_order_short(bit_data)");
                        gs_free_object(porder->data_memory, porder->levels,
                                       "construct_ht_order_short(levels)");
                    }
                    porder->data_memory = 0;
                    porder->levels = (uint *)phtr->levels; /* actually const */
                    porder->bit_data = (void *)phtr->bit_data; /* actually const */
                    goto out;
                }
            }
        }
    }
#if DUMP_TOS
/* Lets look at the bit data which is the TOS level by level if I understand what the above
   code is supposed to be doing */
    {
        char file_name[50];
        gp_file *fid;

        snprintf(file_name, 50, "TOS_porder_%dx%d.raw", porder->width, porder->height);
        fid = gp_fopen(porder->data_memory, file_name, "wb");
        if (fid) {
            gp_fwrite(porder->bit_data, sizeof(unsigned short), size, fid);
            gp_fclose(fid);
        }
    }
#endif

 out:
    return 0;
}

/*
 * Construct a uint-representation order from a threshold array.
 * Uses porder->width, num_levels, num_bits, levels, bit_data;
 * sets porder->levels[], bit_data[].
 */
static int
construct_ht_order_uint(gx_ht_order *porder, const byte *thresholds)
{
    uint size = porder->num_bits;
    uint i;
    uint *bits = (uint *)porder->bit_data;
    uint *levels = porder->levels;
    uint num_levels = porder->num_levels;

    memset(levels, 0, num_levels * sizeof(*levels));

    /* Count the number of threshold elements with each value. */
    for (i = 0; i < size; i++) {
        uint value = max(1, thresholds[i]);

        if (value + 1 < num_levels)
            levels[value + 1]++;
    }
    for (i = 2; i < num_levels; ++i)
        levels[i] += levels[i - 1];
    /* Now construct the actual order. */
    {
        uint width = porder->width;
        uint padding = bitmap_raster(width) * 8 - width;

        for (i = 0; i < size; i++) {
            uint value = max(1, thresholds[i]);

            bits[levels[value]++] = i + (i / width * padding);
        }
    }

    /* Check whether this is a predefined halftone. */
    {
        const gx_dht_proc *phtrp = gx_device_halftone_list;

        for (; *phtrp; ++phtrp) {
            const gx_device_halftone_resource_t *const *pphtr = (*phtrp)();
            const gx_device_halftone_resource_t *phtr;

            while ((phtr = *pphtr++) != 0) {
                if (phtr->Width == porder->width &&
                    phtr->Height == porder->height &&
                    phtr->elt_size == sizeof(uint) &&
                    !memcmp(phtr->levels, levels, num_levels * sizeof(*levels)) &&
                    !memcmp(phtr->bit_data, porder->bit_data,
                        (size_t)size * phtr->elt_size)
                    ) {
                    /*
                     * This is a predefined halftone.  Free the levels and
                     * bit_data arrays, replacing them with the built-in ones.
                     */
                    if (porder->data_memory) {
                        gs_free_object(porder->data_memory, porder->bit_data,
                            "construct_ht_order_uint(bit_data)");
                        gs_free_object(porder->data_memory, porder->levels,
                            "construct_ht_order_uint(levels)");
                    }
                    porder->data_memory = 0;
                    porder->levels = (uint *)phtr->levels; /* actually const */
                    porder->bit_data = (void *)phtr->bit_data; /* actually const */
                    goto out;
                }
            }
        }
    }
out:
    return 0;
}

/* Return the bit coordinate using the standard representation. */
static int
ht_bit_index_default(const gx_ht_order *porder, uint index, gs_int_point *ppt)
{
    const gx_ht_bit *phtb = &((const gx_ht_bit *)porder->bit_data)[index];
    uint offset = phtb->offset;
    int bit = 0;

    while (!(((const byte *)&phtb->mask)[bit >> 3] & (0x80 >> (bit & 7))))
        ++bit;
    ppt->x = (offset % porder->raster * 8) + bit;
    ppt->y = offset / porder->raster;
    return 0;
}

/* Return the bit coordinate using the short representation. */
static int
ht_bit_index_short(const gx_ht_order *porder, uint index, gs_int_point *ppt)
{
    uint bit_index = ((const ushort *)porder->bit_data)[index];
    uint bit_raster = porder->raster * 8;

    ppt->x = bit_index % bit_raster;
    ppt->y = bit_index / bit_raster;
    return 0;
}

/* Return the bit coordinate using the uint representation. */
static int
ht_bit_index_uint(const gx_ht_order *porder, uint index, gs_int_point *ppt)
{
    uint bit_index = ((const uint *)porder->bit_data)[index];
    uint bit_raster = porder->raster * 8;

    ppt->x = bit_index % bit_raster;
    ppt->y = bit_index / bit_raster;
    return 0;
}

/* Update a halftone tile using the default order representation. */
static int
render_ht_default(gx_ht_tile *pbt, int level, const gx_ht_order *porder)
{
    int old_level = pbt->level;
    register const gx_ht_bit *p =
        (const gx_ht_bit *)porder->bit_data + old_level;
    register byte *data = pbt->tiles.data;

    /*
     * Invert bits between the two levels.  Note that we can use the same
     * loop to turn bits either on or off, using xor.  The Borland compiler
     * generates truly dreadful code if we don't use a temporary, and it
     * doesn't hurt better compilers, so we always use one.
     */
#define INVERT_DATA(i)\
     BEGIN\
       ht_mask_t *dp = (ht_mask_t *)&data[p[i].offset];\
       *dp ^= p[i].mask;\
     END
#ifdef DEBUG
#  define INVERT(i)\
     BEGIN\
       if_debug3('H', "[H]invert level=%d offset=%u mask=0x%x\n",\
                 (int)(p + i - (const gx_ht_bit *)porder->bit_data),\
                 p[i].offset, p[i].mask);\
       INVERT_DATA(i);\
     END
#else
#  define INVERT(i) INVERT_DATA(i)
#endif
  sw:switch (level - old_level) {
        default:
            if (level > old_level) {
                INVERT(0); INVERT(1); INVERT(2); INVERT(3);
                p += 4; old_level += 4;
            } else {
                INVERT(-1); INVERT(-2); INVERT(-3); INVERT(-4);
                p -= 4; old_level -= 4;
            }
            goto sw;
        case 7: INVERT(6);
        case 6: INVERT(5);
        case 5: INVERT(4);
        case 4: INVERT(3);
        case 3: INVERT(2);
        case 2: INVERT(1);
        case 1: INVERT(0);
        case 0: break;		/* Shouldn't happen! */
        case -7: INVERT(-7);
        case -6: INVERT(-6);
        case -5: INVERT(-5);
        case -4: INVERT(-4);
        case -3: INVERT(-3);
        case -2: INVERT(-2);
        case -1: INVERT(-1);
    }
#undef INVERT_DATA
#undef INVERT
    return 0;
}

/* Update a halftone tile using the short representation. */
static int
render_ht_short(gx_ht_tile *pbt, int level, const gx_ht_order *porder)
{
    int old_level = pbt->level;
    register const ushort *p = (const ushort *)porder->bit_data + old_level;
    register byte *data = pbt->tiles.data;

    /* Invert bits between the two levels. */
#define INVERT_DATA(i)\
     BEGIN\
       uint bit_index = p[i];\
       byte *dp = &data[bit_index >> 3];\
       *dp ^= 0x80 >> (bit_index & 7);\
     END
#ifdef DEBUG
#  define INVERT(i)\
     BEGIN\
       if_debug3('H', "[H]invert level=%d offset=%u mask=0x%x\n",\
                 (int)(p + i - (const ushort *)porder->bit_data),\
                 p[i] >> 3, 0x80 >> (p[i] & 7));\
       INVERT_DATA(i);\
     END
#else
#  define INVERT(i) INVERT_DATA(i)
#endif
  sw:switch (level - old_level) {
        default:
            if (level > old_level) {
                INVERT(0); INVERT(1); INVERT(2); INVERT(3);
                p += 4; old_level += 4;
            } else {
                INVERT(-1); INVERT(-2); INVERT(-3); INVERT(-4);
                p -= 4; old_level -= 4;
            }
            goto sw;
        case 7: INVERT(6);
        case 6: INVERT(5);
        case 5: INVERT(4);
        case 4: INVERT(3);
        case 3: INVERT(2);
        case 2: INVERT(1);
        case 1: INVERT(0);
        case 0: break;		/* Shouldn't happen! */
        case -7: INVERT(-7);
        case -6: INVERT(-6);
        case -5: INVERT(-5);
        case -4: INVERT(-4);
        case -3: INVERT(-3);
        case -2: INVERT(-2);
        case -1: INVERT(-1);
    }
#undef INVERT_DATA
#undef INVERT
    return 0;
}

/* Update a halftone tile using the uint representation. */
static int
render_ht_uint(gx_ht_tile *pbt, int level, const gx_ht_order *porder)
{
    int old_level = pbt->level;
    register const uint *p = (const uint *)porder->bit_data + old_level;
    register byte *data = pbt->tiles.data;

    /* Invert bits between the two levels. */
#define INVERT_DATA(i)\
     BEGIN\
       uint bit_index = p[i];\
       byte *dp = &data[bit_index >> 3];\
       *dp ^= 0x80 >> (bit_index & 7);\
     END
#ifdef DEBUG
#  define INVERT(i)\
     BEGIN\
       if_debug3('H', "[H]invert level=%d offset=%u mask=0x%x\n",\
                 (int)(p + i - (const uint *)porder->bit_data),\
                 p[i] >> 3, 0x80 >> (p[i] & 7));\
       INVERT_DATA(i);\
     END
#else
#  define INVERT(i) INVERT_DATA(i)
#endif
sw:switch (level - old_level) {
default:
    if (level > old_level) {
        INVERT(0); INVERT(1); INVERT(2); INVERT(3);
        p += 4; old_level += 4;
    }
    else {
        INVERT(-1); INVERT(-2); INVERT(-3); INVERT(-4);
        p -= 4; old_level -= 4;
    }
    goto sw;
case 7: INVERT(6);
case 6: INVERT(5);
case 5: INVERT(4);
case 4: INVERT(3);
case 3: INVERT(2);
case 2: INVERT(1);
case 1: INVERT(0);
case 0: break;		/* Shouldn't happen! */
case -7: INVERT(-7);
case -6: INVERT(-6);
case -5: INVERT(-5);
case -4: INVERT(-4);
case -3: INVERT(-3);
case -2: INVERT(-2);
case -1: INVERT(-1);
}
#undef INVERT_DATA
#undef INVERT
return 0;
}

/* Define the procedure vectors for the order data implementations. */
const gx_ht_order_procs_t ht_order_procs_table[3] = {
    { sizeof(gx_ht_bit), construct_ht_order_default, ht_bit_index_default,
      render_ht_default },
    { sizeof(ushort), construct_ht_order_short, ht_bit_index_short,
      render_ht_short },
    { sizeof(uint), construct_ht_order_uint, ht_bit_index_uint,
      render_ht_uint }
};
