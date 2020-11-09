/*
  This file is part of libmicrohttpd
  Copyright (C) 2016-2019 Karlson2k (Evgeny Grin)

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
 * @file testcurl/mhd_has_in_name.h
 * @brief Static functions and macros helpers for testsuite.
 * @author Karlson2k (Evgeny Grin)
 */

#include <string.h>

/**
 * Check whether program name contains specific @a marker string.
 * Only last component in pathname is checked for marker presence,
 * all leading directories names (if any) are ignored. Directories
 * separators are handled correctly on both non-W32 and W32
 * platforms.
 * @param prog_name program name, may include path
 * @param marker    marker to look for.
 * @return zero if any parameter is NULL or empty string or
 *         @prog_name ends with slash or @marker is not found in
 *         program name, non-zero if @maker is found in program
 *         name.
 */
static int
has_in_name(const char *prog_name, const char *marker)
{
  size_t name_pos;
  size_t pos;

  if (!prog_name || !marker || !prog_name[0] || !marker[0])
    return 0;

  pos = 0;
  name_pos = 0;
  while (prog_name[pos])
    {
      if ('/' == prog_name[pos])
        name_pos = pos + 1;
#if defined(_WIN32) || defined(__CYGWIN__)
      else if ('\\' == prog_name[pos])
        name_pos = pos + 1;
#endif /* _WIN32 || __CYGWIN__ */
      pos++;
    }
  if (name_pos == pos)
    return 0;
  return strstr(prog_name + name_pos, marker) != (char*)0;
}
