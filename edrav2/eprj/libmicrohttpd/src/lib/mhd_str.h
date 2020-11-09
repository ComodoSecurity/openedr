/*
  This file is part of libmicrohttpd
  Copyright (C) 2015, 2016 Karlson2k (Evgeny Grin)

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
 * @file microhttpd/mhd_str.h
 * @brief  Header for string manipulating helpers
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_STR_H
#define MHD_STR_H 1

#include "mhd_options.h"

#include <stdint.h>
#include <stdlib.h>
#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#endif /* HAVE_STDBOOL_H */

#ifdef MHD_FAVOR_SMALL_CODE
#include "mhd_limits.h"
#endif /* MHD_FAVOR_SMALL_CODE */

#ifndef MHD_STATICSTR_LEN_
/**
 * Determine length of static string / macro strings at compile time.
 */
#define MHD_STATICSTR_LEN_(macro) (sizeof(macro)/sizeof(char) - 1)
#endif /* ! MHD_STATICSTR_LEN_ */

/*
 * Block of functions/macros that use US-ASCII charset as required by HTTP
 * standards. Not affected by current locale settings.
 */

#ifndef MHD_FAVOR_SMALL_CODE
/**
 * Check two string for equality, ignoring case of US-ASCII letters.
 * @param str1 first string to compare
 * @param str2 second string to compare
 * @return non-zero if two strings are equal, zero otherwise.
 */
int
MHD_str_equal_caseless_ (const char * str1,
                 const char * str2);
#else  /* MHD_FAVOR_SMALL_CODE */
/* Reuse MHD_str_equal_caseless_n_() to reduce size */
#define MHD_str_equal_caseless_(s1,s2) MHD_str_equal_caseless_n_((s1),(s2), SIZE_MAX)
#endif /* MHD_FAVOR_SMALL_CODE */


/**
 * Check two string for equality, ignoring case of US-ASCII letters and
 * checking not more than @a maxlen characters.
 * Compares up to first terminating null character, but not more than
 * first @a maxlen characters.
 * @param str1 first string to compare
 * @param str2 second string to compare
 * @param maxlen maximum number of characters to compare
 * @return non-zero if two strings are equal, zero otherwise.
 */
int
MHD_str_equal_caseless_n_ (const char * const str1,
                  const char * const str2,
                  size_t maxlen);


/**
 * Check whether @a str has case-insensitive @a token.
 * Token could be surrounded by spaces and tabs and delimited by comma.
 * Match succeed if substring between start, end of string or comma
 * contains only case-insensitive token and optional spaces and tabs.
 * @warning token must not contain null-charters except optional
 *          terminating null-character.
 * @param str the string to check
 * @param token the token to find
 * @param token_len length of token, not including optional terminating
 *                  null-character.
 * @return non-zero if two strings are equal, zero otherwise.
 */
bool
MHD_str_has_token_caseless_ (const char * str,
                             const char * const token,
                             size_t token_len);

/**
 * Check whether @a str has case-insensitive static @a tkn.
 * Token could be surrounded by spaces and tabs and delimited by comma.
 * Match succeed if substring between start, end of string or comma
 * contains only case-insensitive token and optional spaces and tabs.
 * @warning tkn must be static string
 * @param str the string to check
 * @param tkn the static string of token to find
 * @return non-zero if two strings are equal, zero otherwise.
 */
#define MHD_str_has_s_token_caseless_(str,tkn) \
    MHD_str_has_token_caseless_((str),(tkn),MHD_STATICSTR_LEN_(tkn))

#ifndef MHD_FAVOR_SMALL_CODE
/* Use individual function for each case to improve speed */

/**
 * Convert decimal US-ASCII digits in string to number in uint64_t.
 * Conversion stopped at first non-digit character.
 * @param str string to convert
 * @param out_val pointer to uint64_t to store result of conversion
 * @return non-zero number of characters processed on succeed,
 *         zero if no digit is found, resulting value is larger
 *         then possible to store in uint64_t or @a out_val is NULL
 */
size_t
MHD_str_to_uint64_ (const char * str,
                    uint64_t * out_val);

/**
 * Convert not more then @a maxlen decimal US-ASCII digits in string to
 * number in uint64_t.
 * Conversion stopped at first non-digit character or after @a maxlen 
 * digits.
 * @param str string to convert
 * @param maxlen maximum number of characters to process
 * @param out_val pointer to uint64_t to store result of conversion
 * @return non-zero number of characters processed on succeed,
 *         zero if no digit is found, resulting value is larger
 *         then possible to store in uint64_t or @a out_val is NULL
 */
size_t
MHD_str_to_uint64_n_ (const char * str,
                      size_t maxlen,
                      uint64_t * out_val);


