/*
 * x_jingle.h
 *
 *  Created on: 17.11.2010
 *      Author: artur
 */

#ifndef X_JINGLE_H_
#define X_JINGLE_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <pthread.h>

#include <ezxml.h>
#include <xmppagent.h>

#include <JingleTransport.h>

#ifdef __cplusplus
extern "C" {
#endif

/* flags_r0 */
#define J_FLAG0_TRANSPORT_OK (1<<0)
#define J_FLAG0_CONTENT_OK (1<<1)
#define J_FLAG0_VIDEO_OK (1<<2)
#define J_FLAG0_AUDIO_OK (1<<3)
#define J_FLAG0_WANT_ACCEPT (1<<4)
#define J_FLAG0_ACCEPT_OK (1<<5)
#define J_FLAG0_BIND_OK (1<<6)

/* flags_r1 */
#define J_FLAG1_IPV6_OK (1<<0)
#define J_FLAG1_IPV4_OK (1<<1)

enum
{
  cmd_nil = 0,
  session_initiate,
  session_accept,
  session_terminate,
  transport_info,
  transport_accept,
  content_accept,
};

/**
 *
 */
struct JingleSession;
struct JingleChannel;
typedef struct JingleChannel TJingleChannel;
typedef TJingleChannel *PJingleChannel;

/* channel states */
#define J_CHANNEL_PAYLOAD_OK (1<<8)
#define J_CHANNEL_TRANSPORT_LOCAL (1<<9)
#define J_CHANNEL_TRANSPORT_REMOTE (1<<10)
#define J_CHANNEL_WANT_CONNECT (1<<16)
#define J_CHANNEL_CONNECTED (1<<17)
#define J_CHANNEL_DISCONNECTED (1<<18)
#define J_CHANNEL_TIMED_OUT (1<<19)

/**
 * @brief Jingle logical data channel.
 *
 * This describes and holds data of logical data channel.
 * This is the pair of payload and transport objects.
 *
 */
struct JingleChannel
{
  /* I/O queue */
  queue_t *outQueue;
  queue_t *inQueue;
  /* attributes */
  int reg0;
  ezxml_t xml;
};

void *JingleChannel_new(struct XmppBus *bus);


struct JingleSession;


/**
 *
 */
typedef struct JingleSession
{
  /* jingle state machine registers */
  uint32_t flags_r0;
  uint32_t flags_r1;
  uint32_t control_r0;
  /* last command register */
  uint32_t cmd_r;
  /* last incoming message id register */
  char *last_msg_id_r;
} TJingleSession;

typedef TJingleSession *PJingleSession;


struct JingleSession *JingleSession_new(struct XmppBus *bus, const char *name);
void JingleSession_delete(struct JingleSession *);

#ifdef __cplusplus
}
#endif

#endif /* X_JINGLE_H_ */
