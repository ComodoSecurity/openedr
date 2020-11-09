/*
  This file is part of libmicrohttpd
  Copyright (C) 2019 Karlson2k (Evgeny Grin)

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
 * @file microhttpd/test_md5.h
 * @brief  Unit tests for md5 functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_options.h"
#include "md5.h"
#include "test_helpers.h"
#include <stdio.h>

static int verbose = 0; /* verbose level (0-1)*/


struct str_with_len
{
  const char * const str;
  const size_t len;
};

#define D_STR_W_LEN(s) {(s), (sizeof((s)) / sizeof(char)) - 1}

struct data_unit1
{
  const struct str_with_len str_l;
  const uint8_t digest[MD5_DIGEST_SIZE];
};

static const struct data_unit1 data_units1[] = {
    {D_STR_W_LEN("1234567890!@~%&$@#{}[]\\/!?`."),
        {0x1c, 0x68, 0xc2, 0xe5, 0x1f, 0x63, 0xc9, 0x5f, 0x17, 0xab, 0x1f, 0x20, 0x8b, 0x86, 0x39, 0x57}},
    {D_STR_W_LEN("Simple string."),
        {0xf1, 0x2b, 0x7c, 0xad, 0xa0, 0x41, 0xfe, 0xde, 0x4e, 0x68, 0x16, 0x63, 0xb4, 0x60, 0x5d, 0x78}},
    {D_STR_W_LEN("abcdefghijklmnopqrstuvwxyz"),
        {0xc3, 0xfc, 0xd3, 0xd7, 0x61, 0x92, 0xe4, 0x00, 0x7d, 0xfb, 0x49, 0x6c, 0xca, 0x67, 0xe1, 0x3b}},
    {D_STR_W_LEN("zyxwvutsrqponMLKJIHGFEDCBA"),
        {0x05, 0x61, 0x3a, 0x6b, 0xde, 0x75, 0x3a, 0x45, 0x91, 0xa8, 0x81, 0xb0, 0xa7, 0xe2, 0xe2, 0x0e}},
    {D_STR_W_LEN("abcdefghijklmnopqrstuvwxyzzyxwvutsrqponMLKJIHGFEDCBA"
                 "abcdefghijklmnopqrstuvwxyzzyxwvutsrqponMLKJIHGFEDCBA"),
        {0xaf, 0xab, 0xc7, 0xe9, 0xe7, 0x17, 0xbe, 0xd6, 0xc0, 0x0f, 0x78, 0x8c, 0xde, 0xdd, 0x11, 0xd1}},
};

static const size_t units1_num = sizeof(data_units1) / sizeof(data_units1[0]);

struct bin_with_len
{
  const uint8_t bin[512];
  const size_t len;
};

struct data_unit2
{
  const struct bin_with_len bin_l;
  const uint8_t digest[MD5_DIGEST_SIZE];
};

