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
/* Client interface to default (C heap) allocator */
/* Requires gsmemory.h */

#ifndef gsmalloc_INCLUDED
#  define gsmalloc_INCLUDED

/* Define a memory manager that allocates directly from the C heap. */
typedef struct gs_malloc_block_s gs_malloc_block_t;
typedef struct gs_malloc_memory_s {
    gs_memory_common;
    gs_malloc_block_t *allocated;
    long limit;
    long used;
    long max_used;
} gs_malloc_memory_t;

/* Allocate and initialize a malloc memory manager. */
gs_malloc_memory_t *gs_malloc_memory_init(void);

/* Release all the allocated blocks, and free the memory manager. */
/* The cast is unfortunate, but unavoidable. */
#define gs_malloc_memory_release(mem)\
  gs_memory_free_all((gs_memory_t *)mem, FREE_ALL_EVERYTHING,\
		     "gs_malloc_memory_release")

/*
 * Define a default allocator that allocates from the C heap.
 * (We would really like to get rid of this.)
 */
extern gs_malloc_memory_t *gs_malloc_memory_default;
extern gs_memory_t *gs_memory_t_default;  /* may be locked */
#define gs_memory_default (*gs_memory_t_default)

/*
 * The following procedures are historical artifacts that we hope to
 * get rid of someday.
 */
gs_memory_t * gs_malloc_init(const gs_memory_t *parent);
void gs_malloc_release(void);
#define gs_malloc(nelts, esize, cname)\
  (void *)gs_alloc_byte_array(&gs_memory_default, nelts, esize, cname)
#define gs_free(data, nelts, esize, cname)\
  gs_free_object(&gs_memory_default, data, cname)

/* Define an accessor for the limit on the total allocated heap space. */
#define gs_malloc_limit (gs_malloc_memory_default->limit)

/* Define an accessor for the maximum amount ever allocated from the heap. */
#define gs_malloc_max (gs_malloc_memory_default->max_used)

/* ---------------- Locking ---------------- */

/* Create a locked wrapper for a heap allocator. */
int gs_malloc_wrap(gs_memory_t **wrapped, gs_malloc_memory_t *contents);

/* Get the wrapped contents. */
gs_malloc_memory_t *gs_malloc_wrapped_contents(gs_memory_t *wrapped);

/* Free the wrapper, and return the wrapped contents. */
gs_malloc_memory_t *gs_malloc_unwrap(gs_memory_t *wrapped);

#endif /* gsmalloc_INCLUDED */
