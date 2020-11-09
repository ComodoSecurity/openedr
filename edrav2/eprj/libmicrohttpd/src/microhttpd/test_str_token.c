/*
  This file is part of libmicrohttpd
  Copyright (C) 2017 Karlson2k (Evgeny Grin)

  This test tool is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2, or
  (at your option) any later version.

  This test tool is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/**
 * @file microhttpd/test_str_token.c
 * @brief  Unit tests for some mhd_str functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_options.h"
#include <stdio.h>
#include "mhd_str.h"


static int
expect_found_n(const char *str, const char *token, size_t token_len)
{
  if (!MHD_str_has_token_caseless_(str, token, token_len))
    {
      fprintf(stderr, "MHD_str_has_token_caseless_() FAILED:\n\tMHD_str_has_token_caseless_(%s, %s, %lu) return false\n",
              str, token, (unsigned long) token_len);
      return 1;
    }
  return 0;
}

#define expect_found(s,t) expect_found_n((s),(t),MHD_STATICSTR_LEN_(t))

static int
expect_not_found_n(const char *str, const char *token, size_t token_len)
{
  if (MHD_str_has_token_caseless_(str, token, token_len))
    {
      fprintf(stderr, "MHD_str_has_token_caseless_() FAILED:\n\tMHD_str_has_token_caseless_(%s, %s, %lu) return true\n",
              str, token, (unsigned long) token_len);
      return 1;
    }
  return 0;
}

#define expect_not_found(s,t) expect_not_found_n((s),(t),MHD_STATICSTR_LEN_(t))

int check_match(void)
{
  int errcount = 0;
  errcount += expect_found("string", "string");
  errcount += expect_found("String", "string");
  errcount += expect_found("string", "String");
  errcount += expect_found("strinG", "String");
  errcount += expect_found("\t strinG", "String");
  errcount += expect_found("strinG\t ", "String");
  errcount += expect_found(" \t tOkEn  ", "toKEN");
  errcount += expect_found("not token\t,  tOkEn  ", "toKEN");
  errcount += expect_found("not token,\t  tOkEn, more token", "toKEN");
  errcount += expect_found("not token,\t  tOkEn\t, more token", "toKEN");
  errcount += expect_found(",,,,,,test,,,,", "TESt");
  errcount += expect_found(",,,,,\t,test,,,,", "TESt");
  errcount += expect_found(",,,,,,test, ,,,", "TESt");
  errcount += expect_found(",,,,,, test,,,,", "TESt");
  errcount += expect_found(",,,,,, test not,test,,", "TESt");
  errcount += expect_found(",,,,,, test not,,test,,", "TESt");
  errcount += expect_found(",,,,,, test not ,test,,", "TESt");
  errcount += expect_found(",,,,,, test", "TESt");
  errcount += expect_found(",,,,,, test      ", "TESt");
  errcount += expect_found("no test,,,,,, test      ", "TESt");
  return errcount;
}

int check_not_match(void)
{
  int errcount = 0;
  errcount += expect_not_found("strin", "string");
  errcount += expect_not_found("Stringer", "string");
  errcount += expect_not_found("sstring", "String");
  errcount += expect_not_found("string", "Strin");
  errcount += expect_not_found("\t( strinG", "String");
  errcount += expect_not_found(")strinG\t ", "String");
  errcount += expect_not_found(" \t tOkEn t ", "toKEN");
  errcount += expect_not_found("not token\t,  tOkEner  ", "toKEN");
  errcount += expect_not_found("not token,\t  tOkEns, more token", "toKEN");
  errcount += expect_not_found("not token,\t  tOkEns\t, more token", "toKEN");
  errcount += expect_not_found(",,,,,,testing,,,,", "TESt");
  errcount += expect_not_found(",,,,,\t,test,,,,", "TESting");
  errcount += expect_not_found("tests,,,,,,quest, ,,,", "TESt");
  errcount += expect_not_found(",,,,,, testы,,,,", "TESt");
  errcount += expect_not_found(",,,,,, test not,хtest,,", "TESt");
  errcount += expect_not_found("testing,,,,,, test not,,test2,,", "TESt");
  errcount += expect_not_found(",testi,,,,, test not ,test,,", "TESting");
  errcount += expect_not_found(",,,,,,2 test", "TESt");
  errcount += expect_not_found(",,,,,,test test      ", "test");
  errcount += expect_not_found("no test,,,,,, test      test", "test");
  return errcount;
}

int main(int argc, char * argv[])
{
  int errcount = 0;
  (void)argc; (void)argv; /* Unused. Silent compiler warning. */
  errcount += check_match();
  errcount += check_not_match();
  return errcount == 0 ? 0 : 1;
}
