/* Copyright (C) 2001-2020 Artifex Software, Inc.
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

#ifndef PDF_UTF8
#define PDF_UTF8

#include "ghostpdf.h"
#include "pdf_types.h"
#include "pdf_stack.h"

int locale_to_utf8(pdf_context *ctx, pdf_string *input, pdf_string **output);

#endif
