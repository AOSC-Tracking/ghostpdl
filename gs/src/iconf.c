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
/* Configuration-dependent tables and initialization for interpreter */
#include "stdio_.h"		/* stdio for stream.h */
#include "gstypes.h"
#include "gsmemory.h"		/* for iminst.h */
#include "gconf.h"
#include "iref.h"
#include "ivmspace.h"
#include "opdef.h"
#include "ifunc.h"
#include "iapi.h"
#include "iminst.h"
#include "iplugin.h"

/* Define the default values for an interpreter instance. */
const gs_main_instance gs_main_instance_init_values =
{gs_main_instance_default_init_values};

/* Set up the .ps file name string array. */
/* We fill in the lengths at initialization time. */
#ifdef GS_DEBUGGER
#define ref_debug unsigned long line_number; unsigned long line_number2
#else
#define ref_debug
#endif

#define ref_(t) struct { struct tas_s tas; t value; ref_debug}
#define string_(s,len)\
 { { (t_string<<r_type_shift) + a_readonly + avm_foreign, len }, s },
#define psfile_(fns,len) string_(fns,len)
const ref_(const char *) gs_init_file_array[] = {
#include "gconf.h"
    string_(0, 0)
};
#undef psfile_

/* Set up the emulator name string array similarly. */
#define emulator_(ems,len) string_(ems,len)
const ref_(const char *) gs_emulator_name_array[] = {
#include "gconf.h"
    string_(0, 0)
};
#undef emulator_

/* Set up the function type table similarly. */
#define function_type_(i,proc) extern build_function_proc(proc);
#include "gconf.h"
#undef function_type_
#define function_type_(i,proc) {i,proc},
const build_function_type_t build_function_type_table[] = {
#include "gconf.h"
    {0}
};
#undef function_type_
const uint build_function_type_table_count =
    countof(build_function_type_table) - 1;

/* Initialize the operators. */
	/* Declare the externs. */
#define oper_(xx_op_defs) extern const op_def xx_op_defs[];
oper_(interp_op_defs)		/* Interpreter operators */
#include "gconf.h"
#undef oper_
 
const op_def *const op_defs_all[] = {
#define oper_(defs) defs,
    oper_(interp_op_defs)	/* Interpreter operators */
#include "gconf.h"
#undef oper_ 
    0
};
const uint op_def_count = (countof(op_defs_all) - 1) * OP_DEFS_MAX_SIZE;

/* Set up the plugin table. */

#define plugin_(proc) extern plugin_instantiation_proc(proc);
#include "gconf.h"
#undef plugin_

extern_i_plugin_table();
#define plugin_(proc) proc,
const i_plugin_instantiation_proc i_plugin_table[] = {
#include "gconf.h"
    0
};
#undef plugin_
