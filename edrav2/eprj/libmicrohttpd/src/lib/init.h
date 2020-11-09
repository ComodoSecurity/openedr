/*
  This file is part of libmicrohttpd
  Copyright (C) 2007-2018 Daniel Pittman and Christian Grothoff

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
 * @file lib/init.h
 * @brief functions to initialize library
 * @author Christian Grothoff
 */
#ifndef INIT_H
#define INIT_H

#include "internal.h"

#ifdef _AUTOINIT_FUNCS_ARE_SUPPORTED
/**
 * Do nothing - global initialisation is
 * performed by library constructor.
 */
#define MHD_check_global_init_() (void)0
#else  /* ! _AUTOINIT_FUNCS_ARE_SUPPORTED */
/**
 * Check whether global initialisation was performed
 * and call initialiser if necessary.
 */
void
MHD_check_global_init_ (void);
#endif /* ! _AUTOINIT_FUNCS_ARE_SUPPORTED */


#endif  /* INIT_H */
