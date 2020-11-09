/*
     This file is part of libmicrohttpd
     Copyright (C) 2007, 2011, 2017, 2019 Christian Grothoff, Karlson2k (Evgeny Grin)

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
 * @file reason_phrase.c
 * @brief  Tables of the string response phrases
 * @author Elliot Glaysher
 * @author Christian Grothoff (minor code clean up)
 * @author Karlson2k (Evgeny Grin)
 */
#include "platform.h"
#include "microhttpd.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

static const char *const invalid_hundred[] = {
  NULL
};

static const char *const one_hundred[] = {
  /* 100 */ "Continue"               /* RFC7231, Section 6.2.1 */,
  /* 101 */ "Switching Protocols"    /* RFC7231, Section 6.2.2 */,
  /* 102 */ "Processing"             /* RFC2518 */,
  /* 103 */ "Early Hints"            /* RFC8297 */
};

static const char *const two_hundred[] = {
  /* 200 */ "OK"                     /* RFC7231, Section 6.3.1 */,
  /* 201 */ "Created"                /* RFC7231, Section 6.3.2 */,
  /* 202 */ "Accepted"               /* RFC7231, Section 6.3.3 */,
  /* 203 */ "Non-Authoritative Information" /* RFC7231, Section 6.3.4 */,
  /* 204 */ "No Content"             /* RFC7231, Section 6.3.5 */,
  /* 205 */ "Reset Content"          /* RFC7231, Section 6.3.6 */,
  /* 206 */ "Partial Content"        /* RFC7233, Section 4.1 */,
  /* 207 */ "Multi-Status"           /* RFC4918 */,
  /* 208 */ "Already Reported"       /* RFC5842 */,
  /* 209 */ "Unknown"                /* Not used */,
  /* 210 */ "Unknown"                /* Not used */,
  /* 211 */ "Unknown"                /* Not used */,
  /* 212 */ "Unknown"                /* Not used */,
  /* 213 */ "Unknown"                /* Not used */,
  /* 214 */ "Unknown"                /* Not used */,
  /* 215 */ "Unknown"                /* Not used */,
  /* 216 */ "Unknown"                /* Not used */,
  /* 217 */ "Unknown"                /* Not used */,
  /* 218 */ "Unknown"                /* Not used */,
  /* 219 */ "Unknown"                /* Not used */,
  /* 220 */ "Unknown"                /* Not used */,
  /* 221 */ "Unknown"                /* Not used */,
  /* 222 */ "Unknown"                /* Not used */,
  /* 223 */ "Unknown"                /* Not used */,
  /* 224 */ "Unknown"                /* Not used */,
  /* 225 */ "Unknown"                /* Not used */,
  /* 226 */ "IM Used"                /* RFC3229 */
};

static const char *const three_hundred[] = {
  /* 300 */ "Multiple Choices"       /* RFC7231, Section 6.4.1 */,
  /* 301 */ "Moved Permanently"      /* RFC7231, Section 6.4.2 */,
  /* 302 */ "Found"                  /* RFC7231, Section 6.4.3 */,
  /* 303 */ "See Other"              /* RFC7231, Section 6.4.4 */,
  /* 304 */ "Not Modified"           /* RFC7232, Section 4.1 */,
  /* 305 */ "Use Proxy"              /* RFC7231, Section 6.4.5 */,
  /* 306 */ "Switch Proxy"           /* Not used! RFC7231, Section 6.4.6 */,
  /* 307 */ "Temporary Redirect"     /* RFC7231, Section 6.4.7 */,
  /* 308 */ "Permanent Redirect"     /* RFC7538 */
};

