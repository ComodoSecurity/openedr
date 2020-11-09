This chapter discusses how one should manage sessions, that is, share state between multiple
HTTP requests from the same user.  We use a simple example where the user submits multiple
forms and the server is supposed to accumulate state from all of these forms.  Naturally, as
this is a network protocol, our session mechanism must support having many users with
many concurrent sessions at the same time.

In order to track users, we use a simple session cookie.  A session cookie expires when the
user closes the browser.  Changing from session cookies to persistent cookies only requires
adding an expiration time to the cookie.  The server creates a fresh session cookie whenever
a request without a cookie is received, or if the supplied session cookie is not known to
the server.

@heading Looking up the cookie

Since MHD parses the HTTP cookie header for us, looking up an existing cookie
is straightforward:

@verbatim
const char *value;

value = MHD_lookup_connection_value (connection,
                                     MHD_COOKIE_KIND,
                                     "KEY");
@end verbatim

Here, "KEY" is the name we chose for our session cookie.


@heading Setting the cookie header

MHD requires the user to provide the full cookie format string in order to set
cookies.  In order to generate a unique cookie, our example creates a random
64-character text string to be used as the value of the cookie:

@verbatim
char value[128];
char raw_value[65];

for (unsigned int i=0;i<sizeof (raw_value);i++)
  raw_value = 'A' + (rand () % 26); /* bad PRNG! */
raw_value[64] = '\0';
snprintf (value, sizeof (value),
          "%s=%s",
          "KEY",
          raw_value);
@end verbatim

Given this cookie value, we can then set the cookie header in our HTTP response
as follows:

@verbatim
assert (MHD_YES ==
        MHD_set_connection_value (connection,
                                  MHD_HEADER_KIND,
                                  MHD_HTTP_HEADER_SET_COOKIE,
                                  value));
@end verbatim


@heading Remark: Session expiration

It is of course possible that clients stop their interaction with the
server at any time.  In order to avoid using too much storage, the
server must thus discard inactive sessions at some point.  Our example
implements this by discarding inactive sessions after a certain amount
of time.  Alternatively, the implementation may limit the total number
of active sessions.  Which bounds are used for idle sessions or the
total number of sessions obviously depends largely on the type of
the application and available server resources.

@heading Example code

A sample application implementing a website with multiple
forms (which are dynamically created using values from previous
POST requests from the same session) is available
as the example @code{sessions.c}.

Note that the example uses a simple, $O(n)$ linked list traversal to
look up sessions and to expire old sessions.  Using a hash table and a
heap would be more appropriate if a large number of concurrent
sessions is expected.

@heading Remarks

Naturally, it is quite conceivable to store session data in a database
instead of in memory.  Still, having mechanisms to expire data
associated with long-time idle sessions (where the business process
has still not finished) is likely a good idea.
