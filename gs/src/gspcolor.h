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


/* Client interface to Pattern color */

#ifndef gspcolor_INCLUDED
#  define gspcolor_INCLUDED

#include "gsccolor.h"
#include "gsuid.h"

/* ---------------- Types and structures ---------------- */

/*
 * Unfortunately, we defined the gs_client_pattern structure before we
 * realized that we would have to accommodate multiple PatternTypes.
 * Consequently, we distinguish the different PatternTypes with a hack.
 * We know that PatternType 1 patterns always have a positive PaintType.
 * Therefore, we overlay the PaintType field of PatternType 1 patterns
 * with the negative of the PatternType for generalized patterns.
 * This allows us to distinguish PatternType 1 patterns from all others.
 * This is a really bad hack, but doing anything else would require
 * a non-backward-compatible change for clients, since we didn't
 * require clients to use a procedure to initialize Patterns (another
 * mistake, in retrospect, which we've now also fixed).
 */

/* General pattern template (called "prototype pattern" in Red Book) */
typedef struct gs_pattern_type_s gs_pattern_type_t;

#define gs_pattern_template_common\
  gs_uid uid;		/* must be first in case we ever subclass properly */\
  int negPatternType;	/* overlays PaintType, see above */\
  const gs_pattern_type_t *type
#define PatternType(ppt)\
  ((ppt)->negPatternType < 0 ? -(ppt)->negPatternType : 1)
typedef struct gs_pattern_template_s {
    gs_pattern_template_common;
} gs_pattern_template_t;

/* ---------------- Procedures ---------------- */

/* Set a Pattern color or a Pattern color space. */
int gs_setpattern(P2(gs_state *, const gs_client_color *));
int gs_setpatternspace(P1(gs_state *));

/*
 * The gs_memory_t argument for gs_make_pattern may be NULL, meaning use the
 * same allocator as for the gs_state argument.  Note that gs_make_pattern
 * uses rc_alloc_struct_1 to allocate pattern instances.
 */
int gs_make_pattern(P5(gs_client_color *, const gs_pattern_template_t *,
		       const gs_matrix *, gs_state *, gs_memory_t *));
const gs_pattern_template_t *gs_get_pattern(P1(const gs_client_color *));

/*
 * Adjust the reference count of a pattern. This is intended to support
 * applications (such as PCL) which maintain client colors outside of the
 * graphic state. Since the pattern instance structure is opaque to these
 * applications, they need some way to release or retain the instances as
 * needed.
 */
void gs_pattern_reference(P2(gs_client_color * pcc, int delta));

#endif /* gspcolor_INCLUDED */
