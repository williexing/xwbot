/*
 * filetransfer.c
 *
 *  Created on: Aug 11, 2010
 *      Author: artur
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#include <stdlib.h>
#include <stdio.h>

#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <pthread.h>
#include <sched.h>

#include <ezxml.h>
#include <xmppagent.h>
#include <crypt/sha1_rfc3174.h>

extern void *
sock5srv(void *arg);

static __inline__ void
_bs_wake_up_jobs(struct x_bus *sess)
{
  pthread_mutex_lock(&sess->jobs_lock);
  pthread_cond_broadcast(&sess->jobs_cond);
  pthread_mutex_unlock(&sess->jobs_lock);
}

/**
 * TODO
 * Checks unfinished bytestream jobs
 */
static
int
xmpp_bs_complete_job(struct x_bus *sess, ezxml_t job)
{
  int err;
  char *fnam;
  char buf[256];

  int dst_fd;
  int src_fd;
  int status = -1;

  pthread_mutex_lock(&sess->lock);
  err = sscanf(ezxml_attr(job, XML_TAG_STATUS), "%d", &status);
  err = sscanf(ezxml_attr(job, "dst_fd"), "%d", &dst_fd);
  err = sscanf(ezxml_attr(job, "src_fd"), "%d", &src_fd);
  pthread_mutex_unlock(&sess->lock);

  if (status == 1) /* connected state */
    {
      pthread_mutex_lock(&sess->lock);
      fnam = (char *) ezxml_attr(job, "name");
      if (fnam != NULL)
        {
          fnam = malloc(strlen(ICONBIT_UPLOAD_DIR) + strlen(fnam) + 2);
          sprintf(fnam, ICONBIT_UPLOAD_DIR"/%s", ezxml_attr(job, "name"));
          dst_fd = open(fnam, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR
              | S_IRGRP);

          sprintf(buf, "%d", dst_fd);
          ezxml_set_attr_d(job,"dst_fd",buf);
          ezxml_set_attr(job, "status", "2"); /* file opened */

          status = 2;
          free(fnam);
        }
      pthread_mutex_unlock(&sess->lock);
    }

  if (status == 2)
    {
      if ((err = read(src_fd, buf, sizeof(buf))) > 0)
        {
          err = write(dst_fd, buf, err);
          return 0;
        }
      /* completed job */
      else
        {
          close(src_fd);
          close(dst_fd);
          return 1;
        }
    };

  return -1;
}

/**
 *
 */
void *
xmpp_bs_complete(void *arg)
{
  struct x_bus *sess;
  int err;
  ezxml_t j, _j;

  if (!arg)
    return NULL;

  sess = (struct x_bus *) arg;
  for (;;)
    {
      pthread_mutex_lock(&sess->lock);
      j = ezxml_get(sess->dbcore, "bytestreams", 0, "running", 0, "job", -1);
      pthread_mutex_unlock(&sess->lock);

      if (j)
        {
          while (j)
            {
              err = xmpp_bs_complete_job(sess, j);
              pthread_mutex_lock(&sess->lock);
              _j = ezxml_next(j);
              if (err != 0)
                ezxml_remove(j);
              pthread_mutex_unlock(&sess->lock);
              j = _j;
            }
        }
      else
        {
          pthread_mutex_lock(&sess->jobs_lock);
          pthread_cond_wait(&sess->jobs_cond, &sess->jobs_lock);
          pthread_mutex_unlock(&sess->jobs_lock);
        }
    }

  return NULL;
}

/**
 * Connect to bytestream
 */
int
xmpp_socks5_connect(const char *host, const char *port, const char *dgst)
{
  char buf[128];
  int sock;
  int err;

  struct addrinfo hints;
  struct addrinfo *res;

  ENTER;
  printf("Connecting to %s on %s\n", (char *) host, (char *) port);

  memset(&hints, 0, sizeof(struct addrinfo));
  /* set-up hints structure */
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  err = getaddrinfo(host, port, &hints, &res);
  if (err)
    {
      gai_strerror(err);
      return -1;
    }

  if (res->ai_addr == NULL)
    {
      gai_strerror(err);
      return -1;
    }

  switch (res->ai_addr->sa_family)
    {
  case AF_INET:
    printf("IPv4 Address\n");
    break;
  case AF_INET6:
    printf("IPv6 Address\n");
    break;
  default:
    printf("UNKNOWN Address family\n");
    break;
    }

  sock = socket(AF_INET, SOCK_STREAM, 0);

  if (connect(sock, res->ai_addr, res->ai_addrlen) != 0)
    {
      perror("Connect()");
      close(sock);
      return -1;
    }
#if 1
  printf("Ok! Connected!\n");

  buf[0] = 0x05;
  buf[1] = 0x01;
  buf[2] = 0x00;

  err = send(sock, buf, 3, 0);
#ifdef TRACE_DEBUG
  printf("Sent %d bytes to server\n", err);
#endif

  err = recv(sock, buf, 128, 0);
#ifdef TRACE_DEBUG
  printf("Received %d bytes (0x%x 0x%x) from server\n", err, buf[0], buf[1]);
#endif
  if (buf[1])
    {
      return -1;
    }
#endif
  // send cmd
  buf[0] = 0x05;
  buf[1] = 0x01;
  buf[2] = 0x00;
  buf[3] = 0x03;
  buf[4] = (char) 40;

  memcpy(buf + 5, dgst, 40);

  err = send(sock, buf, 47, 0);
#ifdef TRACE_DEBUG
  printf("Sent %d bytes to server\n", err);
#endif

  err = recv(sock, buf, 128, 0);
#ifdef TRACE_DEBUG
  printf("Received %d bytes from server\n", err);
#endif
  if (err > 0 && buf[1])
    {
      close(sock);
      return -1;
    }

  printf("SOCKS5 replied new hostname = '%s'\n", buf + 5);

  return sock;
}

