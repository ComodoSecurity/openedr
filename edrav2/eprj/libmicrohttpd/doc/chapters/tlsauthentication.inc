We left the basic authentication chapter with the unsatisfactory conclusion that
any traffic, including the credentials, could be intercepted by anyone between
the browser client and the server. Protecting the data while it is sent over
unsecured lines will be the goal of this chapter.

Since version 0.4, the @emph{MHD} library includes support for encrypting the
traffic by employing SSL/TSL. If @emph{GNU libmicrohttpd} has been configured to
support these, encryption and decryption can be applied transparently on the
data being sent, with only minimal changes to the actual source code of the example.


@heading Preparation

First, a private key for the server will be generated. With this key, the server
will later be able to authenticate itself to the client---preventing anyone else
from stealing the password by faking its identity. The @emph{OpenSSL} suite, which
is available on many operating systems, can generate such a key. For the scope of
this tutorial, we will be content with a 1024 bit key:
@verbatim
> openssl genrsa -out server.key 1024
@end verbatim
@noindent

In addition to the key, a certificate describing the server in human readable tokens
is also needed. This certificate will be attested with our aforementioned key. In this way,
we obtain a self-signed certificate, valid for one year.

@verbatim
> openssl req -days 365 -out server.pem -new -x509 -key server.key
@end verbatim
@noindent

To avoid unnecessary error messages in the browser, the certificate needs to
have a name that matches the @emph{URI}, for example, "localhost" or the domain.
If you plan to have a publicly reachable server, you will need to ask a trusted third party,
called @emph{Certificate Authority}, or @emph{CA}, to attest the certificate for you. This way,
any visitor can make sure the server's identity is real.

Whether the server's certificate is signed by us or a third party, once it has been accepted
by the client, both sides will be communicating over encrypted channels. From this point on,
it is the client's turn to authenticate itself. But this has already been implemented in the basic
authentication scheme.


@heading Changing the source code

We merely have to extend the server program so that it loads the two files into memory,

@verbatim
int
main ()
{
  struct MHD_Daemon *daemon;
  char *key_pem;
  char *cert_pem;

  key_pem = load_file (SERVERKEYFILE);
  cert_pem = load_file (SERVERCERTFILE);

  if ((key_pem == NULL) || (cert_pem == NULL))
  {
    printf ("The key/certificate files could not be read.\n");
    return 1;
  }
@end verbatim
@noindent

and then we point the @emph{MHD} daemon to it upon initalization.
@verbatim

  daemon = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_SSL,
  	   		     PORT, NULL, NULL,
                             &answer_to_connection, NULL,
                             MHD_OPTION_HTTPS_MEM_KEY, key_pem,
                             MHD_OPTION_HTTPS_MEM_CERT, cert_pem,
                             MHD_OPTION_END);

  if (NULL == daemon)
    {
      printf ("%s\n", cert_pem);

      free (key_pem);
      free (cert_pem);

      return 1;
    }
@end verbatim
@noindent


The rest consists of little new besides some additional memory cleanups.
@verbatim

  getchar ();

  MHD_stop_daemon (daemon);
  free (key_pem);
  free (cert_pem);

  return 0;
}
@end verbatim
@noindent


The rather unexciting file loader can be found in the complete example @code{tlsauthentication.c}.


@heading Remarks
@itemize @bullet
@item
While the standard @emph{HTTP} port is 80, it is 443 for @emph{HTTPS}. The common internet browsers assume
standard @emph{HTTP} if they are asked to access other ports than these. Therefore, you will have to type
@code{https://localhost:8888} explicitly when you test the example, or the browser will not know how to
handle the answer properly.

@item
The remaining weak point is the question how the server will be trusted initially. Either a @emph{CA} signs the
certificate or the client obtains the key over secure means. Anyway, the clients have to be aware (or configured)
that they should not accept certificates of unknown origin.

@item
The introduced method of certificates makes it mandatory to set an expiration date---making it less feasible to
hardcode certificates in embedded devices.

@item
The cryptographic facilities consume memory space and computing time. For this reason, websites usually consists
both of uncritically @emph{HTTP} parts and secured @emph{HTTPS}.

@end itemize


@heading Client authentication

You can also use MHD to authenticate the client via SSL/TLS certificates
(as an alternative to using the password-based Basic or Digest authentication).
To do this, you will need to link your application against @emph{gnutls}.
Next, when you start the MHD daemon, you must specify the root CA that you're
willing to trust:
@verbatim
  daemon = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_SSL,
  	   		     PORT, NULL, NULL,
                             &answer_to_connection, NULL,
                             MHD_OPTION_HTTPS_MEM_KEY, key_pem,
                             MHD_OPTION_HTTPS_MEM_CERT, cert_pem,
			     MHD_OPTION_HTTPS_MEM_TRUST, root_ca_pem,
                             MHD_OPTION_END);