static const struct data_unit2 data_units2[] = {
    { { {97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
        117, 118, 119, 120, 121, 122}, 26},/* a..z ASCII sequence */
      {0xc3, 0xfc, 0xd3, 0xd7, 0x61, 0x92, 0xe4, 0x00, 0x7d, 0xfb, 0x49, 0x6c, 0xca, 0x67, 0xe1, 0x3b}
    },
    { { {65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65,
        65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65,
        65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65
        }, 72 },/* 'A' x 72 times */
      {0x24, 0xa5, 0xef, 0x36, 0x82, 0x80, 0x3a, 0x06, 0x2f, 0xea, 0xad, 0xad, 0x76, 0xda, 0xbd, 0xa8}
    },
    { { {19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
        43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
        68, 69, 70, 71, 72, 73}, 55},/* 19..73 sequence */
      {0x6d, 0x2e, 0x6e, 0xde, 0x5d, 0x64, 0x6a, 0x17, 0xf1, 0x09, 0x2c, 0xac, 0x19, 0x10, 0xe3, 0xd6}
    },
    { { {7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
        57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69}, 63},/* 7..69 sequence */
      {0x88, 0x13, 0x48, 0x47, 0x73, 0xaa, 0x92, 0xf2, 0xc9, 0xdd, 0x69, 0xb3, 0xac, 0xf4, 0xba, 0x6e}
    },
    { { {38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
        62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
        87, 88, 89, 90, 91, 92}, 55},/* 38..92 sequence */
      {0x80, 0xf0, 0x05, 0x7e, 0xa2, 0xf7, 0xc8, 0x43, 0x12, 0xd3, 0xb1, 0x61, 0xab, 0x52, 0x3b, 0xaf}
    },
    { { {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
        28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
        53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72},
      72},/* 1..72 sequence */
      {0xc3, 0x28, 0xc5, 0xad, 0xc9, 0x26, 0xa9, 0x99, 0x95, 0x4a, 0x5e, 0x25, 0x50, 0x34, 0x51, 0x73}
    },
    { { {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
        27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
        77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
        101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
        121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
        141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
        161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
        181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200,
        201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220,
        221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
        241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255}, 256}, /* 0..255 sequence */
      {0xe2, 0xc8, 0x65, 0xdb, 0x41, 0x62, 0xbe, 0xd9, 0x63, 0xbf, 0xaa, 0x9e, 0xf6, 0xac, 0x18, 0xf0}
    },
    { { {199, 198, 197, 196, 195, 194, 193, 192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180,
        179, 178, 177, 176, 175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 165, 164, 163, 162, 161, 160,
        159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 149, 148, 147, 146, 145, 144, 143, 142, 141, 140,
        139}, 61}, /* 199..139 sequence */
      {0xbb, 0x3f, 0xdb, 0x4a, 0x96, 0x03, 0x36, 0x37, 0x38, 0x78, 0x5e, 0x44, 0xbf, 0x3a, 0x85, 0x51}
    },
    { { {255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244, 243, 242, 241, 240, 239, 238, 237, 236,
        235, 234, 233, 232, 231, 230, 229, 228, 227, 226, 225, 224, 223, 222, 221, 220, 219, 218, 217, 216,
        215, 214, 213, 212, 211, 210, 209, 208, 207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196,
        195, 194, 193, 192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180, 179, 178, 177, 176,
        175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 165, 164, 163, 162, 161, 160, 159, 158, 157, 156,
        155, 154, 153, 152, 151, 150, 149, 148, 147, 146, 145, 144, 143, 142, 141, 140, 139, 138, 137, 136,
        135, 134, 133, 132, 131, 130, 129, 128, 127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116,
        115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96, 95,
        94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70,
        69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45,
        44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20,
        19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1}, 255}, /* 255..1 sequence */
      {0x52, 0x21, 0xa5, 0x83, 0x4f, 0x38, 0x7c, 0x73, 0xba, 0x18, 0x22, 0xb1, 0xf9, 0x7e, 0xae, 0x8b}
    },
    { { {41, 35, 190, 132, 225, 108, 214, 174, 82, 144, 73, 241, 241, 187, 233, 235, 179, 166, 219, 60, 135,
        12, 62, 153, 36, 94, 13, 28, 6, 183, 71, 222, 179, 18, 77, 200, 67, 187, 139, 166, 31, 3, 90, 125, 9,
        56, 37, 31, 93, 212, 203, 252, 150, 245, 69, 59, 19, 13, 137, 10, 28, 219, 174, 50, 32, 154, 80, 238,
        64, 120, 54, 253, 18, 73, 50, 246, 158, 125, 73, 220, 173, 79, 20, 242, 68, 64, 102, 208, 107, 196,
        48, 183, 50, 59, 161, 34, 246, 34, 145, 157, 225, 139, 31, 218, 176, 202, 153, 2, 185, 114, 157, 73,
        44, 128, 126, 197, 153, 213, 233, 128, 178, 234, 201, 204, 83, 191, 103, 214, 191, 20, 214, 126, 45,
        220, 142, 102, 131, 239, 87, 73, 97, 255, 105, 143, 97, 205, 209, 30, 157, 156, 22, 114, 114, 230,
        29, 240, 132, 79, 74, 119, 2, 215, 232, 57, 44, 83, 203, 201, 18, 30, 51, 116, 158, 12, 244, 213,
        212, 159, 212, 164, 89, 126, 53, 207, 50, 34, 244, 204, 207, 211, 144, 45, 72, 211, 143, 117, 230,
        217, 29, 42, 229, 192, 247, 43, 120, 129, 135, 68, 14, 95, 80, 0, 212, 97, 141, 190, 123, 5, 21, 7,
        59, 51, 130, 31, 24, 112, 146, 218, 100, 84, 206, 177, 133, 62, 105, 21, 248, 70, 106, 4, 150, 115,
        14, 217, 22, 47, 103, 104, 212, 247, 74, 74, 208, 87, 104}, 255}, /* pseudo-random data */
      {0x55, 0x61, 0x2c, 0xeb, 0x29, 0xee, 0xa8, 0xb2, 0xf6, 0x10, 0x7b, 0xc1, 0x5b, 0x0f, 0x01, 0x95}
    }
};

static const size_t units2_num = sizeof(data_units2) / sizeof(data_units2[0]);


/*
 *  Helper functions
 */

/**
 * Print bin as hex
 *
 * @param bin binary data
 * @param len number of bytes in bin
 * @param hex pointer to len*2+1 bytes buffer
 */
