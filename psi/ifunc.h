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


/* Internal interpreter interfaces for Functions */

#ifndef ifunc_INCLUDED
#  define ifunc_INCLUDED

#include "gsfunc.h"
#include "iref.h"

/* Define build procedures for the various function types. */
#define build_function_proc(proc)\
  int proc(i_ctx_t *i_ctx_p, const ref *op, const gs_function_params_t *params, int depth,\
           gs_function_t **ppfn, gs_memory_t *mem)
typedef build_function_proc((*build_function_proc_t));

/* Define the table of build procedures, indexed by FunctionType. */
typedef struct build_function_type_s {
    int type;
    build_function_proc_t proc;
} build_function_type_t;
extern const build_function_type_t build_function_type_table[];
extern const uint build_function_type_table_count;

/* Build a function structure from a PostScript dictionary. */
int fn_build_function(i_ctx_t *i_ctx_p, const ref * op, gs_function_t ** ppfn,
      gs_memory_t *mem, const float *shading_domain, const int num_inputs);
int fn_build_sub_function(i_ctx_t *i_ctx_p, const ref * op, gs_function_t ** ppfn,
  int depth, gs_memory_t *mem, const float *shading_domain, const int num_inputs);

/* Called only from the routines in zcolor.c, in order to convert a tint-transform
 * procedure to a function. Little more than a place-holder to avoid making a number
 * of functions non-static
 */
int buildfunction(i_ctx_t * i_ctx_p, ref *arr, ref *pproc, int type);

/*
 * Collect a heap-allocated array of floats.  If the key is missing, set
 * *pparray = 0 and return 0; otherwise set *pparray and return the number
 * of elements.  Note that 0-length arrays are acceptable, so if the value
 * returned is 0, the caller must check whether *pparray == 0.
 */
int fn_build_float_array(const ref * op, const char *kstr, bool required,
                         bool even, const float **pparray,
                         gs_memory_t *mem);

/*
 * Similar to fn_build_float_array() except
 * - numeric parameter is accepted and converted to 1-element array
 * - number of elements is not checked for even/odd
 */
int
fn_build_float_array_forced(const ref * op, const char *kstr, bool required,
                     const float **pparray, gs_memory_t *mem);

/*
 * If a PostScript object is a Function procedure, return the function
 * object, otherwise return 0.
 */
gs_function_t *ref_function(const ref *op);

/*
 * Operator to execute a function.
 * <in1> ... <function_struct> %execfunction <out1> ...
 */
int zexecfunction(i_ctx_t *);

#endif /* ifunc_INCLUDED */
