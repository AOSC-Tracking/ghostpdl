/* Portions Copyright (C) 2001 artofcode LLC.
   Portions Copyright (C) 1996, 2001 Artifex Software Inc.
   Portions Copyright (C) 1988, 2000 Aladdin Enterprises.
   This software is based in part on the work of the Independent JPEG Group.
   All Rights Reserved.

   This software is distributed under license and may not be copied, modified
   or distributed except as expressly authorized under the terms of that
   license.  Refer to licensing information at http://www.artifex.com/ or
   contact Artifex Software, Inc., 101 Lucas Valley Road #110,
   San Rafael, CA  94903, (415)492-9861, for further information. */

/*$RCSfile$ $Revision$ */
/* Client interface to DeviceN color */

#ifndef gscdevn_INCLUDED
#  define gscdevn_INCLUDED

#include "gscspace.h"

/*
 * Fill in a DeviceN color space.  Does not include allocation
 * and initialization of the color space.
 * Note that the client is responsible for memory management of the
 * tint transform Function.
 */
int gs_build_DeviceN(
			gs_color_space *pcspace,
			uint num_components,
			const gs_color_space *palt_cspace,
			gs_memory_t *pmem
			);
/*
 * Allocate and fill in a DeviceN color space.
 * Note that the client is responsible for memory management of the
 * tint transform Function.
 */
int gs_cspace_build_DeviceN(
			       gs_color_space **ppcspace,
			       gs_separation_name *psnames,
			       uint num_components,
			       const gs_color_space *palt_cspace,
			       gs_memory_t *pmem
			       );

/* Set the tint transformation procedure for a DeviceN color space. */
/* VMS limits procedure names to 31 characters, and some systems only */
/* compare the first 23 characters. */
extern int gs_cspace_set_devn_proc(const gs_memory_t *mem,
				   gs_color_space * pcspace,
				   int (*proc)(const float *,
					       float *,
					       const gs_imager_state *,
					       void *
					       ),
				   void *proc_data
				   );

/* Set the DeviceN tint transformation procedure to a Function. */
#ifndef gs_function_DEFINED
typedef struct gs_function_s gs_function_t;
#  define gs_function_DEFINED
#endif
int gs_cspace_set_devn_function(const gs_memory_t *mem,
				gs_color_space *pcspace,
				gs_function_t *pfn);

/*
 * If the DeviceN tint transformation procedure is a Function,
 * return the function object, otherwise return 0.
 */
gs_function_t *gs_cspace_get_devn_function(const gs_color_space *pcspace);

/* Map a DeviceN color using a Function. */
int map_devn_using_function(const float *in, float *out,
			const gs_imager_state *pis, void *data);

#endif /* gscdevn_INCLUDED */