static void
bin2hex (const uint8_t *bin,
        size_t len,
        char *hex)
{
  while (len-- > 0)
    {
      unsigned int b1, b2;
      b1 = (*bin >> 4) & 0xf;
      *hex++ = (char)((b1 > 9) ? (b1 + 'A' - 10) : (b1 + '0'));
      b2 = *bin++ & 0xf;
      *hex++ = (char)((b2 > 9) ? (b2 + 'A' - 10) : (b2 + '0'));
    }
  *hex = 0;
}

static int
check_result (const char *test_name,
              unsigned int check_num,
              const uint8_t calcualted[MD5_DIGEST_SIZE],
              const uint8_t expected[MD5_DIGEST_SIZE])
{
  int failed = memcmp(calcualted, expected, MD5_DIGEST_SIZE);
  check_num++; /* Print 1-based numbers */
  if (failed)
    {
      char calc_str[MD5_DIGEST_STRING_LENGTH];
      char expc_str[MD5_DIGEST_STRING_LENGTH];
      bin2hex(calcualted, MD5_DIGEST_SIZE, calc_str);
      bin2hex(expected, MD5_DIGEST_SIZE, expc_str);
      fprintf (stderr, "FAILED: %s check %u: calculated digest %s, expected digest %s.\n",
               test_name, check_num, calc_str, expc_str);
      fflush (stderr);
    }
  else if (verbose)
    {
      char calc_str[MD5_DIGEST_STRING_LENGTH];
      bin2hex(calcualted, MD5_DIGEST_SIZE, calc_str);
      printf ("PASSED: %s check %u: calculated digest %s match expected digest.\n",
               test_name, check_num, calc_str);
      fflush (stdout);
    }
  return failed ? 1 : 0;
}


/*
 *  Tests
 */

/* Calculated MD5 as one pass for whole data */
int test1_str(void)
{
  int num_failed = 0;
  for (unsigned int i = 0; i < units1_num; i++)
    {
      struct MD5Context ctx;
      uint8_t digest[MD5_DIGEST_SIZE];

      MHD_MD5Init (&ctx);
      MHD_MD5Update (&ctx, (const uint8_t*)data_units1[i].str_l.str, data_units1[i].str_l.len);
      MHD_MD5Final (&ctx, digest);
      num_failed += check_result (__FUNCTION__, i, digest,
                                  data_units1[i].digest);
    }
  return num_failed;
}

int test1_bin(void)
{
  int num_failed = 0;
  for (unsigned int i = 0; i < units2_num; i++)
    {
      struct MD5Context ctx;
      uint8_t digest[MD5_DIGEST_SIZE];

      MHD_MD5Init (&ctx);
      MHD_MD5Update (&ctx, data_units2[i].bin_l.bin, data_units2[i].bin_l.len);
      MHD_MD5Final (&ctx, digest);
      num_failed += check_result (__FUNCTION__, i, digest,
                                  data_units2[i].digest);
    }
  return num_failed;
}

/* Calculated MD5 as two iterations for whole data */
int test2_str(void)
{
  int num_failed = 0;
  for (unsigned int i = 0; i < units1_num; i++)
    {
      struct MD5Context ctx;
      uint8_t digest[MD5_DIGEST_SIZE];
      size_t part_s = data_units1[i].str_l.len / 4;

      MHD_MD5Init (&ctx);
      MHD_MD5Update (&ctx, (const uint8_t*)data_units1[i].str_l.str, part_s);
      MHD_MD5Update (&ctx, (const uint8_t*)data_units1[i].str_l.str + part_s, data_units1[i].str_l.len - part_s);
      MHD_MD5Final (&ctx, digest);
      num_failed += check_result (__FUNCTION__, i, digest,
                                  data_units1[i].digest);
    }
  return num_failed;
}

int test2_bin(void)
{
  int num_failed = 0;
  for (unsigned int i = 0; i < units2_num; i++)
    {
      struct MD5Context ctx;
      uint8_t digest[MD5_DIGEST_SIZE];
      size_t part_s = data_units2[i].bin_l.len * 2 / 3;

      MHD_MD5Init (&ctx);
      MHD_MD5Update (&ctx, data_units2[i].bin_l.bin, part_s);
      MHD_MD5Update (&ctx, data_units2[i].bin_l.bin + part_s, data_units2[i].bin_l.len - part_s);
      MHD_MD5Final (&ctx, digest);
      num_failed += check_result (__FUNCTION__, i, digest,
                                  data_units2[i].digest);
    }
  return num_failed;
}

int main(int argc, char * argv[])
{
  int num_failed = 0;
  (void)has_in_name; /* Mute compiler warning. */
  if (has_param(argc, argv, "-v") || has_param(argc, argv, "--verbose"))
    verbose = 1;

  num_failed += test1_str();
  num_failed += test1_bin();

  num_failed += test2_str();
  num_failed += test2_bin();

  return num_failed ? 1 : 0;
}
