/*
  This file is part of libmicrohttpd
  Copyright (C) 2015 Karlson2k (Evgeny Grin)

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
 * @file microhttpd/mhd_limits.h
 * @brief  limits values definitions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_LIMITS_H
#define MHD_LIMITS_H

#include "platform.h"

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif /* HAVE_LIMITS_H */

#define MHD_UNSIGNED_TYPE_MAX_(type) ((type)-1)
/* Assume 8 bits per byte, no padding bits. */
#define MHD_SIGNED_TYPE_MAX_(type) \
        ( (type)((( ((type)1) << (sizeof(type)*8 - 2)) - 1)*2 + 1) )
#define MHD_TYPE_IS_SIGNED_(type) (((type)0)>((type)-1))

#ifndef UINT_MAX
#ifdef __UINT_MAX__
#define UINT_MAX __UINT_MAX__
#else  /* ! __UINT_MAX__ */
#define UINT_MAX MHD_UNSIGNED_TYPE_MAX_(unsigned int)
#endif /* ! __UINT_MAX__ */
#endif /* !UINT_MAX */

#ifndef LONG_MAX
#ifdef __LONG_MAX__
#define LONG_MAX __LONG_MAX__
#else  /* ! __LONG_MAX__ */
#define LONG_MAX MHD_SIGNED_TYPE_MAX(long)
#endif /* ! __LONG_MAX__ */
#endif /* !OFF_T_MAX */

#ifndef ULLONG_MAX
#define ULLONG_MAX MHD_UNSIGNED_TYPE_MAX_(MHD_UNSIGNED_LONG_LONG)
#endif /* !ULLONG_MAX */

#ifndef INT32_MAX
#ifdef __INT32_MAX__
#define INT32_MAX __INT32_MAX__
#else  /* ! __INT32_MAX__ */
#define INT32_MAX ((int32_t)0x7FFFFFFF)
#endif /* ! __INT32_MAX__ */
#endif /* !INT32_MAX */

#ifndef UINT32_MAX
#ifdef __UINT32_MAX__
#define UINT32_MAX __UINT32_MAX__
#else  /* ! __UINT32_MAX__ */
#define UINT32_MAX ((int32_t)0xFFFFFFFF)
#endif /* ! __UINT32_MAX__ */
#endif /* !UINT32_MAX */

#ifndef UINT64_MAX
#ifdef __UINT64_MAX__
#define UINT64_MAX __UINT64_MAX__
#else  /* ! __UINT64_MAX__ */
#define UINT64_MAX ((uint64_t)0xFFFFFFFFFFFFFFFF)
#endif /* ! __UINT64_MAX__ */
#endif /* !UINT64_MAX */

#ifndef INT64_MAX
#ifdef __INT64_MAX__
#define INT64_MAX __INT64_MAX__
#else  /* ! __INT64_MAX__ */
#define INT64_MAX ((int64_t)0x7FFFFFFFFFFFFFFF)
#endif /* ! __UINT64_MAX__ */
#endif /* !INT64_MAX */

#ifndef SIZE_MAX
#ifdef __SIZE_MAX__
#define SIZE_MAX __SIZE_MAX__
#elif defined(UINTPTR_MAX)
#define SIZE_MAX UINTPTR_MAX
#else  /* ! __SIZE_MAX__ */
#define SIZE_MAX MHD_UNSIGNED_TYPE_MAX_(size_t)
#endif /* ! __SIZE_MAX__ */ 
#endif /* !SIZE_MAX */

#ifndef SSIZE_MAX
#ifdef __SSIZE_MAX__
#define SSIZE_MAX __SSIZE_MAX__
#elif defined(PTRDIFF_MAX)
#define SSIZE_MAX PTRDIFF_MAX
#elif defined(INTPTR_MAX)
#define SSIZE_MAX INTPTR_MAX
#else
#define SSIZE_MAN MHD_SIGNED_TYPE_MAX_(ssize_t)
#endif
#endif /* ! SSIZE_MAX */

#ifndef OFF_T_MAX
#ifdef OFF_MAX
#define OFF_T_MAX OFF_MAX
#elif defined(OFFT_MAX)
#define OFF_T_MAX OFFT_MAX
#elif defined(__APPLE__) && defined(__MACH__)
#define OFF_T_MAX INT64_MAX
#else
#define OFF_T_MAX MHD_SIGNED_TYPE_MAX_(off_t)
#endif
#endif /* !OFF_T_MAX */

#if defined(_LARGEFILE64_SOURCE) && !defined(OFF64_T_MAX)
#define OFF64_T_MAX MHD_SIGNED_TYPE_MAX_(uint64_t)
#endif /* _LARGEFILE64_SOURCE && !OFF64_T_MAX */

#ifndef TIME_T_MAX
#define TIME_T_MAX ((time_t)              \
       ( MHD_TYPE_IS_SIGNED_(time_t) ?    \
           MHD_SIGNED_TYPE_MAX_(time_t) : \
           MHD_UNSIGNED_TYPE_MAX_(time_t)))
#endif /* !TIME_T_MAX */

#ifndef TIMEVAL_TV_SEC_MAX
#ifndef _WIN32
#define TIMEVAL_TV_SEC_MAX TIME_T_MAX
#else  /* _WIN32 */
#define TIMEVAL_TV_SEC_MAX LONG_MAX
#endif /* _WIN32 */
#endif /* !TIMEVAL_TV_SEC_MAX */

#ifndef MHD_FD_BLOCK_SIZE
#ifdef _WIN32
#define MHD_FD_BLOCK_SIZE 16384 /* 16k */
#else /* _WIN32 */
#define MHD_FD_BLOCK_SIZE 4096 /* 4k */
#endif /* _WIN32 */
#endif /* !MHD_FD_BLOCK_SIZE */

#endif /* MHD_LIMITS_H */
