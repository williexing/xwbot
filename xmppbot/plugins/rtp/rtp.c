/*
 * Copyright (c) 2010, artur
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * rtp.c
 *
 *  Created on: Aug 20, 2010
 *      Author: artur
 */

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>

#include <speex/speex.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include <pthread.h>
#include <xmppagent.h>

#include <x_codec.h>
#include <rtp.h>

#include <ezxml.h>

const unsigned long long EPOCH = 2208988800ULL;
const unsigned long long NTP_SCALE_FRAC = 4294967295ULL;

/**
 *	@brief consumer tasklet
 */
void *
rtp_consumer_tasklet(void *arg)
{
  ezxml_t params;
  ezxml_t ez_target;
  struct timeval tp;
  struct timespec ts;
  long long int abstime;
  long int usecs;
  int sock;
  p_tlv_t data;
  queue_t *queue = NULL;
  pthread_mutex_t mx;
  struct sockaddr_in cliaddr;
  char *ip;
  int port;
  int *sstate;

  ENTER;

  params = (ezxml_t) arg;
  if (!params)
    return NULL;

  TRACE("%s\n",ezxml_toxml(params));

  ez_target = ezxml_child(params, "target");
  if (!ez_target)
    goto cons_exit;

  if (ezxml_attr(ez_target, "socket"))
    {
      sock = (int) atoi(ezxml_attr(ez_target, "socket"));
    }
  if (sock < 0)
    {
      TRACE("INVALID SOCKET %d!!!!!!!!!!!!!!!!!!!!\n",sock);
      goto cons_exit;
    }

  ip = (char *) ezxml_attr(ez_target, "ip");
  port = atoi(ezxml_attr(ez_target, "port"));
  sstate = (int *) atoi(ezxml_attr(ez_target, "session-control"));

  cliaddr.sin_family = AF_INET;
  cliaddr.sin_port = htons(port);
  inet_aton(ip, &cliaddr.sin_addr);

  /* duplicate socket */
  sock = dup(sock);

  if (ezxml_attr(params, "dataqueue"))
    {
      queue = (void *) atoi(ezxml_attr(params, "dataqueue"));
    }
  if (!queue)
    {
      TRACE("INVALID QUEUE ADDR 0x%x !!!!!!!!!!!!!!!!!!!\n",(int)queue);
      goto cons_exit;
    }

  if (ezxml_attr(params, "period"))
    {
      usecs = (long) atol(ezxml_attr(params, "period"));
    }
  if (!usecs)
    {
      TRACE("INVALID QUEUE ADDR %ld !!!!!!!!!!!!!!!!!!!\n",usecs);
      goto cons_exit;
    }

  TRACE("STARTING RTP SERVER with PERIOD of %ld\n",(long int)usecs);
  ezxml_free(params);
  params = NULL;

  /* create timer mutex */
  pthread_mutex_init(&mx, NULL);
  pthread_mutex_lock(&mx);

  gettimeofday(&tp, NULL);
  /* Convert from timeval to timespec */
  abstime = ((long long int) tp.tv_sec) * 1000000 + (long long int) tp.tv_usec;
  abstime += usecs;
  tp.tv_sec = (__time_t ) (abstime / 1000000);
  tp.tv_usec = (__suseconds_t ) (abstime % 1000000);
  ts.tv_sec = tp.tv_sec;
  ts.tv_nsec = tp.tv_usec * 1000;

  for (; *sstate;)
    {
      /* increment timer */
      abstime = ((long long int) ts.tv_sec) * 1000000 + ts.tv_nsec / 1000;
      abstime += usecs;
      ts.tv_sec = (__time_t ) (abstime / 1000000);
      ts.tv_nsec = (long int) (abstime % 1000000) * 1000;
      /* send data */
      /*
       TRACE ("%lld [%d:%ld]\n",
       abstime,
       (int)ts.tv_sec,
       (long int)ts.tv_nsec);
       */
      data = queue_pop_front_data(queue);
      if (data)
        {
          sendto(sock, data->value, data->len, 0, (struct sockaddr *) &cliaddr,
              sizeof(struct sockaddr));
          free(data);
        }
      /* wait */
      pthread_mutex_timedlock(&mx, &ts);
    }

  TRACE ("CONSUMER THREAD END\n");

  close(sock);

  pthread_mutex_unlock(&mx);
  pthread_mutex_destroy(&mx);

  if (queue)
    queue_free_w_data(queue);

  TRACE ("CONSUMER THREAD EXIT\n");

  /* wait half second */
  usleep(500);

  TRACE ("CONSUMER THREAD OK\n");

  EXIT;

  cons_exit:
  /* free arguments as soon as possible */
  if (params)
    ezxml_free(params);
  pthread_exit(NULL);
}

