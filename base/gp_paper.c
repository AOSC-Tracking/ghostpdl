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


/* $Id:$ */
/* Standard implementation of gp_defaultpapersize */
#include "gx.h"
#include "gp.h"

/* Get the system default paper size.  See gp_paper.h for details. */
int
gp_defaultpapersize(char *ptr, int *plen)
{

    /* None available */
    if (*plen > 0)
        *ptr = 0;
    *plen = 1;
    return 1;
}
