/* Copyright (C) 2002-2003 artofcode LLC. All rights reserved.

   This software is distributed under license and may not be copied, modified
   or distributed except as expressly authorized under the terms of that
   license.  Refer to licensing information at http://www.artifex.com/ or
   contact Artifex Software, Inc., 101 Lucas Valley Road #110,
   San Rafael, CA  94903, (415)492-9861, for further information. */

/* $Id$ */
/*
Header for functions to serialize a type 1 font as PostScript code that can
then be passed to FreeType via the FAPI FreeType bridge.
Started by Graham Asher, 26th July 2002.
*/

#ifndef write_t1_INCLUDED
#define write_t1_INCLUDED

#include "ifapi.h"

long FF_serialize_type1_font(FAPI_font* a_fapi_font,unsigned char* a_buffer,long a_buffer_size);

#endif
