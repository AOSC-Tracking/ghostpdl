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


/* Private Adobe Type 1 / Type 2 charstring interpreter definitions */

#ifndef gxtype1_INCLUDED
#  define gxtype1_INCLUDED

#include "gscrypt1.h"
#include "gsgdata.h"
#include "gstype1.h"
#include "gxhintn.h"
#include "gxmatrix.h"
#include "gspath.h"
#include "gzpath.h"

/* This file defines the structures for the state of a Type 1 / */
/* Type 2 charstring interpreter. */

/*
 * Because of oversampling, one pixel in the Type 1 interpreter may
 * correspond to several device pixels.  This is also true of the hint data,
 * since the CTM reflects the transformation to the oversampled space.
 * To help keep the font level hints separated from the character level hints,
 * we store the scaling factor separately with each set of hints.
 */
typedef struct pixel_scale_s {
    fixed unit;			/* # of pixels per device pixel */
    fixed half;			/* unit / 2 */
    int log2_unit;		/* log2(unit / fixed_1) */
} pixel_scale;
typedef struct point_scale_s {
    pixel_scale x, y;
} point_scale;

#define set_pixel_scale(pps, log2)\
  (pps)->unit = ((pps)->half = fixed_half << ((pps)->log2_unit = log2)) << 1
#define scaled_rounded(v, pps)\
  (((v) + (pps)->half) & -(pps)->unit)

/*
 * The Type 2 charstring documentation says that the total number of hints
 * is limited to 96.
 */

#define max_total_stem_hints 96

/* ------ Interpreter state ------ */

/* Define the control state of the interpreter. */
/* This is what must be saved and restored */
/* when calling a CharString subroutine. */
typedef struct {
    const byte *ip;
    const byte *ip_end;
    crypt_state dstate;
    gs_glyph_data_t cs_data;	/* original CharString or Subr, */
                                /* for GC */
} ip_state_t;

/* Get the next byte from a CharString.  It may or may not be encrypted. */
#define charstring_this(ch, state, encrypted)\
  (encrypted ? decrypt_this(ch, state) : ch)
#define charstring_next(ch, state, chvar, encrypted)\
  (encrypted ? (chvar = decrypt_this(ch, state),\
                decrypt_skip_next(ch, state)) :\
   (chvar = ch))
#define charstring_skip_next(ch, state, encrypted)\
  (encrypted ? decrypt_skip_next(ch, state) : 0)

/* This is the full state of the Type 1 interpreter. */
#define ostack_size 48		/* per Type 2 documentation */
#define ipstack_size 10		/* per documentation */
struct gs_type1_state_s {
    t1_hinter h;
    /* The following are set at initialization */
    gs_font_type1 *pfont;	/* font-specific data */
    gs_gstate *pgs;              /* gs_gstate */
    gx_path *path;		/* path for appending */
    bool no_grid_fitting;
    int paint_type;		/* 0/3 for fill, 1/2 for stroke */
    void *callback_data;
    fixed_coeff fc;		/* cached fixed coefficients */
    float flatness;		/* flatness for character curves */
    point_scale scale;		/* oversampling scale */
    gs_log2_scale_point log2_subpixels;	/* log2 of the number of subpixels */
    gs_fixed_point origin;	/* character origin */
    /* The following are updated dynamically */
    fixed ostack[ostack_size];	/* the Type 1 operand stack */
    int os_count;		/* # of occupied stack entries */
    ip_state_t ipstack[ipstack_size + 1];	/* control stack */
    int ips_count;		/* # of occupied entries */
    int init_done;		/* -1 if not done & not needed, */
                                /* 0 if not done & needed, 1 if done */
    bool sb_set;		/* true if lsb is preset */
    bool width_set;		/* true if width is set (for seac parts) */
    bool seac_flag;		/* true if executing the accent */
    /* (Type 2 charstrings only) */
    int num_hints;		/* number of hints (Type 2 only) */
    gs_fixed_point lsb;		/* left side bearing (design coords) */
    gs_fixed_point width;	/* character width (design coords) */
    int seac_accent;		/* accent character code for seac, or -1 */
    fixed asb;			/* the asb parameter of seac */
    gs_fixed_point compound_lsb;/* lsb of the compound character
                                   (i.e. of the accented character
                                   defined with seac). */
    gs_fixed_point save_adxy;	/* passes seac adx/ady across the base character. */
    fixed asb_diff;		/* asb - compound_lsb.x, */
                                /* needed to adjust Flex endpoint
                                   when processing the accent character;
                                   Zero when processing the base character. */
    gs_fixed_point adxy;	/* seac accent displacement,
                                   needed to adjust currentpoint
                                   when processing the accent character;
                                   Zero when processing the base character. */
    fixed base_lsb;		/* The lsb of the base character for 'seac'. */
    int flex_path_state_flags;	/* record whether path was open */
                                /* at start of Flex section */
    gs_fixed_point origin_offset; /* Origin offset due to replaced metrics. */
#define flex_max 8
    int flex_count;
    int ignore_pops;		/* # of pops to ignore (after */
                                /* a known othersubr call) */
    /* The following are set dynamically. */
    gs_fixed_point vs_offset;	/* device space offset for centering */
                                /* middle stem of vstem3 */
                                /* of subpath */
    fixed transient_array[32];	/* Type 2 transient array, */
    /* will be variable-size someday */
};

