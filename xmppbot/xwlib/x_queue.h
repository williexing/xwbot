/*
 * x_utils.h
 *
 *  Created on: 27.10.2010
 *      Author: artur
 */

#ifndef X_QUEUE_H_
#define X_QUEUE_H_

#if 1
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>

#define X_SEMAPHORE_DECLARE(x) sem_t x
#define x_sem_init(x) sem_init(&x,0,0)
#define x_sem_post(x) sem_post(&x)
#define x_sem_wait(x) sem_wait(&x)
#define X_CRITSECT_DECLARE(x) pthread_mutex_t x
#define x_cs_init(x) pthread_mutex_init(&x,NULL)
#define x_cs_enter(x) pthread_mutex_lock(&x)
#define x_cs_try_enter(x) pthread_mutex_trylock(&x)
#define x_cs_exit(x) pthread_mutex_unlock(&x)
#endif

typedef struct x_msg
{
  struct x_msg *next;
  struct x_msg *prev;
  struct timeval stamp;
  unsigned int len;
  unsigned int data_len;
  unsigned char *head;
  unsigned char *end;
  unsigned char *data;
  unsigned char *tail;
} x_msg_t;

typedef struct x_queue
{
  X_SEMAPHORE_DECLARE(cs_sem);
  X_CRITSECT_DECLARE(cs_mx);
  unsigned int qlen;
  x_msg_t *head;
  x_msg_t *tail;
} x_queue_t;

x_queue_t *x_queue_alloc(void);
void x_queue_init(x_queue_t *queue);
x_msg_t *x_queue_dequeue_head(x_queue_t *queue);
void x_queue_queue_tail(x_queue_t *queue, x_msg_t *msg_buf);

/*#define UNIT_TESTS*/
#ifdef UNIT_TESTS
void qtest1(void);
#endif

#endif /* X_UTILS_H_ */