@end verbatim

With this, you can then obtain client certificates for each session.
In order to obtain the identity of the client, you first need to
obtain the raw GnuTLS session handle from @emph{MHD} using
@code{MHD_get_connection_info}.

@verbatim
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

gnutls_session_t tls_session;
union MHD_ConnectionInfo *ci;

ci = MHD_get_connection_info (connection,
                              MHD_CONNECTION_INFO_GNUTLS_SESSION);
tls_session = ci->tls_session;
@end verbatim

You can then extract the client certificate:

@verbatim
/**
 * Get the client's certificate
 *
 * @param tls_session the TLS session
 * @return NULL if no valid client certificate could be found, a pointer
 *  	to the certificate if found
 */
static gnutls_x509_crt_t
get_client_certificate (gnutls_session_t tls_session)
{
  unsigned int listsize;
  const gnutls_datum_t * pcert;
  gnutls_certificate_status_t client_cert_status;
  gnutls_x509_crt_t client_cert;

  if (tls_session == NULL)
    return NULL;
  if (gnutls_certificate_verify_peers2(tls_session,
				       &client_cert_status))
    return NULL;
  pcert = gnutls_certificate_get_peers(tls_session,
				       &listsize);
  if ( (pcert == NULL) ||
       (listsize == 0))
    {
      fprintf (stderr,
	       "Failed to retrieve client certificate chain\n");
      return NULL;
    }
  if (gnutls_x509_crt_init(&client_cert))
    {
      fprintf (stderr,
	       "Failed to initialize client certificate\n");
      return NULL;
    }
  /* Note that by passing values between 0 and listsize here, you
     can get access to the CA's certs */
  if (gnutls_x509_crt_import(client_cert,
			     &pcert[0],
			     GNUTLS_X509_FMT_DER))
    {
      fprintf (stderr,
	       "Failed to import client certificate\n");
      gnutls_x509_crt_deinit(client_cert);
      return NULL;
    }
  return client_cert;
}
@end verbatim

Using the client certificate, you can then get the client's distinguished name
and alternative names:

@verbatim
/**
 * Get the distinguished name from the client's certificate
 *
 * @param client_cert the client certificate
 * @return NULL if no dn or certificate could be found, a pointer
 * 			to the dn if found
 */
char *
cert_auth_get_dn(gnutls_x509_crt_c client_cert)
{
  char* buf;
  size_t lbuf;

  lbuf = 0;
  gnutls_x509_crt_get_dn(client_cert, NULL, &lbuf);
  buf = malloc(lbuf);
  if (buf == NULL)
    {
      fprintf (stderr,
	       "Failed to allocate memory for certificate dn\n");
      return NULL;
    }
  gnutls_x509_crt_get_dn(client_cert, buf, &lbuf);
  return buf;
}


/**
 * Get the alternative name of specified type from the client's certificate
 *
 * @param client_cert the client certificate
 * @param nametype The requested name type
 * @param index The position of the alternative name if multiple names are
 * 			matching the requested type, 0 for the first matching name
 * @return NULL if no matching alternative name could be found, a pointer
 * 			to the alternative name if found
 */
char *
MHD_cert_auth_get_alt_name(gnutls_x509_crt_t client_cert,
			   int nametype,
			   unsigned int index)
{
  char* buf;
  size_t lbuf;
  unsigned int seq;
  unsigned int subseq;
  unsigned int type;
  int result;

  subseq = 0;
  for (seq=0;;seq++)
    {
      lbuf = 0;
      result = gnutls_x509_crt_get_subject_alt_name2(client_cert, seq, NULL, &lbuf,
						     &type, NULL);
      if (result == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
	return NULL;
      if (nametype != (int) type)
	continue;
      if (subseq == index)
	break;
      subseq++;
    }
  buf = malloc(lbuf);
  if (buf == NULL)
    {
      fprintf (stderr,
	       "Failed to allocate memory for certificate alt name\n");
      return NULL;
    }
  result = gnutls_x509_crt_get_subject_alt_name2(client_cert,
						 seq,
						 buf,
						 &lbuf,
						 NULL, NULL);
  if (result != nametype)
    {
      fprintf (stderr,
	       "Unexpected return value from gnutls: %d\n",
	       result);
      free (buf);
      return NULL;
    }
  return buf;
}
@end verbatim

Finally, you should release the memory associated with the client
certificate:

@verbatim
gnutls_x509_crt_deinit (client_cert);
@end verbatim



@heading Using TLS Server Name Indication (SNI)

SNI enables hosting multiple domains under one IP address with TLS.  So
SNI is the TLS-equivalent of virtual hosting.  To use SNI with MHD, you
need at least GnuTLS 3.0.  The main change compared to the simple hosting
of one domain is that you need to provide a callback instead of the key
and certificate.  For example, when you start the MHD daemon, you could
do this:
@verbatim
  daemon = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_SSL,
  	   		     PORT, NULL, NULL,
                             &answer_to_connection, NULL,
                             MHD_OPTION_HTTPS_CERT_CALLBACK, &sni_callback,
                             MHD_OPTION_END);