extern_st(st_gs_type1_state);
#define public_st_gs_type1_state() /* in gxtype1.c */\
  gs_public_st_composite(st_gs_type1_state, gs_type1_state, "gs_type1_state",\
    gs_type1_state_enum_ptrs, gs_type1_state_reloc_ptrs)

/* ------ Shared Type 1 / Type 2 interpreter fragments ------ */

/* Define a pointer to the charstring interpreter stack. */
typedef fixed *cs_ptr;

/* Clear the operand stack. */
/* The cast avoids compiler warning about a "negative subscript." */
#define CLEAR_CSTACK(cstack, csp)\
  (csp = (cs_ptr)(cstack) - 1)

/* Copy the operand stack out of the saved state. */
#define INIT_CSTACK(cstack, csp, pcis)\
  BEGIN\
    memset(cstack, 0x00, sizeof(cstack));\
    if ( pcis->os_count == 0 )\
      CLEAR_CSTACK(cstack, csp);\
    else {\
      memcpy(cstack, pcis->ostack, pcis->os_count * sizeof(fixed));\
      csp = &cstack[pcis->os_count - 1];\
    }\
  END
#define CS_CHECK_CSTACK_BOUNDS(csaddr, cs) \
      (csaddr >= &(cs[0]) && \
        csaddr < &(cs[ostack_size]))

#define CS_CHECK_TRANSIENT_BOUNDS(csaddr, cs) \
      (csaddr >= &(cs[0]) && \
        csaddr < &(cs[32]))         /* size defined in gs_type1_state_s above */

#define CS_CHECK_PUSH(csp, cstack)\
  BEGIN\
    if (csp >= &cstack[countof(cstack)-1])\
      return_error(gs_error_invalidfont);\
  END

#define CS_CHECK_PUSHN(csp, cstack, n)\
  BEGIN\
    if (csp >= &cstack[countof(cstack) - n])\
      return_error(gs_error_invalidfont);\
  END

#define CS_CHECK_POP(csp, cstack)\
  BEGIN\
    if (csp < &cstack[0])\
      return_error(gs_error_invalidfont);\
  END

#define CS_CHECK_IPSTACK(ips, ipstack)\
  BEGIN\
    if (ips > &ipstack[ipstack_size] \
        || ips < &ipstack[0])\
      return_error(gs_error_invalidfont);\
  END

/* Decode a 1-byte number. */
#define decode_num1(var, c)\
  (var = c_value_num1(c))
#define decode_push_num1(csp, cstack, c)\
  BEGIN\
    CS_CHECK_PUSH(csp, cstack);\
    *++csp = int2fixed(c_value_num1(c));\
  END

/* Decode a 2-byte number. */
#define decode_num2(var, c, cip, state, encrypted)\
  BEGIN\
    uint c2 = *cip++;\
    int cn = charstring_this(c2, state, encrypted);\
\
    var = (c < c_neg2_0 ? c_value_pos2(c, 0) + cn :\
           c_value_neg2(c, 0) - cn);\
    charstring_skip_next(c2, state, encrypted);\
  END
#define decode_push_num2(csp, cstack, c, cip, state, encrypted)\
  BEGIN\
    uint c2 = *cip++;\
    int cn;\
\
    CS_CHECK_PUSH(csp, cstack);\
    cn = charstring_this(c2, state, encrypted);\
    if ( c < c_neg2_0 )\
      { if_debug2('1', "[1] (%d)+%d\n", c_value_pos2(c, 0), cn);\
        *++csp = int2fixed(c_value_pos2(c, 0) + (int)cn);\
      }\
    else\
      { if_debug2('1', "[1] (%d)-%d\n", c_value_neg2(c, 0), cn);\
        *++csp = int2fixed(c_value_neg2(c, 0) - (int)cn);\
      }\
    charstring_skip_next(c2, state, encrypted);\
  END

/* Decode a 4-byte number, but don't push it, because Type 1 and Type 2 */
/* charstrings scale it differently. */
#if ARCH_SIZEOF_LONG > 4
#  define sign_extend_num4(lw)\
     lw = (lw ^ 0x80000000L) - 0x80000000L
#else
#  define sign_extend_num4(lw) DO_NOTHING
#endif
#define decode_num4(lw, cip, state, encrypted)\
  BEGIN\
    int i;\
    uint c4;\
\
    lw = 0;\
    for ( i = 4; --i >= 0; )\
      { charstring_next(*cip, state, c4, encrypted);\
        lw = (lw << 8) + c4;\
        cip++;\
      }\
    sign_extend_num4(lw);\
  END

int gs_type1_check_float(crypt_state *state, bool encrypted, const byte **cip, cs_ptr csp, long lw);

/* ------ Shared Type 1 / Type 2 charstring utilities ------ */

void gs_type1_finish_init(gs_type1_state * pcis);

int gs_type1_sbw(gs_type1_state * pcis, fixed sbx, fixed sby,
                 fixed wx, fixed wy);

/* blend returns the number of values to pop. */
int gs_type1_blend(gs_type1_state *pcis, fixed *csp, int num_results);

int gs_type1_seac(gs_type1_state * pcis, const fixed * cstack,
                  fixed asb_diff, ip_state_t * ipsp);

int gs_type1_endchar(gs_type1_state * pcis);

/* Get the metrics (l.s.b. and width) from the Type 1 interpreter. */
void type1_cis_get_metrics(const gs_type1_state * pcis, double psbw[4]);

#endif /* gxtype1_INCLUDED */