/**
 * @brief source tasklet
 */
void *
rtp_source_tasklet(void *arg)
{
  FILE *fin;
  ezxml_t params;
  ezxml_t ez_target;
  OggVorbis_File vf;
  rtp_hdr_t *_rtp;
  int eof = 0;
  int current_section;
  long int len;
  int i;
  int seq;
  p_tlv_t data;
  queue_t *queue = NULL;
  uint32_t tstamp = 1234;
  void *state;
  char *fname;
  SpeexBits bits;
  vorbis_info *vi;
  int mul;
  int framebytesiz;
#define SPEEX_BRATE 8000
#define PCM_FRAME_LEN 160
#define SPEEX_FRAME_LEN 200
#define _MAX_SEC 100
  float pcmdata[PCM_FRAME_LEN];
  char *ptr, *p1, *p3;

  int enclen = SPEEX_FRAME_LEN;
  long int usecs = 20000;

  params = (ezxml_t) arg;
  if (!params)
    return NULL;

  // 	TRACE("%s\n",ezxml_toxml(params));

  ENTER;
  ez_target = ezxml_child(params, "source");
  if (!ez_target)
    goto src_exit;

  if (ezxml_attr(params, "dataqueue"))
    {
      queue = (void *) atoi(ezxml_attr(params, "dataqueue"));
    }
  if (!queue)
    {
      TRACE("INVALID QUEUE ADDR 0x%x !!!!!!!!!!!!!!!!!!!\n",(int)queue);
      goto src_exit;
    }

  if (ezxml_attr(params, "period"))
    {
      usecs = (long) atol(ezxml_attr(params, "period"));
    }
  if (!usecs)
    {
      TRACE("INVALID PERIOD VALUE %ld !!!!!!!!!!!!!!!!!!!\n",usecs);
      goto src_exit;
    }

  if (ezxml_attr(ez_target, "href"))
    {
      fname = (char *) ezxml_attr(ez_target, "href");
    }
  if (!fname)
    {
      TRACE("INVALID SOURCE REFERENCE !!!!!!!!!!!!!!!!!!!\n");
      goto src_exit;
    }

  fin = fopen(fname, "r");
  if (!fin)
    goto src_exit;

  if (ov_open_callbacks(fin, &vf, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0)
    {
      TRACE("Input does not appear to be an Ogg bitstream.\n");
      goto src_exit;
    }

  ezxml_free(params);
  params = NULL;

  vi = ov_info(&vf, -1);
  mul = vi->rate / SPEEX_BRATE;

  TRACE("Bitstream is %d channel, %ldHz, %d\n"
      , vi->channels, vi->rate, mul);

  /* calculate frame size in bytes */
  framebytesiz = PCM_FRAME_LEN * 2 * vi->channels * mul;
  ptr = (char *) malloc(framebytesiz);
  p1 = ptr;
  p3 = ptr + framebytesiz;

  /* create speex state */
  state = x_speex_default_state(&bits);
  if (!state)
    {
      TRACE("INVALID SPEEX STATE !!!!!!!!!!!!!!!!!!!!\n");
      queue_free_w_data(queue);
      goto src_exit;
    }

  while (!eof)
    {
      //	TRACE("Decoding %d bytes\n",p3-p1);
      len = ov_read(&vf, (char *) p1, p3 - p1, 0, 2, 1, &current_section);
      if (len == 0)
        {
          eof = 1;
        }
      else if (len < 0)
        {
          eof = 1;
        }
      else
        {
          // TRACE("Decoded %ld bytes\n",len);
          if (len < (p3 - p1))
            {
              p1 += len;
            }
          else
            {
              /* reset pointer */
              p1 = ptr;
              for (i = 0; i < PCM_FRAME_LEN; i++)
                pcmdata[i] = *(short *) (ptr + mul * i * vi->channels
                    * sizeof(short));

              enclen = 200;
              x_speex_encdata(state, &bits, pcmdata, PCM_FRAME_LEN, ptr,
                  &enclen);

              //	TRACE("ENCODED %d bytes\n",enclen);
              data = malloc(sizeof(tlv_t) + sizeof(rtp_hdr_t) + enclen);
              data->len = sizeof(rtp_hdr_t) + enclen;
              data->type = 1;
              _rtp = (rtp_hdr_t *) data->value;
              _rtp->octet1.control1.v = 2;
              _rtp->octet1.control1.p = 0;
              _rtp->octet1.control1.x = 0;
              _rtp->octet1.control1.cc = 0;
              _rtp->octet2.control2.m = 0;
              _rtp->octet2.control2.pt = 97 & 0x7f;
              _rtp->seq = htons(seq++);
              _rtp->ssrc_id = 1234;
              tstamp += PCM_FRAME_LEN;
              _rtp->t_stamp = htonl(tstamp);
              memcpy(_rtp->payload, ptr, enclen);

              if (queue_push_back_data(queue, data) < 0)
                break;
            }
        }
    }
  if (ptr)
    free(ptr);

  ov_clear(&vf);
  fclose(fin);

  TRACE ("SOURCE THREAD EXIT\n");

  EXIT;

  src_exit:
  /* free arguments as soon as possible */
  if (params)
    ezxml_free(params);
  return NULL;
}

/**
 *
 */
void *
rtp_server(void *arg)
{
  char buf[25];
  ezxml_t params;
  ezxml_t ez_tmp;
  ezxml_t params2;
  ezxml_t params3;
  queue_t *queue = NULL;
  pthread_t t1;
  pthread_t t2;
  long int usecs = 20000;

  params = (ezxml_t) arg;
  if (!params)
    return NULL;

  params2 = ezxml_new("params2");
  if (!params2)
    {
      return NULL;
    }
  params3 = ezxml_new("params3");
  if (!params3)
    {
      ezxml_free(params2);
      return NULL;
    }

  /* maximum 256 frames per queue */
  queue = queue_new(256, 12);
  if (!queue)
    {
      TRACE("INVALID QUEUE!!!!!!!!!!!!!!!!!!!!\n");
      ezxml_free(params);
      ezxml_free(params2);
      return NULL;
    }

  sprintf(buf, "%d", (int) queue);
  ezxml_set_attr_d(params2,"dataqueue",buf);
  ezxml_set_attr_d(params3,"dataqueue",buf);

  sprintf(buf, "%ld", (long int) usecs);
  ezxml_set_attr_d(params2,"period",buf);
  ezxml_set_attr_d(params3,"period",buf);

  ez_tmp = ezxml_child(params, "target");
  ez_tmp = ezxml_cut(ez_tmp);
  if (ez_tmp)
    ezxml_insert(ez_tmp, params2, 0);

  ez_tmp = ezxml_add_child(params3, "source", 0);
  ezxml_set_attr_d(ez_tmp,"href",
      "/WORK/audiotest.ogg");

  /* start consumer */
  pthread_create(&t2, NULL, rtp_source_tasklet, (void *) params3);
  /* start consumer */
  pthread_create(&t1, NULL, rtp_consumer_tasklet, (void *) params2);

  pthread_exit(NULL);
}

/****************************************************************
 *
 *	Jingle Player Command Interface
 *
 *****************************************************************/
static ezxml_t
player_do_cmd(struct x_bus *sess, ezxml_t form)
{
  char *func = NULL;
  char *action, *sessid;

  ezxml_t _stnz1 = NULL;
  ezxml_t _stnz2 = NULL;
  ezxml_t _stnz3 = NULL;
  ezxml_t _xform = NULL;

  func = (char *) ezxml_attr(form, XML_TAG_NODE);
  action = (char *) ezxml_attr(form, XML_TAG_ACTION);
  sessid = (char *) ezxml_attr(form, XML_TAG_SESSIONID);

  // create command response
  _stnz1 = ezxml_new(XML_TAG_COMMAND);
  ezxml_set_attr(_stnz1, XML_TAG_XMLNS, XMLNS_COMMANDS);
  ezxml_set_attr_d(_stnz1,XML_TAG_NODE,func);
  if (sessid != NULL)
    ezxml_set_attr_d(_stnz1,XML_TAG_SESSIONID,sessid);

  _xform = ezxml_get(form, XML_TAG_X, -1);

  // switch xforms command
  if ((action != NULL && !strcmp(action, XML_TAG_CANCEL)) || (_xform != NULL
      && !strcmp(ezxml_attr(_xform, "type"), XML_TAG_CANCEL)))
    {
      // TODO Change status upon parameters list!
      ezxml_set_attr(_stnz1, XML_TAG_STATUS, "canceled");
      _stnz2 = ezxml_add_child(_stnz1, XML_TAG_NOTE, 0);
      ezxml_set_attr(_stnz2, XML_TAG_TYPE, "info");
      ezxml_set_txt(_stnz2, "Command canceled");
    }
  // return command help
  else
  // if (!_xform ||
  // strcmp(xforms_get_type(_xform),XML_TAG_SUBMIT))
    {
      // TODO Change status upon parameters list!
      ezxml_set_attr(_stnz1, XML_TAG_STATUS, "executing");
      // add actions node
      _stnz2 = ezxml_add_child(_stnz1, "actions", 0);
      ezxml_set_attr(_stnz2, "execute", "complete");
      _stnz3 = ezxml_add_child(_stnz2, "complete", 0);
      _stnz3 = ezxml_add_child(_stnz2, "next", 0);
      _stnz3 = ezxml_add_child(_stnz2, "prev", 0);
      _stnz3 = ezxml_add_child(_stnz2, "stop", 0);
      _stnz3 = ezxml_add_child(_stnz2, "pause", 0);
      _stnz3 = ezxml_add_child(_stnz2, "play", 0);

      // add xforms
      _stnz2 = ezxml_add_child(_stnz1, XML_TAG_X, 0);
      ezxml_set_attr(_stnz2, XML_TAG_XMLNS, "jabber:x:data");
      ezxml_set_attr(_stnz2, XML_TAG_TYPE, "form");

      _stnz3 = ezxml_add_child(_stnz2, XML_TAG_FIELD, 0);
      ezxml_set_attr(_stnz3, XML_TAG_VAR, "playing");
      ezxml_set_attr(_stnz3, XML_TAG_TYPE, "text-single");
      ezxml_set_attr(_stnz3, "label", "Now Playing");
      _stnz3 = ezxml_add_child(_stnz3, "value", 0);
    }

  return _stnz1;
}

/**
 *
 */
static struct xmlns_provider_item player_items[] =
  {
    { .name = "jingleplayer", .desc = "Jingle Player" },
    { 0, } };

/**
 *
 */
static int
player_on_cmd_stanza(struct x_bus *sess, ezxml_t __stnz)
{
  char *func = NULL;
  ezxml_t _stnz1 = NULL;

  ENTER;
  func = (char *) ezxml_attr(__stnz, "node");

  if (func && !strcmp("jingleplayer", func))
    {
      _stnz1 = player_do_cmd(sess, __stnz);
      /* create Error reply */
      if (!_stnz1)
        {
          _stnz1 = ezxml_new("error");
          ezxml_set_attr(_stnz1, "type", "modify");
          ezxml_set_attr_d(_stnz1,"code","400");
        }
      xmpp_session_reply_iq_stanza(sess,__stnz,_stnz1);
    }

  EXIT;
  return 0;
}

/**
 *
 */
static struct xmlns_provider player_cmd_provider =
  { .stanza_rx = player_on_cmd_stanza, .items = player_items };

static
void
player_init(void)
{
  dbm_add_xmlns_provider(XMLNS_COMMANDS, &player_cmd_provider);
  return;
}
PLUGIN_INIT(player_init)
;

#ifdef UNIT_TEST
/* unit test */
int main(int argc, char **argv)
  {
    struct sockaddr_in saddr;
    int sock;
    char buf[25];
    pthread_t t;
    ezxml_t s,_s;

    /* create and bind socket */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(48641);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind (sock,(struct sockaddr *)&saddr,sizeof(struct sockaddr_in)) < 0)
    return -1;

    s = ezxml_new("params");
    _s = ezxml_add_child(s,"target",0);
    ezxml_set_attr_d(_s,"ip","127.0.0.1");
    ezxml_set_attr_d(_s,"port","8010");
    sprintf(buf,"%d",sock);
    ezxml_set_attr_d(_s,"socket",buf);

    rtp_server((void *)s);

    //pthread_create (&t,NULL,rtp_server,(void *)s);
    // pthread_join(t,NULL);
    exit(0);
  }
#endif /* UNIT_TEST */

