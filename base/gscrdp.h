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


/* Interface for device-specified CRDs */

#ifndef gscrdp_INCLUDED
#  define gscrdp_INCLUDED

#include "gscie.h"
#include "gsparam.h"
#include "gsdevice.h"

/*
 * A driver can provide any number of its own CRDs through (read-only)
 * device parameters whose values are slightly modified PostScript-style
 * dictionaries.  The driver doesn't need to concern itself with how the
 * parameters are encoded: it simply constructs a CRD and calls
 * param_write_cie_render1.
 *
 * Logically, the pcrd parameter for this procedure and the next one
 * should be declared as const gs_cie_render *, but the procedures may
 * cause certain cached (idempotent) values to be computed.
 */
int param_write_cie_render1(gs_param_list * plist, gs_param_name key,
                            gs_cie_render * pcrd,
                            gs_memory_t * mem);

/*
 * For internal use, we also provide an API that writes the CRD directly
 * into a parameter list, rather than as a named parameter in a larger
 * list.
 */
int param_put_cie_render1(gs_param_list * plist, gs_cie_render * pcrd,
                          gs_memory_t * mem);

/*
 * Client code that wants to initialize a CRD from a device parameter
 * uses the following complementary procedure.  The customary way to
 * use this is:

 gs_c_param_list list;
 ...
 gs_c_param_list_write(&list, mem);
 gs_c_param_request(&list, "ParamName");
 code = gs_getdeviceparams(dev, &list);
 << error if code < 0 >>
 gs_c_param_list_read(&list);
 code = gs_cie_render1_param_initialize(pcrd, &list, "ParamName", dev);
 gs_c_param_list_release(&list);
 << error if code < 0 >>

 * where "ParamName" is the parameter name, e.g., "CRDDefault".
 */
int gs_cie_render1_param_initialize(gs_cie_render * pcrd,
                                    gs_param_list * plist,
                                    gs_param_name key,
                                    gx_device * dev);

/*
 * Again, we provide an internal procedure that doesn't involve a
 * parameter name.
 */
int param_get_cie_render1(gs_cie_render * pcrd,
                          gs_param_list * plist,
                          gx_device * dev);

/*
 * The actual representation of the CRD is a slightly modified PostScript
 * ColorRenderingType 1 dictionary.  THE FOLLOWING IS SUBJECT TO CHANGE
 * WITHOUT NOTICE.  Specifically, the following keys are different:
 *      ColorRenderingType = GX_DEVICE_CRD1_TYPE
 */
#define GX_DEVICE_CRD1_TYPE 101
/*
 *      (Instead of TransformPQR = [T1 T2 T3]:)
 *        TransformPQRName = procedure name (a name)
 *        TransformPQRData = procedure data (a string)
 *      (Instead of EncodeLMN/ABC = [E1 E2 E3]:)
 *        EncodeLMN/ABCValues = [V1,1 V1,2 ... V3,N], where Vi,j is the
 *          j'th sampled value of the i'th encoding array, mapped linearly
 *          to the corresponding domain (see gscie.h)
 *      (Instead of RenderTable = [NA NB NC table m T1 ... Tm]:)
 *        RenderTableSize = [NA NB NC m]
 *        RenderTableTable = table (an array of strings)
 *        RenderTableTValues = [V1,1 V1,2 ... Vm,N] (see above)
 * The PostScript setcolorrendering operator selects the correct operator
 * according to the ColorRenderingType key.
 */

#endif /* gscrdp_INCLUDED */
