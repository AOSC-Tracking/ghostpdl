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



#ifndef zchar42_INCLUDED
#  define zchar42_INCLUDED

#include "iref.h"
#include "gxfapi.h"

/* Get a Type 42 character metrics and set the cache device. */
int zchar42_set_cache(i_ctx_t *i_ctx_p, gs_font_base *pbfont, ref *cnref,
            uint glyph_index, op_proc_t cont, op_proc_t *exec_cont);

#endif /* zchar42_INCLUDED */