static const char *const four_hundred[] = {
  /* 400 */ "Bad Request"            /* RFC7231, Section 6.5.1 */,
  /* 401 */ "Unauthorized"           /* RFC7235, Section 3.1 */,
  /* 402 */ "Payment Required"       /* RFC7231, Section 6.5.2 */,
  /* 403 */ "Forbidden"              /* RFC7231, Section 6.5.3 */,
  /* 404 */ "Not Found"              /* RFC7231, Section 6.5.4 */,
  /* 405 */ "Method Not Allowed"     /* RFC7231, Section 6.5.5 */,
  /* 406 */ "Not Acceptable"         /* RFC7231, Section 6.5.6 */,
  /* 407 */ "Proxy Authentication Required" /* RFC7235, Section 3.2 */,
  /* 408 */ "Request Timeout"        /* RFC7231, Section 6.5.7 */,
  /* 409 */ "Conflict"               /* RFC7231, Section 6.5.8 */,
  /* 410 */ "Gone"                   /* RFC7231, Section 6.5.9 */,
  /* 411 */ "Length Required"        /* RFC7231, Section 6.5.10 */,
  /* 412 */ "Precondition Failed"    /* RFC7232, Section 4.2; RFC8144, Section 3.2 */,
  /* 413 */ "Payload Too Large"      /* RFC7231, Section 6.5.11 */,
  /* 414 */ "URI Too Long"           /* RFC7231, Section 6.5.12 */,
  /* 415 */ "Unsupported Media Type" /* RFC7231, Section 6.5.13; RFC7694, Section 3 */,
  /* 416 */ "Range Not Satisfiable"  /* RFC7233, Section 4.4 */,
  /* 417 */ "Expectation Failed"     /* RFC7231, Section 6.5.14 */,
  /* 418 */ "Unknown"                /* Not used */,
  /* 419 */ "Unknown"                /* Not used */,
  /* 420 */ "Unknown"                /* Not used */,
  /* 421 */ "Misdirected Request"    /* RFC7540, Section 9.1.2 */,
  /* 422 */ "Unprocessable Entity"   /* RFC4918 */,
  /* 423 */ "Locked"                 /* RFC4918 */,
  /* 424 */ "Failed Dependency"      /* RFC4918 */,
  /* 425 */ "Too Early"              /* RFC8470 */,
  /* 426 */ "Upgrade Required"       /* RFC7231, Section 6.5.15 */,
  /* 427 */ "Unknown"                /* Not used */,
  /* 428 */ "Precondition Required"  /* RFC6585 */,
  /* 429 */ "Too Many Requests"      /* RFC6585 */,
  /* 430 */ "Unknown"                /* Not used */,
  /* 431 */ "Request Header Fields Too Large" /* RFC6585 */,
  /* 432 */ "Unknown"                /* Not used */,
  /* 433 */ "Unknown"                /* Not used */,
  /* 434 */ "Unknown"                /* Not used */,
  /* 435 */ "Unknown"                /* Not used */,
  /* 436 */ "Unknown"                /* Not used */,
  /* 437 */ "Unknown"                /* Not used */,
  /* 438 */ "Unknown"                /* Not used */,
  /* 439 */ "Unknown"                /* Not used */,
  /* 440 */ "Unknown"                /* Not used */,
  /* 441 */ "Unknown"                /* Not used */,
  /* 442 */ "Unknown"                /* Not used */,
  /* 443 */ "Unknown"                /* Not used */,
  /* 444 */ "Unknown"                /* Not used */,
  /* 445 */ "Unknown"                /* Not used */,
  /* 446 */ "Unknown"                /* Not used */,
  /* 447 */ "Unknown"                /* Not used */,
  /* 448 */ "Unknown"                /* Not used */,
  /* 449 */ "Reply With"             /* MS IIS extension */,
  /* 450 */ "Blocked by Windows Parental Controls" /* MS extension */,
  /* 451 */ "Unavailable For Legal Reasons" /* RFC7725 */
};

static const char *const five_hundred[] = {
  /* 500 */ "Internal Server Error"  /* RFC7231, Section 6.6.1 */,
  /* 501 */ "Not Implemented"        /* RFC7231, Section 6.6.2 */,
  /* 502 */ "Bad Gateway"            /* RFC7231, Section 6.6.3 */,
  /* 503 */ "Service Unavailable"    /* RFC7231, Section 6.6.4 */,
  /* 504 */ "Gateway Timeout"        /* RFC7231, Section 6.6.5 */,
  /* 505 */ "HTTP Version Not Supported" /* RFC7231, Section 6.6.6 */,
  /* 506 */ "Variant Also Negotiates" /* RFC2295 */,
  /* 507 */ "Insufficient Storage"   /* RFC4918 */,
  /* 508 */ "Loop Detected"          /* RFC5842 */,
  /* 509 */ "Bandwidth Limit Exceeded" /* Apache extension */,
  /* 510 */ "Not Extended"           /* RFC2774 */,
  /* 511 */ "Network Authentication Required" /* RFC6585 */
};


struct MHD_Reason_Block
{
  size_t max;
  const char *const*data;
};

#define BLOCK(m) { (sizeof(m) / sizeof(char*)), m }

static const struct MHD_Reason_Block reasons[] = {
  BLOCK (invalid_hundred),
  BLOCK (one_hundred),
  BLOCK (two_hundred),
  BLOCK (three_hundred),
  BLOCK (four_hundred),
  BLOCK (five_hundred),
};


const char *
MHD_get_reason_phrase_for (unsigned int code)
{
  if ( (code >= 100) &&
       (code < 600) &&
       (reasons[code / 100].max > (code % 100)) )
    return reasons[code / 100].data[code % 100];
  return "Unknown";
}
