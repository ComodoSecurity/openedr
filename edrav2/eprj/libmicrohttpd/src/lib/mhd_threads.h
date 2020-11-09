/*
  This file is part of libmicrohttpd
  Copyright (C) 2016 Karlson2k (Evgeny Grin)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

/**
 * @file microhttpd/mhd_threads.h
 * @brief  Header for platform-independent threads abstraction
 * @author Karlson2k (Evgeny Grin)
 *
 * Provides basic abstraction for threads.
 * Any functions can be implemented as macro on some platforms
 * unless explicitly marked otherwise.
 * Any function argument can be skipped in macro, so avoid
 * variable modification in function parameters.
 *
 * @warning Unlike pthread functions, most of functions return
 *          nonzero on success.
 */

#ifndef MHD_THREADS_H
#define MHD_THREADS_H 1

#include "mhd_options.h"
#ifdef HAVE_STDDEF_H
#  include <stddef.h> /* for size_t */
#else  /* ! HAVE_STDDEF_H */
#  include <stdlib.h> /* for size_t */
#endif /* ! HAVE_STDDEF_H */

#if defined(MHD_USE_POSIX_THREADS)
#  undef HAVE_CONFIG_H
#  include <pthread.h>
#  define HAVE_CONFIG_H 1
#elif defined(MHD_USE_W32_THREADS)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN 1
#  endif /* !WIN32_LEAN_AND_MEAN */
#  include <windows.h>
#else
#  error No threading API is available.
#endif

#ifndef MHD_NO_THREAD_NAMES
#  if defined(MHD_USE_POSIX_THREADS)
#    if defined(HAVE_PTHREAD_SETNAME_NP_GNU) || defined(HAVE_PTHREAD_SET_NAME_NP_FREEBSD) || \
        defined(HAVE_PTHREAD_SETNAME_NP_DARWIN) || defined(HAVE_PTHREAD_SETNAME_NP_NETBSD) || \
        defined(HAVE_PTHREAD_ATTR_SETNAME_NP_NETBSD) || defined(HAVE_PTHREAD_ATTR_SETNAME_NP_IBMI)
#      define MHD_USE_THREAD_NAME_
#    endif /* HAVE_PTHREAD_SETNAME_NP */
#  elif defined(MHD_USE_W32_THREADS)
#    ifdef _MSC_FULL_VER
       /* Thread names only available with VC compiler */
#      define MHD_USE_THREAD_NAME_
#    endif /* _MSC_FULL_VER */
#  endif
#endif

#if defined(MHD_USE_POSIX_THREADS)
  typedef pthread_t MHD_thread_handle_;
#elif defined(MHD_USE_W32_THREADS)
  typedef HANDLE MHD_thread_handle_;
#endif

#if defined(MHD_USE_POSIX_THREADS)
#  define MHD_THRD_RTRN_TYPE_ void*
#  define MHD_THRD_CALL_SPEC_
#elif defined(MHD_USE_W32_THREADS)
#  define MHD_THRD_RTRN_TYPE_ unsigned
#  define MHD_THRD_CALL_SPEC_ __stdcall
#endif

#if defined(MHD_USE_POSIX_THREADS)
  typedef pthread_t MHD_thread_ID_;
#elif defined(MHD_USE_W32_THREADS)
  typedef DWORD MHD_thread_ID_;
#endif

/* Depending on implementation, pthread_create() MAY set thread ID into
 * provided pointer and after it start thread OR start thread and after
 * if set thread ID. In latter case, to avoid data races, additional
 * pthread_self() call is required in thread routine. Is some platform
 * is known for setting thread ID BEFORE starting thread macro
 * MHD_PTHREAD_CREATE__SET_ID_BEFORE_START_THREAD could be defined
 * to save some resources. */
#if defined(MHD_USE_POSIX_THREADS)
#  ifdef MHD_PTHREAD_CREATE__SET_ID_BEFORE_START_THREAD
  union _MHD_thread_handle_ID_
  {
    MHD_thread_handle_  handle; /**< To be used in other threads */
    MHD_thread_ID_      ID;     /**< To be used in thread itself */
  };
  typedef union _MHD_thread_handle_ID_ MHD_thread_handle_ID_;
#  else  /* ! MHD_PTHREAD_CREATE__SET_ID_BEFORE_START_THREAD */
  struct _MHD_thread_handle_ID_
  {
    MHD_thread_handle_  handle; /**< To be used in other threads */
    MHD_thread_ID_      ID;     /**< To be used in thread itself */
  };
  typedef struct _MHD_thread_handle_ID_ MHD_thread_handle_ID_;
