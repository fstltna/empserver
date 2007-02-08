/*
 *  Empire - A multi-player, client/server Internet based war game.
 *  Copyright (C) 1986-2007, Dave Pare, Jeff Bailey, Thomas Ruschak,
 *                           Ken Stevens, Steve McClure
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  ---
 *
 *  See files README, COPYING and CREDITS in the root of the source
 *  tree for related information and legal notices.  It is expected
 *  that future projects/authors will amend these files as needed.
 *
 *  ---
 *
 *  pthread.c: Interface from Empire threads to POSIX threads
 * 
 *  Known contributors to this file:
 *     Sasha Mikheev
 *     Steve McClure, 1998
 *     Markus Armbruster, 2005-2007
 *     Ron Koenderink, 2007
 */

/* Required for PTHREAD_STACK_MIN on some systems, e.g. Solaris: */
#define _XOPEN_SOURCE 500

#include <config.h>

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#if !defined(_WIN32)
#include <sys/time.h>
#include <unistd.h>
#endif

#include "misc.h"
#include "empthread.h"
#include "prototypes.h"

#define EMPTH_KILLED  1
#define EMPTH_INTR 2

struct empth_t {
    char *name;			/* thread name */
    void *ud;			/* user data */
    int state;			/* my state */
    void (*ep)(void *);		/* entry point */
    pthread_t id;		/* thread id */
};

struct empth_rwlock_t {
    char *name;
    pthread_rwlock_t lock;
};

/* Thread-specific data key */
static pthread_key_t ctx_key;

/* Flags that were passed to empth_init() */
static int empth_flags;

/* Pointer to thread context variable */
static void **udata;

/*
 * Non-preemption mutex.
 * Empire code outside this file is only executed while holding this
 * mutex.  This makes sure Empire code is never preempted by Empire
 * code.
 */
static pthread_mutex_t mtx_ctxsw;

static void empth_status(char *format, ...)
    ATTRIBUTE((format (printf, 1, 2)));
static void empth_alarm(int sig);

static void *
empth_start(void *arg)
{
    empth_t *ctx = arg;

    ctx->id = pthread_self();
    pthread_setspecific(ctx_key, ctx);
    pthread_mutex_lock(&mtx_ctxsw);
    *udata = ctx->ud;
    ctx->ep(ctx->ud);
    empth_exit();
    return NULL;
}

static void
empth_status(char *format, ...)
{
    va_list ap;
    static struct timeval startTime;
    struct timeval tv;
    char buf[1024];
    int sec, msec;
    empth_t *a;

    va_start(ap, format);
    if (empth_flags & EMPTH_PRINT) {
	if (startTime.tv_sec == 0)
	    gettimeofday(&startTime, 0);
	gettimeofday(&tv, 0);
	sec = tv.tv_sec - startTime.tv_sec;
	msec = (tv.tv_usec - startTime.tv_usec) / 1000;
	if (msec < 0) {
	    sec++;
	    msec += 1000;
	}
	vsprintf(buf, format, ap);
	a = empth_self();
	printf("%d:%02d.%03d %17s: %s\n", sec / 60, sec % 60, msec / 10,
	       a->name, buf);

    }
    va_end(ap);
}


int
empth_init(void **ctx_ptr, int flags)
{
    empth_t *ctx;
    sigset_t set;
    struct sigaction act;

    empth_flags = flags;
    udata = ctx_ptr;

    empth_init_signals();
    sigemptyset(&set);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    act.sa_handler = empth_alarm;
    sigaction(SIGALRM, &act, NULL);

    pthread_key_create(&ctx_key, NULL);
    pthread_mutex_init(&mtx_ctxsw, NULL);

    ctx = malloc(sizeof(empth_t));
    if (!ctx) {
	logerror("pthread init failed: not enough memory");
	exit(1);
    }
    ctx->name = "Main";
    ctx->ep = 0;
    ctx->ud = 0;
    ctx->id = pthread_self();
    ctx->state = 0;
    pthread_setspecific(ctx_key, ctx);
    pthread_mutex_lock(&mtx_ctxsw);
    logerror("pthreads initialized");
    return 0;
}


/*
 * prio can be used for setting scheeduling policy but...
 * it seems to be optional in POSIX threads and Solaris
 * for example just ignores it.
 * More then that priority is not needed even in lwp threads.
 */
empth_t *
empth_create(int prio, void (*entry)(void *), int size, int flags,
	     char *name, void *ud)
{
    pthread_t t;
    pthread_attr_t attr;
    empth_t *ctx;
    int eno;

    empth_status("creating new thread %s", name);

    ctx = malloc(sizeof(empth_t));
    if (!ctx) {
	logerror("not enough memory to create thread %s", name);
	return NULL;
    }
    ctx->name = strdup(name);
    ctx->ud = ud;
    ctx->state = 0;
    ctx->ep = entry;

    eno = pthread_attr_init(&attr);
    if (eno) {
	logerror("can not create thread attribute %s: %s",
		 name, strerror(eno));
	goto bad;
    }
    if (size < PTHREAD_STACK_MIN)
	size = PTHREAD_STACK_MIN;
    pthread_attr_setstacksize(&attr, size);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    eno = pthread_create(&t, &attr, empth_start, ctx);
    if (eno) {
	logerror("can not create thread: %s: %s", name, strerror(eno));
	goto bad;
    }
    empth_status("new thread id is %ld", (long)t);
    empth_yield();
    return ctx;

  bad:
    pthread_attr_destroy(&attr);
    free(ctx);
    return NULL;
}


