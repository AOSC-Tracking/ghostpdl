/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.

   This file is part of Aladdin Ghostscript.

   Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
   or distributor accepts any responsibility for the consequences of using it,
   or for whether it serves any particular purpose or works at all, unless he
   or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
   License (the "License") for full details.

   Every copy of Aladdin Ghostscript must include a copy of the License,
   normally in a plain ASCII text file named PUBLIC.  The License grants you
   the right to copy, modify and redistribute Aladdin Ghostscript, but only
   under certain conditions described in the License.  Among other things, the
   License requires that the copyright notice and this notice be preserved on
   all copies.
 */

/*Id: gxpageq.h  */
/* Page queue implementation */

/* Initial version 2/1/98 by John Desrosiers (soho@crl.com) */
/* Edited to Ghostscript coding standards 7/17/98 L. Peter Deutsch
   (ghost@aladdin.com) */

#ifndef gxpageq_INCLUDED
# define gxpageq_INCLUDED

# include "gsmemory.h"
# include "gxband.h"
# include "gxsync.h"

/* --------------- Data type definitions --------------------- */

typedef enum {
    gx_page_queue_action_partial_page,
    gx_page_queue_action_full_page,
    gx_page_queue_action_copy_page,
    gx_page_queue_action_terminate
} gx_page_queue_action_t;

#ifndef gx_page_queue_DEFINED
# define gx_page_queue_DEFINED
typedef struct gx_page_queue_s gx_page_queue;
#endif

/*
 * Define a page queue entry object.
 */
typedef struct gx_page_queue_entry_s {
    gx_band_page_info page_info;
    gx_page_queue_action_t action;	/* action code */
    int num_copies;		/* number of copies to render */
    struct gx_page_queue_entry_s *next;		/* link to next in queue */
    gx_page_queue *queue;	/* link to queue the entry is in */
} gx_page_queue_entry;

#define private_st_gx_page_queue_entry()\
  gs_private_st_ptrs2(st_gx_page_queue_entry, gx_page_queue_entry,\
    "gx_page_queue_entry",\
    gx_page_queue_entry_enum_ptrs, gx_page_queue_entry_reloc_ptrs,\
    next, queue)

/*
 * Define the structure used to manage a page queue
 * A page queue is a monitor-locked FIFO which holds completed command
 * list files ready for rendering.
 */
struct gx_page_queue_s {
    gs_memory_t *memory;	/* allocator used to allocate entries */
    gx_monitor_t *monitor;	/* used to serialize access to this structure */
    int entry_count;		/* # elements in page_queue */
    bool dequeue_in_progress;	/* true between start/ & end_dequeue */
    gx_semaphore_t *render_req_sema;	/* sema signalled when page queued */
    bool enable_render_done_signal;	/* enable signals to render_done_sema */
    gx_semaphore_t *render_done_sema;	/* semaphore signaled when (partial) page rendered */
    gx_page_queue_entry *last_in;	/* if <> 0, Last-in queue entry */
    gx_page_queue_entry *first_in;	/* if <> 0, First-in queue entry */
    gx_page_queue_entry *reserve_entry;		/* spare allocation */
};

#define private_st_gx_page_queue()\
  gs_private_st_ptrs4(st_gx_page_queue, gx_page_queue, "gx_page_queue",\
    gx_page_queue_enum_ptrs, gx_page_queue_reloc_ptrs,\
    monitor, first_in, last_in, reserve_entry);

/* -------------- Public Procedure Declaraions --------------------- */

/* All page queue entries must be allocated by this routine. Allocated */
/* entries are initialized & ready to go */
/* rets ptr to allocated object, 0 if VM error */
gx_page_queue_entry *
gx_page_queue_entry_alloc(P1(
    gx_page_queue * queue	/* queue that entry is being alloc'd for */
    ));

/* All page queues entries must be destroyed by this routine */
void gx_page_queue_entry_free(P1(
    gx_page_queue_entry * entry		/* entry to free up */
    ));

/* Init a page queue; this must be done before it can be used. This routine */
/* allocates & inits various necessary structures and will fail if insufficient */
/* memory is available. */
/* -ve error code, or 0 */
int gx_page_queue_init(P2(
    gx_page_queue * queue,	/* page queue to init */
    gs_memory_t * memory	/* allocator for dynamic memory */
    ));

/* Destroy a page queue which was initialized by gx_page_queue_init. Any */
/* page queue entries in the queue are released and destroyed; dynamic */
/* allocations are released. */
void gx_page_queue_dnit(P1(
    gx_page_queue * queue	/* page queue to dnit */
    ));

/* If there are any pages in queue, wait until one of them finishes rendering. */
/* Typically called by writer's out-of-memory error handlers that want to wait */
/* until some memory has been freed. */
/* rets 0 if no pages were waiting for rendering, 1 if actually waited */
int gx_page_queue_wait_one_page(P1(
    gx_page_queue * queue	/* queue to wait on */
    ));

/* Wait until all (if any) pages in queue have finished rendering. Typically */
/* called by writer operations which need to drain the page queue before */
/* continuing. */
void gx_page_queue_wait_until_empty(P1(
    gx_page_queue * queue		/* page queue to wait on */
    ));

/* Add a pageq queue entry to the end of the page queue. If an unsatisfied */
/* reader thread has an outstanding gx_page_queue_start_deque(), wake it up. */
void gx_page_queue_enqueue(P1(
    gx_page_queue_entry * entry	/* entry to add */
    ));

/* Allocate & construct a pageq entry, then to the end of the pageq as */
/* in gx_page_queue_enqueue. If unable to allocate a new pageq entry, uses */
/* the pre-allocated reserve entry held in the pageq. When using the reserve */
/* pageq entry, wait until enough pages have been rendered to allocate a new */
/* reserve for next time -- this should always succeed & returns eFatal if not. */
/* Unless the reserve was used, does not wait for any rendering to complete. */
/* Typically called by writer when it has a (partial) page ready for rendering. */
/* rets 0 ok, gs_error_Fatal if error */
int gx_page_queue_add_page(P4(
    gx_page_queue * queue,		/* page queue to add to */
    gx_page_queue_action_t action,		/* action code to queue */
    const gx_band_page_info * page_info,	/* bandinfo incl. bandlist */
    int page_count		/* # of copies to print if final "print,"
				   /* 0 if partial page, -1 if cancel */
    ));

/* Retrieve the least-recently added queue entry from the pageq. If no */
/* entry is available, waits on a signal from gx_page_queue_enqueue. Must */
/* eventually be followed by a call to gx_page_queue_finish_dequeue for the */
/* same pageq entry. */
/* Even though the pageq is actually removed from the pageq, a mark is made in */
/* the pageq to indicate that the pageq is not "empty" until the */
/* gx_page_queue_finish_dequeue; this is for the benefit of */
/* gx_page_queue_wait_???, since the completing the current page's rendering */
/* may free more memory. */
/* Typically called by renderer thread loop, which looks like: */
/*   do */
/*     { gx_page_queue_start_deqeueue(...); */
/*       render_retrieved_entry(...); */
/*       gx_page_queue_finish_dequeue(...); */
/*     } */
/*    while (some condition); */
gx_page_queue_entry *		/* removed entry */
gx_page_queue_start_dequeue(P1(
    gx_page_queue * queue	/* page queue to retrieve from */
    ));

/* Free the pageq entry, then signal any waiting threads. */
/* Typically used to indicate completion of rendering the pageq entry. */
void gx_page_queue_finish_dequeue(P1(
    gx_page_queue_entry * entry	/* entry that was retrieved to delete */
    ));

#endif /*!defined(gxpageq_h_INCLUDED) */
