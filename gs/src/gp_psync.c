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
/* POSIX pthreads threads / semaphore / monitor implementation */
#include "std.h"
#include "malloc_.h"
#include <pthread.h>
#include "gserror.h"
#include "gserrors.h"
#include "gpsync.h"

/*
 * Thanks to Larry Jones <larry.jones@sdrc.com> for this revision of
 * Aladdin's original code into a form that depends only on POSIX APIs.
 */

/*
 * Some old versions of the pthreads library define
 * pthread_attr_setdetachstate as taking a Boolean rather than an enum.
 * Compensate for this here.
 */
#ifndef PTHREAD_CREATE_DETACHED
#  define PTHREAD_CREATE_DETACHED 1
#endif

/* ------- Synchronization primitives -------- */

/* Semaphore supports wait/signal semantics */

typedef struct pt_semaphore_t {
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} pt_semaphore_t;

uint
gp_semaphore_sizeof(void)
{
    return sizeof(pt_semaphore_t);
}

/*
 * This procedure should really check errno and return something
 * more informative....
 */
#define SEM_ERROR_CODE(mem, scode)\
  (scode != 0 ? gs_note_error(mem, gs_error_ioerror) : 0)

int
gp_semaphore_open(const gs_memory_t *mem, gp_semaphore * sema)
{
    pt_semaphore_t * const sem = (pt_semaphore_t *)sema;
    int scode;

    if (!sema)
	return -1;		/* semaphores are not movable */
    sem->count = 0;
    scode = pthread_mutex_init(&sem->mutex, NULL);
    if (scode == 0)
	scode = pthread_cond_init(&sem->cond, NULL);
    return SEM_ERROR_CODE(mem, scode);
}

int
gp_semaphore_close(const gs_memory_t *mem, gp_semaphore * sema)
{
    pt_semaphore_t * const sem = (pt_semaphore_t *)sema;
    int scode, scode2;

    scode = pthread_cond_destroy(&sem->cond);
    scode2 = pthread_mutex_destroy(&sem->mutex);
    if (scode == 0)
	scode = scode2;
    return SEM_ERROR_CODE(mem, scode);
}

int
gp_semaphore_wait(const gs_memory_t *mem, gp_semaphore * sema)
{
    pt_semaphore_t * const sem = (pt_semaphore_t *)sema;
    int scode, scode2;

    scode = pthread_mutex_lock(&sem->mutex);
    if (scode != 0)
	return SEM_ERROR_CODE(mem, scode);
    while (sem->count == 0) {
        scode = pthread_cond_wait(&sem->cond, &sem->mutex);
        if (scode != 0)
	    break;
    }
    if (scode == 0)
	--sem->count;
    scode2 = pthread_mutex_unlock(&sem->mutex);
    if (scode == 0)
	scode = scode2;
    return SEM_ERROR_CODE(mem, scode);
}

int
gp_semaphore_signal(const gs_memory_t *mem, gp_semaphore * sema)
{
    pt_semaphore_t * const sem = (pt_semaphore_t *)sema;
    int scode, scode2;

    scode = pthread_mutex_lock(&sem->mutex);
    if (scode != 0)
	return SEM_ERROR_CODE(mem, scode);
    if (sem->count++ == 0)
	scode = pthread_cond_signal(&sem->cond);
    scode2 = pthread_mutex_unlock(&sem->mutex);
    if (scode == 0)
	scode = scode2;
    return SEM_ERROR_CODE(mem, scode);
}


/* Monitor supports enter/leave semantics */

uint
gp_monitor_sizeof(void)
{
    return sizeof(pthread_mutex_t);
}

int
gp_monitor_open(const gs_memory_t *mem, gp_monitor * mona)
{
    pthread_mutex_t * const mon = (pthread_mutex_t *)mona;
    int scode;

    if (!mona)
	return -1;		/* monitors are not movable */
    scode = pthread_mutex_init(mon, NULL);
    return SEM_ERROR_CODE(mem, scode);
}

int
gp_monitor_close(const gs_memory_t *mem, gp_monitor * mona)
{
    pthread_mutex_t * const mon = (pthread_mutex_t *)mona;
    int scode;

    scode = pthread_mutex_destroy(mon);
    return SEM_ERROR_CODE(mem, scode);
}

int
gp_monitor_enter(const gs_memory_t *mem, gp_monitor * mona)
{
    pthread_mutex_t * const mon = (pthread_mutex_t *)mona;
    int scode;

    scode = pthread_mutex_lock(mon);
    return SEM_ERROR_CODE(mem, scode);
}

int
gp_monitor_leave(const gs_memory_t *mem, gp_monitor * mona)
{
    pthread_mutex_t * const mon = (pthread_mutex_t *)mona;
    int scode;

    scode = pthread_mutex_unlock(mon);
    return SEM_ERROR_CODE(mem, scode);
}


/* --------- Thread primitives ---------- */

/*
 * In order to deal with the type mismatch between our thread API, where
 * the starting procedure returns void, and the API defined by pthreads,
 * where the procedure returns void *, we need to create a wrapper
 * closure.
 */
typedef struct gp_thread_creation_closure_s {
    gp_thread_creation_callback_t proc;  /* actual start procedure */
    void *proc_data;			/* closure data for proc */
} gp_thread_creation_closure_t;

/* Wrapper procedure called to start the new thread. */
private void *
gp_thread_begin_wrapper(void *thread_data /* gp_thread_creation_closure_t * */)
{
    gp_thread_creation_closure_t closure;

    closure = *(gp_thread_creation_closure_t *)thread_data;
    free(thread_data);
    DISCARD(closure.proc(closure.proc_data));
    return NULL;		/* return value is ignored */
}

int
gp_create_thread(const gs_memory_t *mem, 
		 gp_thread_creation_callback_t proc, void *proc_data)
{
    gp_thread_creation_closure_t *closure =
	(gp_thread_creation_closure_t *)malloc(sizeof(*closure));
    pthread_t ignore_thread;
    pthread_attr_t attr;
    int code;

    if (!closure)
	return_error(mem, gs_error_VMerror);
    closure->proc = proc;
    closure->proc_data = proc_data;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    code = pthread_create(&ignore_thread, &attr, gp_thread_begin_wrapper,
			  closure);
    if (code) {
	free(closure);
	return_error(mem, gs_error_ioerror);
    }
    return 0;
}
