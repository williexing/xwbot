/*
 * x_queue.c
 *
 *  Created on: 27.10.2010
 *      Author: artur
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#undef DEBUG_PRFX
#define DEBUG_PRFX "[x_queue] "
#include <xw/x_utils.h>

#include "x_queue.h"

EXPORT_SYMBOL __inline__ x_queue_t *
x_queue_alloc(void)
{
  return (x_queue_t *) malloc(sizeof(x_queue_t));
}

EXPORT_SYMBOL void
x_queue_init(x_queue_t *q)
{
  q->head = q->tail = NULL;
  q->qlen = 0;
  x_sem_init(q->cs_sem);
  x_cs_init(q->cs_mx);
}

EXPORT_SYMBOL x_msg_t *
x_queue_dequeue_head(x_queue_t *q)
{
  x_msg_t *ret = NULL;

#warning "This blocks on queue..."
  x_sem_wait(q->cs_sem);

  if (!q->qlen)
    return NULL;

  x_cs_enter(q->cs_mx);

  if (q->tail == q->head)
    {
      ret = q->head;
      q->tail = q->head = NULL;
    }
  else
    {
      ret = q->head;
      q->head = q->head->prev;
    }

  x_cs_exit(q->cs_mx);

  return ret;
}

EXPORT_SYMBOL void
x_queue_queue_tail(x_queue_t *q, x_msg_t *mbuf)
{
  x_cs_enter(q->cs_mx);
  if (!q->tail)
    {
      q->head = q->tail = mbuf;
    }
  else
    {
      q->tail->prev = mbuf;
      q->tail = mbuf;
    }
  q->qlen++;
  x_cs_exit(q->cs_mx);
  x_sem_post(q->cs_sem);
}

/*-------------------------------------------*
 *           UNIT_TESTS                      *
 *-------------------------------------------*/
#ifdef UNIT_TESTS

static void *
cons(void *data)
{
  x_queue_t *q = (x_queue_t *) data;
  x_msg_t *msg;
  int i = 1000;
  printf("\t\tcons:\n");

  for (; i; i--)
    {
      msg = x_queue_dequeue_head(q);
      if (msg)
        {
          printf("\t\t<== cons=%d\n", msg->len);
        }
    }
  return NULL;
}

static void *
prod(void *data)
{
  x_queue_t *q = (x_queue_t *) data;
  x_msg_t *msg;
  int i = 1000;
  printf("\tprod:\n");

  for (; i; i--)
    {
      msg = (x_msg_t *) malloc(sizeof(x_msg_t));
      if (msg)
        {
          msg->len = i;
          printf("\tprod=%d ==>\n", msg->len);
          x_queue_queue_tail(q, msg);
        }
    }
  return NULL;
}

void
qtest1(void)
{
  x_queue_t *mqueue;
  pthread_t t[2];

  printf("/----------------------------/\n");
  printf("/------  queue test 1 -------/\n");
  printf("/----------------------------/\n");

  mqueue = (x_queue_t *) malloc(sizeof(x_queue_t));
  x_queue_init(mqueue);

  pthread_create(&t[1], NULL, &prod, mqueue);
  pthread_create(&t[0], NULL, &cons, mqueue);
  pthread_join(t[1], 0);
  pthread_join(t[0], 0);
}

#endif

