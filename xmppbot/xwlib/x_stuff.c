/**
 *  x_stuff.c
 *  gobee
 *
 *  Created by артемка on 21.04.13.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <malloc.h>
#endif

#include "x_lib.h"
#include "x_utils.h"

#define MIN(x,y) ((x) < (y) ? (x) : (y))

void
circbuf_init(circular_buffer_t *cb, int size)
{
  cb->head = cb->tail = cb->start = (uint8_t *)x_malloc(size + 1);
  cb->end = cb->start + size + 1;
  cb->wcnt = cb->rcnt = 0;
}

void
circbuf_deinit(circular_buffer_t *cb)
{
    void *ptr = cb->start;
    cb->start = cb->end = cb->head = cb->tail;
    x_free(ptr);
}

int
circbuf_length(circular_buffer_t *cb)
{
  int l1,l2;
  int len = 0;
  
  if (cb->head <= cb->tail)
  {
    len = cb->tail - cb->head;
  }
  else
  {
    l1 = cb->end - cb->head;
    l2 = cb->tail - cb->start;
    len = l1 + l2;
  }

  return len;
}

#if 0
int
circbuf_write(circular_buffer_t *cb, uint8_t *data, int size)
{
  int ret = 0;
  int l1=0,l2=0, l3=0;
    
  if (cb->head <= cb->tail)
  {
    l1 = cb->end - cb->tail;
    l3 = MIN(l1,size);
    x_memcpy(cb->tail,data,l3);
    cb->tail = cb->tail + l3;
    ret += l3;
    data += l3;
    
    if (l3 < size)
    {
      l2 = size - l3;
      l3 = cb->head - cb->start;
      l3 = MIN(l3,l2); // reserve 1 byte
      x_memcpy(cb->start,data, l3);
      if(l3) cb->tail = cb->start + l3;
      ret += l3;
      data += l3;
   }
  }
  else
  {
    l1 = cb->head - cb->tail;
    l3 = MIN(l1,size);
    x_memcpy(cb->tail,data, l3);
    cb->tail = cb->tail + l3;
    ret += l3;
    data += l3;
  }
  
  cb->wcnt++;
  return ret;
}

int
circbuf_read(circular_buffer_t *cb, const uint8_t *data, int size)
{
  int ret = 0;
  int l1=0,l2=0, l3=0;
  
  if (cb->head > cb->tail)
  {
    l1 = cb->end - cb->head;
    l3 = MIN(l1,size);
    x_memcpy(data, cb->tail, l3);
    cb->head = cb->head + l3;
    ret += l3;
    data += l3;
    
    if (l3 < size)
    {
      l2 = size - l3;
      l3 = cb->tail - cb->start;
      l3 = MIN(l3,l2); // reserve 1 byte
      x_memcpy(data, cb->start, l3);
      cb->head = cb->start + l3;
      ret += l3;
      data += l3;
    }
  }
  else
  {
    l1 = cb->tail - cb->head;
    l3 = MIN(l1,size);
    x_memcpy(data, cb->head, l3);
    cb->head = cb->head + l3;
    ret += l3;
    data += l3;
  }
  
  cb->rcnt++;
  return ret;
}

#else
int
circbuf_read(circular_buffer_t *cb, uint8_t *buf, int buf_size)
{
    int len;
    int size = cb->tail - cb->head;
    if (size < 0)
        size += cb->end - cb->start;

    if (size < buf_size)
        return -1;
    while (buf_size > 0) {
        len = MIN(cb->end - cb->head, buf_size);
        memcpy(buf, cb->head, len);
        buf += len;
        cb->head += len;
        if (cb->head >= cb->end)
            cb->head = cb->start;
        buf_size -= len;
    }
    return 0;
}

int
circbuf_write(circular_buffer_t *cb, const uint8_t *buf, int size)
{
    int len;

    while (size > 0) {
        len = MIN(cb->end - cb->tail, size);
        memcpy(cb->tail, buf, len);
        cb->tail += len;
        if (cb->tail >= cb->end)
            cb->tail = cb->start;
        buf += len;
        size -= len;
    }
}
#endif

#define UNIT_TEST
#ifdef UNIT_TEST
int
main (int argc, char **argv)
{
    circular_buffer_t cb;
    circbuf_init(&cb,8192);
    circbuf_deinit(&cb);
}
#endif