/**
 * Accepts bytestream
 */
static int
on_bytestream_stanza(struct x_bus *sess, ezxml_t __stnz)
{
  int err;
  int src_fd;
  char buf[25];
  char *str;
  char *host;
  char *port;
  char *sid;
  char *jid;
  char *myjid;
  char *shjid; // stream host JID
  int fsock;
  ezxml_t _stnz;
  ezxml_t _stnz1;
  ezxml_t _stnz2;
  ezxml_t root, _s, _s1;
  ezxml_t stream_job = NULL;
  char tmpbuf[10];

  ENTER;

  pthread_mutex_lock(&sess->lock);
  myjid = x_strdup(ezxml_attr(sess->dbcore, "jid"));
  pthread_mutex_unlock(&sess->lock);

  /* seems to be a proxy reply or final streamhost-used */
  root = xmpp_stanza_get_root(__stnz);
  if (!strcmp(ezxml_attr(root, "type"), "result"))
    {
      _s = ezxml_get(sess->dbcore, "bytestreams", 0, "pending", -1);
      if (_s)
        _s = ezxml_child(_s, "job");

      if ((_stnz1 = ezxml_child(__stnz, "streamhost"))) /* proxy reply */
        {
          pthread_mutex_lock(&sess->lock);
          _stnz1 = ezxml_cut(_stnz1);
          _stnz1 = ezxml_insert(_stnz1, _s, 0);
          pthread_mutex_unlock(&sess->lock);

          /* send iq for job */
          _stnz = ezxml_new("iq");
          ezxml_set_attr(_stnz, "id", "67567546");
          ezxml_set_attr(_stnz, "type", "set");
          ezxml_set_attr(_stnz, "from", myjid);
          ezxml_set_attr_d(_stnz,"to",ezxml_attr(_s,"jid"));

          // FIXME Correct random values
          _s1 = ezxml_add_child(_stnz, "query", 0);
          ezxml_set_attr(_s1, "xmlns", XMLNS_BYTESTREAMS);
          ezxml_set_attr(_s1, "mode", "tcp");

          ezxml_set_attr_d(_s1,"sid",ezxml_attr(_s,"id"));

          _s1 = ezxml_add_child(_s1, "streamhost", 0);
          ezxml_set_attr_d(_s1,"jid",ezxml_attr(_stnz1,"jid"));
          ezxml_set_attr_d(_s1,"host",ezxml_attr(_stnz1,"host"));
          ezxml_set_attr_d(_s1,"port",ezxml_attr(_stnz1,"port"));

          xmpp_session_send_stanza(sess, _stnz);
          ezxml_free(_stnz);

          err = 0;
          goto _clean_exit1;
        }
      else if ((_stnz1 = ezxml_child(__stnz, "streamhost-used"))) /* final target reply */
        {
          ENTER;

          jid = (char *) ezxml_attr(root, "from");
          sid = x_strdup((char *) ezxml_attr(_s, "id"));
          _s1 = ezxml_child(_s, "streamhost");
          host = (char *) ezxml_attr(_s1, "host");
          port = (char *) ezxml_attr(_s1, "port");
          shjid = (char *) ezxml_attr(_s1, "jid");

          printf("CONNECTING TO SOCKS5 PROXY %s:%s\n", host, port);

          // open socks5 connection
          sid = realloc(sid, strlen(sid) + strlen(myjid) + strlen(jid));
          strcat(strcat(sid, myjid), jid);
          str = sha1_get_sha1(sid, (unsigned int) strlen(sid));
          fsock = xmpp_socks5_connect(host, port, str);
          if (str)
            free(str); // clear hash
          if (sid)
            free(sid); // clear entire string

          /*
           * Change jobs status and schedule
           */
          pthread_mutex_lock(&sess->lock);
          src_fd = open(ezxml_attr(_s, "name"), O_RDONLY, 0);
          sprintf(buf, "%d", src_fd);
          ezxml_set_attr_d(_s,"src_fd",buf);
          sprintf(buf, "%d", fsock);
          ezxml_set_attr_d(_s,"dst_fd",buf);
          ezxml_set_attr(_s, "status", "2"); /* file opened */

          _s = ezxml_cut(_s);

          _stnz = ezxml_get(sess->dbcore, "bytestreams", 0, "running", -1);
          ezxml_insert(_s, _stnz, 0);
          pthread_mutex_unlock(&sess->lock);

          /* activate SOCKS5 session */
          _stnz = xmpp_iq_new(shjid, myjid, "set");

          // FIXME Correct random values
          _s1 = ezxml_add_child(_stnz, XML_TAG_QUERY, 0);
          ezxml_set_attr(_s1, XML_TAG_XMLNS, XMLNS_BYTESTREAMS);
          ezxml_set_attr_d(_s1,XML_TAG_SID,ezxml_attr(_s,XML_TAG_ID));

          _s1 = ezxml_add_child(_s1, "activate", 0);
          ezxml_set_txt_d(_s1,ezxml_attr(_s,XML_TAG_JID));

          xmpp_session_send_stanza(sess, _stnz);

          /* notify jobs threads */
          _bs_wake_up_jobs(sess);

          err = 0;
          goto _clean_exit1;
        }
      else
        /* FIXME invalid state */
        err = -1;
      goto _clean_exit1;
    }

  /*
   * first take a look at pending job
   *
   * Note: at this stage we must have a pending job
   */
  _stnz2 = ezxml_get(sess->dbcore, "bytestreams", 0, "pending", -1);
  if (!_stnz2)
    goto ftexception;

  stream_job = ezxml_child(_stnz2, "job");
  if (!stream_job)
    goto ftexception;

  // first will look for proxy item
  for (_stnz1 = ezxml_child(__stnz, "streamhost"); _stnz1; _stnz1
      = ezxml_next(_stnz1))
    {
      if ((_stnz2 = ezxml_child(_stnz1, "proxy")))
        break;
    }

  // if no proxy then use first item
  if (!_stnz2)
    if (!(_stnz1 = ezxml_child(__stnz, "streamhost")))
      goto ftexception;

  jid = (char *) ezxml_attr(root, "from");
  sid = x_strdup((char *) ezxml_attr(__stnz, "sid"));
  host = (char *) ezxml_attr(_stnz1, "host");
  port = (char *) ezxml_attr(_stnz1, "port");
  shjid = (char *) ezxml_attr(_stnz1, "jid");

  // open socks5 connection
  sid = realloc(sid, strlen(sid) + strlen(myjid) + strlen(jid));
  strcat(strcat(sid, jid), myjid);

  str = sha1_get_sha1(sid, (unsigned int) strlen(sid));
  printf("SHA1 DIGEST = '%s'\n", str);
  fsock = xmpp_socks5_connect(host, port, str);
  if (str)
    free(str); // clear hash
  if (sid)
    free(sid); // clear entire string

  if (fsock > 0 && stream_job != NULL)
    {

      // create si response
      _stnz1 = ezxml_new("query");
      ezxml_set_attr(_stnz1, "xmlns", XMLNS_BYTESTREAMS);

      _stnz2 = ezxml_add_child(_stnz1, "streamhost-used", 0);
      ezxml_set_attr_d(_stnz2,"jid",shjid);

      // update bytestream jobs
      sprintf(tmpbuf, "%d", fsock);
      ezxml_set_attr_d(stream_job,"src_fd",tmpbuf);
      ezxml_set_attr(stream_job, "status", "1");

      pthread_mutex_lock(&sess->lock);
      // move pending job to running queue
      // create new file transfer job
      _stnz2 = ezxml_get(sess->dbcore, "bytestreams", 0, "running", -1);
      if (!_stnz2)
        {
          _stnz2 = ezxml_child(sess->dbcore, "bytestreams");
          _stnz2 = ezxml_add_child(_stnz2, "running", 0);
        }

      stream_job = ezxml_cut(stream_job);
      ezxml_insert(stream_job, _stnz2, 0);
      pthread_mutex_unlock(&sess->lock);
      xmpp_session_reply_iq_stanza(sess,__stnz,_stnz1);
      /* notify jobs threads */
      _bs_wake_up_jobs(sess);

    }
  else
    {
      /* cancel job */
      goto ftexception;
    }

  err = 0;
  goto _clean_exit1;

  ftexception: if (stream_job)
    ezxml_remove(stream_job);
  stream_job = NULL;

  // create si response
  _stnz1 = ezxml_new("error");
  ezxml_set_attr(_stnz1, "code", "400");
  ezxml_set_attr_d(_stnz1,"type","cancel");

  _stnz2 = ezxml_add_child(_stnz1, "bad-request", 0);
  ezxml_set_attr(_stnz2, "xmlns", XMLNS_XMPP_STANZAS);

  xmpp_session_reply_iq_stanza(sess,__stnz,_stnz1);
  err = -1;

  _clean_exit1: if (myjid)
    free(myjid);
  EXIT;
  return err;
}

static struct xmlns_provider bs_provider =
  { .stanza_rx = on_bytestream_stanza, };

void
bytestream_proto_init(void)
{
  dbm_add_xmlns_provider(XMLNS_BYTESTREAMS, &bs_provider);
  return;
}
PLUGIN_INIT(bytestream_proto_init)
;

