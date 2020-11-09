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
 * @file microhttpd/test_http_reasons.c
 * @brief  Unit tests for MHD_get_reason_phrase_for() function
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_options.h"
#include <stdio.h>
#include "microhttpd.h"
#include "mhd_str.h"

static int expect_result(int code, const char* expected)
{
  const char* const reason = MHD_get_reason_phrase_for(code);
  if (MHD_str_equal_caseless_(reason, expected))
    return 0;
  fprintf(stderr, "Incorrect reason returned for code %d:\n  Returned: \"%s\"  \tExpected: \"%s\"\n",
          code, reason, expected);
  return 1;
}

static int expect_absent(int code)
{
  return expect_result(code, "unknown");
}

static int test_absent_codes(void)
{
  int errcount = 0;
  errcount += expect_absent(0);
  errcount += expect_absent(1);
  errcount += expect_absent(50);
  errcount += expect_absent(99);
  errcount += expect_absent(600);
  errcount += expect_absent(601);
  errcount += expect_absent(900);
  errcount += expect_absent(10000);
  return errcount;
}

static int test_1xx(void)
{
  int errcount = 0;
  errcount += expect_result(MHD_HTTP_CONTINUE, "continue");
  errcount += expect_result(MHD_HTTP_PROCESSING, "processing");
  errcount += expect_absent(110);
  errcount += expect_absent(190);
  return errcount;
}

static int test_2xx(void)
{
  int errcount = 0;
  errcount += expect_result(MHD_HTTP_OK, "ok");
  errcount += expect_result(MHD_HTTP_ALREADY_REPORTED, "already reported");
  errcount += expect_absent(217);
  errcount += expect_result(MHD_HTTP_IM_USED, "im used");
  errcount += expect_absent(230);
  errcount += expect_absent(295);
  return errcount;
}

static int test_3xx(void)
{
  int errcount = 0;
  errcount += expect_result(MHD_HTTP_MULTIPLE_CHOICES, "multiple choices");
  errcount += expect_result(MHD_HTTP_SEE_OTHER, "see other");
  errcount += expect_result(MHD_HTTP_PERMANENT_REDIRECT, "permanent redirect");
  errcount += expect_absent(311);
  errcount += expect_absent(399);
  return errcount;
}

static int test_4xx(void)
{
  int errcount = 0;
  errcount += expect_result(MHD_HTTP_BAD_REQUEST, "bad request");
  errcount += expect_result(MHD_HTTP_NOT_FOUND, "not found");
  errcount += expect_result(MHD_HTTP_URI_TOO_LONG, "uri too long");
  errcount += expect_result(MHD_HTTP_EXPECTATION_FAILED, "expectation failed");
  errcount += expect_result(MHD_HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE, "request header fields too large");
  errcount += expect_absent(441);
  errcount += expect_result(MHD_HTTP_UNAVAILABLE_FOR_LEGAL_REASONS, "unavailable for legal reasons");
  errcount += expect_absent(470);
  errcount += expect_absent(493);
  return errcount;
}

static int test_5xx(void)
{
  int errcount = 0;
  errcount += expect_result(MHD_HTTP_INTERNAL_SERVER_ERROR, "internal server error");
  errcount += expect_result(MHD_HTTP_BAD_GATEWAY, "bad gateway");
  errcount += expect_result(MHD_HTTP_HTTP_VERSION_NOT_SUPPORTED, "http version not supported");
  errcount += expect_result(MHD_HTTP_NETWORK_AUTHENTICATION_REQUIRED, "network authentication required");
  errcount += expect_absent(520);
  errcount += expect_absent(597);
  return errcount;
}

int main(int argc, char * argv[])
{
  int errcount = 0;
  (void)argc; (void)argv; /* Unused. Silent compiler warning. */

  errcount += test_absent_codes();
  errcount += test_1xx();
  errcount += test_2xx();
  errcount += test_3xx();
  errcount += test_4xx();
  errcount += test_5xx();
  return errcount == 0 ? 0 : 1;
}
