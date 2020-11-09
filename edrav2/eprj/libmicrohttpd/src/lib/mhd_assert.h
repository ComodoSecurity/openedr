/*
  This file is part of libmicrohttpd
  Copyright (C) 2017 Karlson2k (Evgeny Grin)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library.
  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file microhttpd/mhd_assert.h
 * @brief  macros for mhd_assert()
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_ASSERT_H
#define MHD_ASSERT_H 1

#include "mhd_options.h"
#ifdef NDEBUG
#  define mhd_assert(ignore) ((void)0)
#else  /* _DEBUG */
#  ifdef HAVE_ASSERT
#    include <assert.h>
#    define mhd_assert(CHK) assert(CHK)
#  else  /* ! HAVE_ASSERT */
#    include <stdio.h>
#    include <stdlib.h>
#    define mhd_assert(CHK) \
       do { \
           if (!(CHK)) { \
             fprintf(stderr, "%s:%u Assertion failed: %s\nProgram aborted.\n", \
                     __FILE__, (unsigned)__LINE__, #CHK); \
             fflush(stderr); abort(); } \
          } while(0)
#  endif /* ! HAVE_ASSERT */
#endif /* _DEBUG */

#endif /* ! MHD_ASSERT_H */
