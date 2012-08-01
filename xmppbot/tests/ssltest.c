/*
 * ssltest.c
 *
 *  Created on: Aug 20, 2011
 *      Author: artemka
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <openssl/ssl.h>

struct ssl_transport
{
  // struct x_transport super;
  SSL *ssl;
  SSL_CTX *ctx;
};

static int
initialize_ctx(struct ssl_transport *sslt)
{
  SSL_load_error_strings();
  SSL_library_init();

  sslt->ctx = SSL_CTX_new(SSLv23_client_method());

  if (sslt->ctx == NULL)
    {
      printf("SSL Error\n");
      return -1;
    }
  else
    printf("SSL Ok\n");

  // SSL_CTX_set_options(sess->ssl_ctx, SSL_OP_NO_SSLv2);

  sslt->ssl = SSL_new(sslt->ctx);

  printf("New Ssl created\n");

  return 0;
}


int main(int argc,char **argv)
{
  struct ssl_transport *sslt = calloc(1, sizeof(struct ssl_transport));

  printf("Running test\n");

  return initialize_ctx(sslt);

}

