/* Suspend until termination of a requests.
   Copyright (C) 1997-2021 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */


/* We use an UGLY hack to prevent gcc from finding us cheating.  The
   implementations of aio_suspend and aio_suspend64 are identical and so
   we want to avoid code duplication by using aliases.  But gcc sees
   the different parameter lists and prints a warning.  We define here
   a function so that aio_suspend64 has no prototype.  */
#define aio_suspend64 XXX
#include <aio.h>
/* And undo the hack.  */
#undef aio_suspend64

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>

#include <libc-lock.h>
#include <aio_misc.h>


struct clparam
{
  const struct aiocb *const *list;
  struct waitlist *waitlist;
  struct requestlist **requestlist;
#ifndef DONT_NEED_AIO_MISC_COND
  pthread_cond_t *cond;
#endif
  int nent;
};


static void
cleanup (void *arg)
{
#ifdef DONT_NEED_AIO_MISC_COND
  /* Acquire the mutex.  If pthread_cond_*wait is used this would
     happen implicitly.  */
  pthread_mutex_lock (&__aio_requests_mutex);
#endif

  const struct clparam *param = (const struct clparam *) arg;

  /* Now remove the entry in the waiting list for all requests
     which didn't terminate.  */
  int cnt = param->nent;
  while (cnt-- > 0)
    if (param->list[cnt] != NULL
	&& param->list[cnt]->__error_code == EINPROGRESS)
      {
	struct waitlist **listp;

	assert (param->requestlist[cnt] != NULL);

	/* There is the chance that we cannot find our entry anymore. This
	   could happen if the request terminated and restarted again.  */
	listp = &param->requestlist[cnt]->waiting;
	while (*listp != NULL && *listp != &param->waitlist[cnt])
	  listp = &(*listp)->next;

	if (*listp != NULL)
	  *listp = (*listp)->next;
      }

#ifndef DONT_NEED_AIO_MISC_COND
  /* Release the conditional variable.  */
  (void) pthread_cond_destroy (param->cond);
#endif

  /* Release the mutex.  */
  pthread_mutex_unlock (&__aio_requests_mutex);
}

#ifdef DONT_NEED_AIO_MISC_COND
static int
__attribute__ ((noinline))
do_aio_misc_wait (unsigned int *cntr, const struct __timespec64 *timeout)
{
  int result = 0;

  AIO_MISC_WAIT (result, *cntr, timeout, 1);

  return result;
}
#endif

int
__aio_suspend_time64 (const struct aiocb *const list[], int nent,
		      const struct __timespec64 *timeout)
{
  if (__glibc_unlikely (nent < 0))
    {
      __set_errno (EINVAL);
      return -1;
    }

  struct waitlist waitlist[nent];
  struct requestlist *requestlist[nent];
#ifndef DONT_NEED_AIO_MISC_COND
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
#endif
  int cnt;
  bool any = false;
  int result = 0;
  unsigned int cntr = 1;

  /* Request the mutex.  */
  pthread_mutex_lock (&__aio_requests_mutex);

  /* There is not yet a finished request.  Signal the request that
     we are working for it.  */
  for (cnt = 0; cnt < nent; ++cnt)
    if (list[cnt] != NULL)
      {
	if (list[cnt]->__error_code == EINPROGRESS)
	  {
	    requestlist[cnt] = __aio_find_req ((aiocb_union *) list[cnt]);

	    if (requestlist[cnt] != NULL)
	      {
#ifndef DONT_NEED_AIO_MISC_COND
		waitlist[cnt].cond = &cond;
#endif
		waitlist[cnt].result = NULL;
		waitlist[cnt].next = requestlist[cnt]->waiting;
		waitlist[cnt].counterp = &cntr;
		waitlist[cnt].sigevp = NULL;
		requestlist[cnt]->waiting = &waitlist[cnt];
		any = true;
	      }
	    else
	      /* We will never suspend.  */
	      break;
	  }
	else
	  /* We will never suspend.  */
	  break;
      }

  struct __timespec64 ts;
  if (timeout != NULL)
    {
      __clock_gettime64 (CLOCK_MONOTONIC, &ts);
      ts.tv_sec += timeout->tv_sec;
      ts.tv_nsec += timeout->tv_nsec;
      if (ts.tv_nsec >= 1000000000)
	{
	  ts.tv_nsec -= 1000000000;
	  ts.tv_sec++;
	}
    }

  /* Only if none of the entries is NULL or finished to be wait.  */
  if (cnt == nent && any)
    {
      struct clparam clparam =
	{
	  .list = list,
	  .waitlist = waitlist,
	  .requestlist = requestlist,
#ifndef DONT_NEED_AIO_MISC_COND
	  .cond = &cond,
#endif
	  .nent = nent
	};

      pthread_cleanup_push (cleanup, &clparam);

#ifdef DONT_NEED_AIO_MISC_COND
      result = do_aio_misc_wait (&cntr, timeout == NULL ? NULL : &ts);
#else
      struct timespec ts32 = valid_timespec64_to_timespec (ts);
      result = pthread_cond_timedwait (&cond, &__aio_requests_mutex,
				       timeout == NULL ? NULL : &ts32);
#endif

      pthread_cleanup_pop (0);
    }

  /* Now remove the entry in the waiting list for all requests
     which didn't terminate.  */
  while (cnt-- > 0)
    if (list[cnt] != NULL && list[cnt]->__error_code == EINPROGRESS)
      {
	struct waitlist **listp;

	assert (requestlist[cnt] != NULL);

	/* There is the chance that we cannot find our entry anymore. This
	   could happen if the request terminated and restarted again.  */
	listp = &requestlist[cnt]->waiting;
	while (*listp != NULL && *listp != &waitlist[cnt])
	  listp = &(*listp)->next;

	if (*listp != NULL)
	  *listp = (*listp)->next;
      }

#ifndef DONT_NEED_AIO_MISC_COND
  /* Release the conditional variable.  */
  if (__glibc_unlikely (pthread_cond_destroy (&cond) != 0))
    /* This must never happen.  */
    abort ();
#endif

  if (result != 0)
    {
#ifndef DONT_NEED_AIO_MISC_COND
      /* An error occurred.  Possibly it's ETIMEDOUT.  We have to translate
	 the timeout error report of `pthread_cond_timedwait' to the
	 form expected from `aio_suspend'.  */
      if (result == ETIMEDOUT)
	__set_errno (EAGAIN);
      else
#endif
	__set_errno (result);

      result = -1;
    }

  /* Release the mutex.  */
  pthread_mutex_unlock (&__aio_requests_mutex);

  return result;
}

#if __TIMESIZE != 64
librt_hidden_def (__aio_suspend_time64)

int
__aio_suspend (const struct aiocb *const list[], int nent,
               const struct timespec *timeout)
{
  struct __timespec64 ts64;

  if (timeout != NULL)
    ts64 = valid_timespec_to_timespec64 (*timeout);

  return __aio_suspend_time64 (list, nent, timeout != NULL ? &ts64 : NULL);
}
#endif
weak_alias (__aio_suspend, aio_suspend)
weak_alias (aio_suspend, aio_suspend64)
