/*
 * stun_test.c
 *
 *  Created on: 17.11.2010
 *      Author: artur
 */

#include <sys/select.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <xmppagent.h>
#include <stun.h>
#include <crc32.h>

// #include <x_utils.h>
// #include <x_utils.h>
#include <x_utils.h>
#include <x_stun.h>
#include <x_jingle.h>

struct XmppBus xsess;

int
test_ice1(void)
{
  int sock;
  struct sockaddr cliaddr;
  struct JingleSession *j_sess;
  in_port_t *q;

  j_sess = JingleSession_new(&xsess,"test1_ice_sess");
  if (!j_sess)
    /* FAIL */
    return -1;

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
      return -1;

  /* setup address */
  memset(&cliaddr, 0, sizeof(cliaddr));
  cliaddr.sa_family = AF_INET;
  q = (in_port_t *)cliaddr.sa_data;
  *q = htons(48642);

  if (bind (sock,&cliaddr,sizeof(cliaddr)) < 0)
    return -1;

  j_sess->sock = sock;
  if (j_sess->ice)
    j_sess->ice(j_sess,"stun.jabber.ru","5249");

  close(sock);
  return (j_sess->lasterr);
}


int main (int argc, char **argv)
{
  struct x_timer *ta;
  volatile int q = 0;

  printf("============ TIME TESTS ============\n");
  printf("Test 1: ");
  ta = timer_new(TIMER_REGULAR, 1000000,&q);
  if (!timer_start(ta))
    while (!q);
  if (q)
    printf("Ok!\n");
  else
    printf("ERROR\n");

  printf("============ ICE TESTS ============\n");
  printf("Test 1: ");
  if (!test_ice1())
    printf("Ok!\n");
  else
    printf("ERROR\n");

  return(0);
}
