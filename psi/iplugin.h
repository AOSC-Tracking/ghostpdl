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


/* Plugin manager */

#ifndef iplugin_INCLUDED
#define iplugin_INCLUDED

#include "iref.h"

typedef struct i_plugin_holder_s i_plugin_holder;
typedef struct i_plugin_instance_s i_plugin_instance;
typedef struct i_plugin_descriptor_s i_plugin_descriptor;
typedef struct i_plugin_client_memory_s i_plugin_client_memory;

struct i_plugin_descriptor_s { /* RTTI for plugins */
    const char *type;          /* Plugin type, such as "FAPI" */
    const char *subtype;       /* Plugin type, such as "UFST" */
    void (*finit)(i_plugin_instance *instance, i_plugin_client_memory *mem); /* Destructor & deallocator for the instance. */
};

struct i_plugin_instance_s {   /* Base class for various plugins */
    const i_plugin_descriptor *d;
};

struct i_plugin_holder_s { /* Forms list of plugins for plugin manager */
    i_plugin_holder *next;
    i_plugin_instance *I;
};

struct i_plugin_client_memory_s { /* must be copying */
    void *client_data;
    void *(*alloc)(i_plugin_client_memory *mem, unsigned int size, const char *id);
    void (*free)(i_plugin_client_memory *mem, void *data, const char *cname);
};

#define plugin_instantiation_proc(proc)\
  int proc(i_plugin_client_memory *client_mem, i_plugin_instance **instance)

#define extern_i_plugin_table()\
  typedef plugin_instantiation_proc((*i_plugin_instantiation_proc));\
  extern const i_plugin_instantiation_proc i_plugin_table[]

void i_plugin_make_memory(i_plugin_client_memory *mem, gs_memory_t *mem_raw);
int i_plugin_init(i_ctx_t *);
void i_plugin_finit(gs_memory_t *mem, i_plugin_holder *list);
i_plugin_instance *i_plugin_find(i_ctx_t *i_ctx_p, const char *type, const char *subtype);
i_plugin_holder * i_plugin_get_list(i_ctx_t *i_ctx_p);

#endif /* iplugin_INCLUDED */