static void
empth_restorectx(void)
{
    empth_t *ctx_ptr;

    ctx_ptr = pthread_getspecific(ctx_key);
    *udata = ctx_ptr->ud;
    if (ctx_ptr->state == EMPTH_KILLED) {
	empth_status("i am dead");
	empth_exit();
    }
    ctx_ptr->state = 0;
    empth_status("context restored");
}

empth_t *
empth_self(void)
{
    return pthread_getspecific(ctx_key);
}

void
empth_exit(void)
{
    empth_t *ctx = pthread_getspecific(ctx_key);

    empth_status("empth_exit");
    pthread_mutex_unlock(&mtx_ctxsw);
    free(ctx->name);
    free(ctx);
    pthread_exit(0);
}

void
empth_yield(void)
{
    pthread_mutex_unlock(&mtx_ctxsw);
    pthread_mutex_lock(&mtx_ctxsw);
    empth_restorectx();
}

void
empth_terminate(empth_t *a)
{
    empth_status("killing thread %s", a->name);
    a->state = EMPTH_KILLED;
    pthread_kill(a->id, SIGALRM);
}

void
empth_select(int fd, int flags)
{

    fd_set readmask;
    fd_set writemask;
    struct timeval tv;
    int n;

    pthread_mutex_unlock(&mtx_ctxsw);
    empth_status("%s select on %d",
		 flags == EMPTH_FD_READ ? "read" : "write", fd);
    while (1) {
	tv.tv_sec = 1000000;
	tv.tv_usec = 0;

	FD_ZERO(&readmask);
	FD_ZERO(&writemask);

	switch (flags) {
	case EMPTH_FD_READ:
	    FD_SET(fd, &readmask);
	    break;
	case EMPTH_FD_WRITE:
	    FD_SET(fd, &writemask);
	    break;
	default:
	    logerror("bad flag %d passed to empth_select", flags);
	    empth_exit();
	}

	n = select(fd + 1, &readmask, &writemask, (fd_set *) 0, &tv);

	if (n < 0) {
	    if (errno == EINTR) {
		/* go handle the signal */
		empth_status("select broken by signal");
		goto done;
		return;
	    }
	    /* strange but we dont get EINTR on select broken by signal */
	    empth_status("select failed (%s)", strerror(errno));
	    goto done;
	    return;
	}

	if (flags == EMPTH_FD_READ && FD_ISSET(fd, &readmask)) {
	    empth_status("input ready");
	    break;
	}
	if (flags == EMPTH_FD_WRITE && FD_ISSET(fd, &writemask)) {
	    empth_status("output ready");
	    break;
	}
    }

  done:
    pthread_mutex_lock(&mtx_ctxsw);
    empth_restorectx();
}

static void
empth_alarm(int sig)
{
    /*
     * Nothing to do --- we handle this signal just to let
     * empth_wakeup() and empth_terminate() interrupt system calls.
     */
}

void
empth_wakeup(empth_t *a)
{
    empth_status("waking up thread %s", a->name);
    if (a->state == 0)
	a->state = EMPTH_INTR;
    pthread_kill(a->id, SIGALRM);
}

int
empth_sleep(time_t until)
{
    empth_t *ctx = pthread_getspecific(ctx_key);
    struct timeval tv;
    int res;

    empth_status("going to sleep %ld sec", until - time(0));
    pthread_mutex_unlock(&mtx_ctxsw);
    do {
	tv.tv_sec = until - time(NULL);
	tv.tv_usec = 0;
	res = select(0, NULL, NULL, NULL, &tv);
    } while (res < 0 && ctx->state == 0);
    empth_status("sleep done. Waiting for lock");
    pthread_mutex_lock(&mtx_ctxsw);
    empth_restorectx();
    return res;
}

int
empth_wait_for_signal(void)
{
    sigset_t set;
    int sig, err;

    sigemptyset(&set);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_mutex_unlock(&mtx_ctxsw);
    for (;;) {
	empth_status("waiting for signals");
	err = sigwait(&set, &sig);
	if (CANT_HAPPEN(err)) {
	    sleep(60);
	    continue;
	}
	empth_status("got awaited signal %d", sig);
	pthread_mutex_lock(&mtx_ctxsw);
	empth_restorectx();
	return sig;
    }
}

empth_rwlock_t *
empth_rwlock_create(char *name)
{
    empth_rwlock_t *rwlock;

    rwlock = malloc(sizeof(*rwlock));
    if (!rwlock)
	return NULL;

    if (pthread_rwlock_init(&rwlock->lock, NULL) != 0) {
	free(rwlock);
	return NULL;
    }

    rwlock->name = strdup(name);
    return rwlock;
}

void
empth_rwlock_destroy(empth_rwlock_t *rwlock)
{
    pthread_rwlock_destroy(&rwlock->lock);
    free(rwlock->name);
    free(rwlock);
}

void
empth_rwlock_wrlock(empth_rwlock_t *rwlock)
{
    pthread_mutex_unlock(&mtx_ctxsw);
    pthread_rwlock_wrlock(&rwlock->lock);
    pthread_mutex_lock(&mtx_ctxsw);
    empth_restorectx();
}

void
empth_rwlock_rdlock(empth_rwlock_t *rwlock)
{
    pthread_mutex_unlock(&mtx_ctxsw);
    pthread_rwlock_rdlock(&rwlock->lock);
    pthread_mutex_lock(&mtx_ctxsw);
    empth_restorectx();
}

void
empth_rwlock_unlock(empth_rwlock_t *rwlock)
{
    pthread_rwlock_unlock(&rwlock->lock);
}
