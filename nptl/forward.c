/* Copyright (C) 2002-2021 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

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

#include <dlfcn.h>
#include <pthreadP.h>
#include <signal.h>
#include <stdlib.h>

#include <shlib-compat.h>
#include <atomic.h>
#include <safe-fatal.h>


/* Pointers to the libc functions.  */
struct pthread_functions __libc_pthread_functions attribute_hidden;
int __libc_pthread_functions_init attribute_hidden;


#define FORWARD2(name, rettype, decl, params, defaction) \
rettype									      \
name decl								      \
{									      \
  if (!__libc_pthread_functions_init)					      \
    defaction;								      \
									      \
  return PTHFCT_CALL (ptr_##name, params);				      \
}

/* Same as FORWARD2, only without return.  */
#define FORWARD_NORETURN(name, rettype, decl, params, defaction) \
rettype									      \
name decl								      \
{									      \
  if (!__libc_pthread_functions_init)					      \
    defaction;								      \
									      \
  PTHFCT_CALL (ptr_##name, params);					      \
}

#define FORWARD(name, decl, params, defretval) \
  FORWARD2 (name, int, decl, params, return defretval)


#if SHLIB_COMPAT(libc, GLIBC_2_0, GLIBC_2_3_2)
FORWARD2 (__pthread_cond_broadcast_2_0, int attribute_compat_text_section,
	  (pthread_cond_2_0_t *cond), (cond), return 0)
compat_symbol (libc, __pthread_cond_broadcast_2_0, pthread_cond_broadcast,
	       GLIBC_2_0);
#endif
FORWARD (__pthread_cond_broadcast, (pthread_cond_t *cond), (cond), 0)
versioned_symbol (libc, __pthread_cond_broadcast, pthread_cond_broadcast,
		  GLIBC_2_3_2);

#if SHLIB_COMPAT(libc, GLIBC_2_0, GLIBC_2_3_2)
FORWARD2 (__pthread_cond_signal_2_0, int attribute_compat_text_section,
	  (pthread_cond_2_0_t *cond), (cond), return 0)
compat_symbol (libc, __pthread_cond_signal_2_0, pthread_cond_signal,
	       GLIBC_2_0);
#endif
FORWARD (__pthread_cond_signal, (pthread_cond_t *cond), (cond), 0)
versioned_symbol (libc, __pthread_cond_signal, pthread_cond_signal,
		  GLIBC_2_3_2);

#if SHLIB_COMPAT(libc, GLIBC_2_0, GLIBC_2_3_2)
FORWARD2 (__pthread_cond_wait_2_0, int attribute_compat_text_section,
	  (pthread_cond_2_0_t *cond, pthread_mutex_t *mutex), (cond, mutex),
	  return 0)
compat_symbol (libc, __pthread_cond_wait_2_0, pthread_cond_wait,
	       GLIBC_2_0);
#endif
FORWARD (__pthread_cond_wait, (pthread_cond_t *cond, pthread_mutex_t *mutex),
	 (cond, mutex), 0)
versioned_symbol (libc, __pthread_cond_wait, pthread_cond_wait,
		  GLIBC_2_3_2);

#if SHLIB_COMPAT(libc, GLIBC_2_0, GLIBC_2_3_2)
FORWARD2 (__pthread_cond_timedwait_2_0, int attribute_compat_text_section,
	  (pthread_cond_2_0_t *cond, pthread_mutex_t *mutex,
	   const struct timespec *abstime), (cond, mutex, abstime),
	  return 0)
compat_symbol (libc, __pthread_cond_timedwait_2_0, pthread_cond_timedwait,
	       GLIBC_2_0);
#endif
FORWARD (__pthread_cond_timedwait,
	 (pthread_cond_t *cond, pthread_mutex_t *mutex,
	  const struct timespec *abstime), (cond, mutex, abstime), 0)
versioned_symbol (libc, __pthread_cond_timedwait, pthread_cond_timedwait,
		  GLIBC_2_3_2);


FORWARD_NORETURN (__pthread_exit, void, (void *retval), (retval),
		  exit (EXIT_SUCCESS))
strong_alias (__pthread_exit, pthread_exit);


FORWARD (pthread_mutex_destroy, (pthread_mutex_t *mutex), (mutex), 0)

FORWARD (pthread_mutex_init,
	 (pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr),
	 (mutex, mutexattr), 0)

FORWARD (pthread_mutex_lock, (pthread_mutex_t *mutex), (mutex), 0)

FORWARD (pthread_mutex_unlock, (pthread_mutex_t *mutex), (mutex), 0)

FORWARD (__pthread_setcancelstate, (int state, int *oldstate),
	 (state, oldstate), 0)
strong_alias (__pthread_setcancelstate, pthread_setcancelstate)

FORWARD (pthread_setcanceltype, (int type, int *oldtype), (type, oldtype), 0)

FORWARD_NORETURN (__pthread_unwind,
                  void attribute_hidden __attribute ((noreturn))
                  __cleanup_fct_attribute attribute_compat_text_section,
                  (__pthread_unwind_buf_t *buf), (buf),
                  __safe_fatal ())
