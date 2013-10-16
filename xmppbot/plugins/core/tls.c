/*
 * tls.c
 *
 *  Created on: Aug 20, 2011
 *      Author: artemka
 */

#include <stdio.h>
#include <stdlib.h>

#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_XMPPTLS_ON
#define DEBUG_PRFX "[tls] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>

#include <openssl/ssl.h>

#if 0
#define CA_LIST "root.pem"
#define KEYFILE "client.pem"
#define PASSWORD "password"

static char *pass;

static int
password_cb (char *buf, int size, int rwflag, void *userdata)
  {
    if (size < strlen(pass) + 1) return (0);
    return (strlen(strcpy(buf,pass)));
  }
#endif

static int xmllogfd;

struct ssl_transport
{
  struct x_transport super;
  CS_DECL(ssl_lock)
  ;
  SSL *ssl;
  SSL_CTX *ctx;
};

static int
tls_write(struct x_transport *ss, char *buf, size_t len, int flags)
{
  int sslerr;
  int err;
  struct ssl_transport *sslt = (struct ssl_transport *) ss;

  if (len == 0)
    return 0;

  CS_LOCK(&sslt->ssl_lock);
  do
    {
      err = SSL_write(sslt->ssl, buf, len);
      if (err <= 0)
        {
          sslerr = SSL_get_error(sslt->ssl, err);
          TRACE("SSL_ERROR: %d, write()\n", sslerr);
        }
      else
        {
          write(xmllogfd, buf, err);
          fsync(xmllogfd);
        }
    }
  while (0 /* sslerr == SSL_ERROR_WANT_WRITE */);
  CS_UNLOCK(&sslt->ssl_lock);

  if (err > 0 && err < len)
    {
      fprintf(stderr,"err=%d,len=%d\n",err, len);
      write(2, buf, len);
      BUG();
    }
  return err;
}

static int
tls_read(struct x_transport *ss, char *buf, size_t len, int flags)
{
  int sslerr;
  int err;
  struct ssl_transport *sslt = (struct ssl_transport *) ss;

  if (len == 0)
    return 0;

  CS_LOCK(&sslt->ssl_lock);
//  do
    {
      err = SSL_read(sslt->ssl, buf, len);
      if (err <= 0)
        {
          sslerr = SSL_get_error(sslt->ssl, err);
          TRACE("SSL_ERROR: %d, read(),SSL_ERROR_WANT_READ=%d,"
          "SSL_ERROR_WANT_WRITE=%d \n",
              sslerr, SSL_ERROR_WANT_READ, SSL_ERROR_WANT_WRITE);
        }
      else
        {
//          write(xmllogfd, buf, err);
          fsync(xmllogfd);
        }
    }
//  while (sslerr == SSL_ERROR_WANT_READ);

  CS_UNLOCK(&sslt->ssl_lock);
  return err;
}

/*
 static int
 tls_on_wr(struct x_transport *ss, char *buf, size_t len, int flags)
 {
 int err = 0;
 ENTER;EXIT;
 return err;
 }
 */

EXPORT_SYMBOL void
xmpp_stop_tls(struct x_transport *ss)
{
  struct ssl_transport *sslt = (struct ssl_transport *) ss;
  if (sslt->ctx)
    SSL_CTX_free(sslt->ctx);
  sslt->ctx = NULL;
  sslt->ssl = NULL;
}

/**
 * Initializes ssl (tls) context for given transport
 *
 */
static int
initialize_ctx(struct ssl_transport *sslt)
{
  SSL_METHOD *m = NULL;
  ENTER;

  CS_INIT(&sslt->ssl_lock);

  CS_LOCK(&sslt->ssl_lock);

  m = SSLv23_client_method();
  // m = TLSv1_client_method();
  ENTER;
  sslt->ctx = SSL_CTX_new(m);

  ENTER;

  if (sslt->ctx == NULL)
    {
      TRACE("SSL Error");
      CS_UNLOCK(&sslt->ssl_lock);
      return -1;
    }
  // SSL_CTX_set_options(sess->ssl_ctx, SSL_OP_NO_SSLv2);

  ENTER;
  sslt->ssl = SSL_new(sslt->ctx);

  EXIT;

  xmllogfd = open("ssl_xmpp.xml", O_CREAT | O_TRUNC | O_ASYNC | O_RDWR,
      S_IRWXU | S_IRWXG);

  CS_UNLOCK(&sslt->ssl_lock);
  return 0;
}

/**
 * Starts SSL negotiations on given socket
 * Creates and returns inherited transport object
 */EXPORT_SYMBOL struct x_transport *
xmpp_start_tls(int sock)
{
  int err;
  struct ssl_transport *sslt = malloc(sizeof(struct ssl_transport));
  memset(sslt, 0, sizeof(struct ssl_transport));

  TRACE("Allocated %d bytes where %d bytes of x_transport\n",
      (int)sizeof(struct ssl_transport), (int)sizeof(struct x_transport));

  SSL_load_error_strings();
  SSL_library_init();

  /* Build our SSL context*/
  if (initialize_ctx(sslt))
    TRACE("Error tls init\n");

  if (!SSL_set_fd(sslt->ssl, sock))
    {
      /*ERR_print_errors_fp(stderr);*/
      goto tls_err;
    }

  /*
   FIXME Think hard on this non-blocking SSL
   Temporarily reset nonblocking option
   */
  unset_sock_to_non_blocking(sock);

  if ((err = SSL_connect(sslt->ssl)) <= 0)
    {
      TRACE("ERROR!!! SSL Connect failed (ERR=%d)\n",
          SSL_get_error(sslt->ssl,err));
      goto tls_err;
    }
  else
    {
      TRACE("OK!!! SSL Connection established\n");
    }

  sslt->super.rd = &tls_read;
  sslt->super.wr = &tls_write;

  /*go back to nonblocking mode*/
  set_sock_to_non_blocking(sock);

  return (struct x_transport *) sslt;

  tls_err: TRACE("ERROR!!! SSL Connect failed\n");
  xmpp_stop_tls((struct x_transport *) sslt);
  free(sslt);
  return NULL;

}