@end verbatim
Here, @code{sni_callback} is the name of a function that you will have to
implement to retrieve the X.509 certificate for an incoming connection.
The callback has type @code{gnutls_certificate_retrieve_function2} and
is documented in the GnuTLS API for the @code{gnutls_certificate_set_retrieve_function2}
as follows:

@deftypefn {Function Pointer} int {*gnutls_certificate_retrieve_function2} (gnutls_session_t, const gnutls_datum_t* req_ca_dn, int nreqs, const gnutls_pk_algorithm_t* pk_algos, int pk_algos_length, gnutls_pcert_st** pcert, unsigned int *pcert_length, gnutls_privkey_t * pkey)

@table @var
@item req_ca_cert
is only used in X.509 certificates. Contains a list with the CA names that the server considers trusted. Normally we should send a certificate that is signed by one of these CAs. These names are DER encoded. To get a more meaningful value use the function @code{gnutls_x509_rdn_get()}.

@item pk_algos
contains a list with server’s acceptable signature algorithms. The certificate returned should support the server’s given algorithms.

@item pcert
should contain a single certificate and public or a list of them.

@item pcert_length
is the size of the previous list.

@item pkey
is the private key.
@end table
@end deftypefn

A possible implementation of this callback would look like this:

@verbatim
struct Hosts
{
  struct Hosts *next;
  const char *hostname;
  gnutls_pcert_st pcrt;
  gnutls_privkey_t key;
};

static struct Hosts *hosts;

int
sni_callback (gnutls_session_t session,
              const gnutls_datum_t* req_ca_dn,
              int nreqs,
              const gnutls_pk_algorithm_t* pk_algos,
              int pk_algos_length,
              gnutls_pcert_st** pcert,
              unsigned int *pcert_length,
              gnutls_privkey_t * pkey)
{
  char name[256];
  size_t name_len;
  struct Hosts *host;
  unsigned int type;

  name_len = sizeof (name);
  if (GNUTLS_E_SUCCESS !=
      gnutls_server_name_get (session,
                              name,
                              &name_len,
                              &type,
                              0 /* index */))
    return -1;
  for (host = hosts; NULL != host; host = host->next)
    if (0 == strncmp (name, host->hostname, name_len))
      break;
  if (NULL == host)
    {
      fprintf (stderr,
               "Need certificate for %.*s\n",
               (int) name_len,
               name);
      return -1;
    }
  fprintf (stderr,
           "Returning certificate for %.*s\n",
           (int) name_len,
           name);
  *pkey = host->key;
  *pcert_length = 1;
  *pcert = &host->pcrt;
  return 0;
}
@end verbatim

Note that MHD cannot offer passing a closure or any other additional information
to this callback, as the GnuTLS API unfortunately does not permit this at this
point.

The @code{hosts} list can be initialized by loading the private keys and X.509
certificats from disk as follows:

@verbatim
static void
load_keys(const char *hostname,
          const char *CERT_FILE,
          const char *KEY_FILE)
{
  int ret;
  gnutls_datum_t data;
  struct Hosts *host;

  host = malloc (sizeof (struct Hosts));
  host->hostname = hostname;
  host->next = hosts;
  hosts = host;

  ret = gnutls_load_file (CERT_FILE, &data);
  if (ret < 0)
  {
    fprintf (stderr,
             "*** Error loading certificate file %s.\n",
             CERT_FILE);
    exit(1);
  }
  ret =
    gnutls_pcert_import_x509_raw (&host->pcrt, &data, GNUTLS_X509_FMT_PEM,
                                  0);
  if (ret < 0)
  {
    fprintf(stderr,
            "*** Error loading certificate file: %s\n",
            gnutls_strerror (ret));
    exit(1);
  }
  gnutls_free (data.data);

  ret = gnutls_load_file (KEY_FILE, &data);
  if (ret < 0)
  {
    fprintf (stderr,
             "*** Error loading key file %s.\n",
             KEY_FILE);
    exit(1);
  }

  gnutls_privkey_init (&host->key);
  ret =
    gnutls_privkey_import_x509_raw (host->key,
                                    &data, GNUTLS_X509_FMT_PEM,
                                    NULL, 0);
  if (ret < 0)
  {
    fprintf (stderr,
             "*** Error loading key file: %s\n",
             gnutls_strerror (ret));
    exit(1);
  }
  gnutls_free (data.data);
}
@end verbatim

The code above was largely lifted from GnuTLS.  You can find other
methods for initializing certificates and keys in the GnuTLS manual
and source code.
