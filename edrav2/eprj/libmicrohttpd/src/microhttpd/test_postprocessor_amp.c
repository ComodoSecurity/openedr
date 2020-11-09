#include "platform.h"
#include "microhttpd.h"
#include "internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

uint64_t num_errors;

int check_post(void *cls, enum MHD_ValueKind kind, const char* key,
                 const char* filename, const char* content_type,
                 const char* content_encoding, const char* data,
                 uint64_t off, size_t size)
{
  (void)cls; (void)kind; (void)filename; (void)content_type;  /* Unused. Silent compiler warning. */
  (void)content_encoding; (void)data; (void)off; (void)size;  /* Unused. Silent compiler warning. */
  if ((0 != strcmp(key, "a")) && (0 != strcmp(key, "b")))
    {
      printf("ERROR: got unexpected '%s'\n", key);
      num_errors++;
    }

  return MHD_YES;
}


int
main (int argc, char *const *argv)
{
  struct MHD_Connection connection;
  struct MHD_HTTP_Header header;
  struct MHD_PostProcessor *pp;
  (void)argc; (void)argv;  /* Unused. Silent compiler warning. */

  num_errors = 0;
  memset (&connection, 0, sizeof (struct MHD_Connection));
  memset (&header, 0, sizeof (struct MHD_HTTP_Header));
  connection.headers_received = &header;
  header.header = MHD_HTTP_HEADER_CONTENT_TYPE;
  header.value = MHD_HTTP_POST_ENCODING_FORM_URLENCODED;
  header.header_size = strlen (header.header);
  header.value_size = strlen (header.value);
  header.kind = MHD_HEADER_KIND;

  pp = MHD_create_post_processor (&connection,
                                  4096, &check_post, NULL);
  if (NULL == pp)
    return 1;

  const char* post = "a=xx+xx+xxx+xxxxx+xxxx+xxxxxxxx+xxx+xxxxxx+xxx+xxx+xxxxxxx+xxxxx%0A+++++++xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx%0A+++++++--%3E%0A++++++++++++++%3Cxxxxx+xxxxx%3D%22xxx%25%22%3E%0A+++++++++++%3Cxx%3E%0A+++++++++++++++%3Cxx+xxxxxxx%3D%22x%22+xxxxx%3D%22xxxxx%22%3E%0A+++++++++++++++++++%3Cxxxxx+xxxxx%3D%22xxx%25%22%3E%0A+++++++++++++++++++++++%3Cxx%3E%0A+++++++++++++++++++++++++++%3Cxx+xxxxx%3D%22xxxx%22%3E%0A+++++++++++++++++++++++++++++++%3Cx+xxxxx%3D%22xxxx-xxxxx%3Axxxxx%22%3Exxxxx%3A%3C%2Fx%3E%0A%0A+++++++++++++++++++++++++++++++%3Cx+xxxxx%3D%22xxxx-xxxxx%3Axxxxx%22%3Exxx%3A%3C%2Fx%3E%0A%0A+++++++++++++++++++++++++++++++%3Cx+xxxxx%3D%22xxxx-xxxxx%3Axxxxx%3B+xxxx-xxxxxx%3A+xxxx%3B%22%3Exxxxx+xxxxx%3A%3C%2Fx%3E%0A+++++++++++++++++++++++++++%3C%2Fxx%3E%0A+++++++++++++++++++++++%3C%2Fxx%3E%0A+++++++++++++++++++%3C%2Fxxxxx%3E%0A+++++++++++++++%3C%2Fxx%3E%0A+++++++++++++++%3Cxx+xxxxx%3D%22xxxx-xxxxx%3A+xxxxx%3B+xxxxx%3A+xxxx%22%3E%26xxxxx%3B+%3Cxxxx%0A+++++++++++++++++++++++xxxxx%3D%22xxxxxxxxxxxxxxx%22%3Exxxx.xx%3C%2Fxxxx%3E%0A+++++++++++++++%3C%2Fxx%3E%0A+++++++++++%3C%2Fxx%3E%0A++++++++++++++++++++++++++%3Cxx%3E%0A+++++++++++++++++++%3Cxx+xxxxx%3D%22xxxx-xxxxx%3A+xxxxx%3B+xxxxx%3A+xxxx%22%3E%26xxxxx%3B+%3Cxxxx%0A+++++++++++++++++++++++++++xxxxx%3D%22xxxxxxxxxxxxxxx%22%3Exxx.xx%3C%2Fxxxx%3E%0A+++++++++++++++++++%3C%2Fxx%3E%0A+++++++++++++++%3C%2Fxx%3E%0A++++++++++++++++++++++%3Cxx%3E%0A+++++++++++++++%3Cxx+xxxxx%3D%22xxxx-xxxxx%3A+xxxxx%3Bxxxx-xxxxxx%3A+xxxx%3B+xxxxx%3A+xxxx%22%3E%26xxxxx%3B+%3Cxxxx%0A+++++++++++++++++++++++xxxxx%3D%22xxxxxxxxxxxxxxx%22%3Exxxx.xx%3C%2Fxxxx%3E%3C%2Fxx%3E%0A+++++++++++%3C%2Fxx%3E%0A+++++++%3C%2Fxxxxx%3E%0A+++++++%3C%21--%0A+++++++xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx%0A+++++++xxx+xx+xxxxx+xxxxxxx+xxxxxxx%0A+++++++xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx%0A+++++++--%3E%0A+++%3C%2Fxxx%3E%0A%0A%0A%0A+++%3Cxxx+xxxxx%3D%22xxxxxxxxx%22+xx%3D%22xxxxxxxxx%22%3E%3C%2Fxxx%3E%0A%0A+++%3Cxxx+xx%3D%22xxxx%22+xxxxx%3D%22xxxx%22%3E%0A+++++++%3Cxxxxx+xxxxx%3D%22xxxxxxxxx%22%3E%0A+++++++++++%3Cxx%3E%0A+++++++++++++++%3Cxx+xxxxxxx%3D%22x%22+xx%3D%22xxxxxxxxxxxxx%22+xxxxx%3D%22xxxxxxxxxxxxx%22%3E%0A+++++++++++++++++++%3Cxxx+xx%3D%22xxxxxx%22%3E%3C%2Fxxx%3E%0A+++++++++++++++%3C%2Fxx%3E%0A+++++++++++%3C%2Fxx%3E%0A+++++++++++%3Cxx%3E%0A+++++++++++++++%3Cxx+xx%3D%22xxxxxxxxxxxxxxxxx%22+xxxxx%3D%22xxxxxxxxxxxxxxxxx%22%3E%3C%2Fxx%3E%0A+++++++++++++++%3Cxx+xx%3D%22xxxxxxxxxxxxxx%22+xxxxx%3D%22xxxxxxxxxxxxxx%22%3E%0A+++++++++++++++++++%3Cxxx+xx%3D%22xxxxxxx%22%3E%3C%2Fxxx%3E%0A+++++++++++++++%3C%2Fxx%3E%0A+++++++++++%3C%2Fxx%3E%0A+++++++++++%3Cxx%3E%0A+++++++++++++++%3Cxx+xxxxxxx%3D%22x%22+xx%3D%22xxxxxxxxxxxxx%22+xxxxx%3D%22xxxxxxxxxxxxx%22%3E%0A+++++++++++++++++++%3Cxxx+xx%3D%22xxxxxx%22%3E%3C%2Fxxx%3E%0A+++++++++++++++%3C%2Fxx%3E%0A+++++++++++%3C%2Fxx%3E%0A+++++++%3C%2Fxxxxx%3E%0A+++%3C%2Fxxx%3E%0A%3C%2Fxxx%3E%0A%0A%3Cxxx+xx%3D%22xxxxxx%22%3E%3C%2Fxxx%3E%0A%0A%3C%2Fxxxx%3E%0A%3C%2Fxxxx%3E+&b=value";

  if (MHD_YES != MHD_post_process (pp, post, strlen(post)))
    num_errors++;
  MHD_destroy_post_processor (pp);

  return num_errors == 0 ? 0 : 2;
}