/**
 * Convert hexadecimal US-ASCII digits in string to number in uint32_t.
 * Conversion stopped at first non-digit character.
 * @param str string to convert
 * @param out_val pointer to uint32_t to store result of conversion
 * @return non-zero number of characters processed on succeed, 
 *         zero if no digit is found, resulting value is larger
 *         then possible to store in uint32_t or @a out_val is NULL
 */
size_t
MHD_strx_to_uint32_ (const char * str,
                     uint32_t * out_val);


/**
 * Convert not more then @a maxlen hexadecimal US-ASCII digits in string
 * to number in uint32_t.
 * Conversion stopped at first non-digit character or after @a maxlen 
 * digits.
 * @param str string to convert
 * @param maxlen maximum number of characters to process
 * @param out_val pointer to uint32_t to store result of conversion
 * @return non-zero number of characters processed on succeed,
 *         zero if no digit is found, resulting value is larger
 *         then possible to store in uint32_t or @a out_val is NULL
 */
size_t
MHD_strx_to_uint32_n_ (const char * str,
                      size_t maxlen,
                      uint32_t * out_val);


/**
 * Convert hexadecimal US-ASCII digits in string to number in uint64_t.
 * Conversion stopped at first non-digit character.
 * @param str string to convert
 * @param out_val pointer to uint64_t to store result of conversion
 * @return non-zero number of characters processed on succeed, 
 *         zero if no digit is found, resulting value is larger
 *         then possible to store in uint64_t or @a out_val is NULL
 */
size_t
MHD_strx_to_uint64_ (const char * str,
                     uint64_t * out_val);


/**
 * Convert not more then @a maxlen hexadecimal US-ASCII digits in string
 * to number in uint64_t.
 * Conversion stopped at first non-digit character or after @a maxlen 
 * digits.
 * @param str string to convert
 * @param maxlen maximum number of characters to process
 * @param out_val pointer to uint64_t to store result of conversion
 * @return non-zero number of characters processed on succeed,
 *         zero if no digit is found, resulting value is larger
 *         then possible to store in uint64_t or @a out_val is NULL
 */
size_t
MHD_strx_to_uint64_n_ (const char * str,
                       size_t maxlen,
                       uint64_t * out_val);

#else  /* MHD_FAVOR_SMALL_CODE */
/* Use one universal function and macros to reduce size */

/**
 * Generic function for converting not more then @a maxlen
 * hexadecimal or decimal US-ASCII digits in string to number.
 * Conversion stopped at first non-digit character or after @a maxlen 
 * digits.
 * To be used only within macro.
 * @param str the string to convert
 * @param maxlen the maximum number of characters to process
 * @param out_val the pointer to uint64_t to store result of conversion
 * @param val_size the size of variable pointed by @a out_val
 * @param max_val the maximum decoded number
 * @param base the numeric base, 10 or 16
 * @return non-zero number of characters processed on succeed,
 *         zero if no digit is found, resulting value is larger
 *         then @ max_val or @a out_val is NULL
 */
size_t
MHD_str_to_uvalue_n_ (const char * str,
                      size_t maxlen,
                      void * out_val,
                      size_t val_size,
                      uint64_t max_val,
                      int base);

#define MHD_str_to_uint64_(s,ov) MHD_str_to_uvalue_n_((s),SIZE_MAX,(ov),\
                                             sizeof(uint64_t),UINT64_MAX,10)

#define MHD_str_to_uint64_n_(s,ml,ov) MHD_str_to_uvalue_n_((s),(ml),(ov),\
                                             sizeof(uint64_t),UINT64_MAX,10)

#define MHD_strx_to_sizet_(s,ov) MHD_str_to_uvalue_n_((s),SIZE_MAX,(ov),\
                                             sizeof(size_t),SIZE_MAX,16)

#define MHD_strx_to_sizet_n_(s,ml,ov) MHD_str_to_uvalue_n_((s),(ml),(ov),\
                                             sizeof(size_t),SIZE_MAX,16)

#define MHD_strx_to_uint32_(s,ov) MHD_str_to_uvalue_n_((s),SIZE_MAX,(ov),\
                                             sizeof(uint32_t),UINT32_MAX,16)

#define MHD_strx_to_uint32_n_(s,ml,ov) MHD_str_to_uvalue_n_((s),(ml),(ov),\
                                             sizeof(uint32_t),UINT32_MAX,16)

#define MHD_strx_to_uint64_(s,ov) MHD_str_to_uvalue_n_((s),SIZE_MAX,(ov),\
                                             sizeof(uint64_t),UINT64_MAX,16)

#define MHD_strx_to_uint64_n_(s,ml,ov) MHD_str_to_uvalue_n_((s),(ml),(ov),\
                                             sizeof(uint64_t),UINT64_MAX,16)

#endif /* MHD_FAVOR_SMALL_CODE */

#endif /* MHD_STR_H */
