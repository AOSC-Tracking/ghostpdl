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

/*Id: gsfcmap.h  */
/* CMap data definition */
/* Requires gsstruct.h */

#ifndef gsfcmap_INCLUDED
#  define gsfcmap_INCLUDED

#include "gsccode.h"

/* Define the abstract type for a CMap. */
#ifndef gs_cmap_DEFINED
#  define gs_cmap_DEFINED
typedef struct gs_cmap_s gs_cmap;

#endif

/* We only need the structure descriptor for testing. */
extern_st(st_cmap);

/* Define the structure for CIDSystemInfo. */
typedef struct gs_cid_system_info_s {
    gs_const_string Registry;
    gs_const_string Ordering;
    int Supplement;
} gs_cid_system_info;
/* We only need the structure descriptor for embedding. */
extern_st(st_cid_system_info);
#define public_st_cid_system_info() /* in gsfcmap.c */\
  gs_public_st_const_strings2(st_cid_system_info, gs_cid_system_info,\
    "gs_cid_system_info", cid_si_enum_ptrs, cid_si_reloc_ptrs,\
    Registry, Ordering)

/* ---------------- Procedural interface ---------------- */

/*
 * Decode a character from a string using a CMap, updating the index.
 * Return 0 for a CID or name, N > 0 for a character code where N is the
 * number of bytes in the code, or an error.  For undefined characters,
 * we return CID 0.
 */
int gs_cmap_decode_next(P6(const gs_cmap * pcmap, const gs_const_string * str,
			   uint * pindex, uint * pfidx,
			   gs_char * pchr, gs_glyph * pglyph));

#endif /* gsfcmap_INCLUDED */