#  endif /* ! MHD_PTHREAD_CREATE__SET_ID_BEFORE_START_THREAD */
#elif defined(MHD_USE_W32_THREADS)
  struct _MHD_thread_handle_ID_
  {
    MHD_thread_handle_  handle; /**< To be used in other threads */
    MHD_thread_ID_      ID;     /**< To be used in thread itself */
  };
  typedef struct _MHD_thread_handle_ID_ MHD_thread_handle_ID_;
#endif

#if defined(MHD_USE_POSIX_THREADS)
/**
 * Wait until specified thread is ended and free thread handle on success.
 * @param thread handle to watch
 * @return nonzero on success, zero otherwise
 */
#define MHD_join_thread_(thread) (!pthread_join((thread), NULL))
#elif defined(MHD_USE_W32_THREADS)
/**
 * Wait until specified thread is ended and free thread handle on success.
 * @param thread handle to watch
 * @return nonzero on success, zero otherwise
 */
#define MHD_join_thread_(thread) (WAIT_OBJECT_0 == WaitForSingleObject((thread), INFINITE) ? (CloseHandle((thread)), !0) : 0)
#endif

#if defined(MHD_USE_POSIX_THREADS)
/**
 * Check whether provided thread ID match current thread.
 * @param ID thread ID to match
 * @return nonzero on match, zero otherwise
 */
#define MHD_thread_ID_match_current_(ID) (pthread_equal((ID), pthread_self()))
#elif defined(MHD_USE_W32_THREADS)
/**
 * Check whether provided thread ID match current thread.
 * @param ID thread ID to match
 * @return nonzero on match, zero otherwise
 */
#define MHD_thread_ID_match_current_(ID) (GetCurrentThreadId() == (ID))
#endif

#if defined(MHD_USE_POSIX_THREADS)
#  ifdef MHD_PTHREAD_CREATE__SET_ID_BEFORE_START_THREAD
/**
 * Initialise thread ID.
 * @param thread_handle_ID_ptr pointer to thread handle-ID
 */
#define MHD_thread_init_(thread_handle_ID_ptr) (void)0
#  else  /* ! MHD_PTHREAD_CREATE__SET_ID_BEFORE_START_THREAD */
/**
 * Initialise thread ID.
 * @param thread_handle_ID_ptr pointer to thread handle-ID
 */
#define MHD_thread_init_(thread_handle_ID_ptr) ((thread_handle_ID_ptr)->ID=pthread_self())
#  endif /* ! MHD_PTHREAD_CREATE__SET_ID_BEFORE_START_THREAD */
#elif defined(MHD_USE_W32_THREADS)
/**
 * Initialise thread ID.
 * @param thread_handle_ID_ptr pointer to thread handle-ID
 */
#define MHD_thread_init_(thread_handle_ID_ptr) ((thread_handle_ID_ptr)->ID=GetCurrentThreadId())
#endif

/**
 * Signature of main function for a thread.
 *
 * @param cls closure argument for the function
 * @return termination code from the thread
 */
typedef MHD_THRD_RTRN_TYPE_
(MHD_THRD_CALL_SPEC_ *MHD_THREAD_START_ROUTINE_)(void *cls);


/**
 * Create a thread and set the attributes according to our options.
 *
 * If thread is created, thread handle must be freed by MHD_join_thread_().
 *
 * @param thread        handle to initialize
 * @param stack_size    size of stack for new thread, 0 for default
 * @param start_routine main function of thread
 * @param arg argument  for start_routine
 * @return non-zero on success; zero otherwise
 */
int
MHD_create_thread_ (MHD_thread_handle_ID_ *thread,
                    size_t stack_size,
                    MHD_THREAD_START_ROUTINE_ start_routine,
                    void *arg);

#ifndef MHD_USE_THREAD_NAME_
#define MHD_create_named_thread_(t,n,s,r,a) MHD_create_thread_((t),(s),(r),(a))
#else  /* MHD_USE_THREAD_NAME_ */
/**
 * Create a named thread and set the attributes according to our options.
 *
 * @param thread        handle to initialize
 * @param thread_name   name for new thread
 * @param stack_size    size of stack for new thread, 0 for default
 * @param start_routine main function of thread
 * @param arg argument  for start_routine
 * @return non-zero on success; zero otherwise
 */
int
MHD_create_named_thread_ (MHD_thread_handle_ID_ *thread,
                          const char* thread_name,
                          size_t stack_size,
                          MHD_THREAD_START_ROUTINE_ start_routine,
                          void *arg);

#endif /* MHD_USE_THREAD_NAME_ */

#endif /* ! MHD_THREADS_H */
