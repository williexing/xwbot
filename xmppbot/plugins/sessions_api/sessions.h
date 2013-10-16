/*
 * sessions.h
 *
 *  Created on: Aug 15, 2011
 *      Author: artemka
 */

#ifndef SESSIONS_H_
#define SESSIONS_H_

#include <ev.h>

#include <xwlib/x_types.h>
#include <xwlib/x_obj.h>
#include <xwlib/x_utils.h>

#include <plugins/ice/x_stun.h>

#ifdef __cplusplus
extern "C"
  {
#endif

#define MAX_FDS_PER_SESSION 16

enum
{
  XSESS_IO_MSG = 0,
  XSESS_IO_IQ,
  XSESS_IO_PRESENCE,
  XSESS_IO_JINGLE,
  XSESS_IO_FILE
};

#define CHANNEL_STATE_TRANSPORT_READY 0x1
#define CHANNEL_STATE_MEDIA_READY 0x2
//#define CHANNEL_STATE_MEDIA_READY 0x2
#define CHANNEL_STATE_READY 0x3


/**
 * @ingroup SESSION_API
 *
 * read callback type
 */
typedef int
(*x_session_read_cb_f)(int iod, char *buf, size_t *siz, void *cbdata);
typedef int
(*x_session_write_cb_f)(int iod, char *buf, size_t siz, void *cbdata);

/**
 * @ingroup SESSION_API
 *
 * x_candidate_type:
 * @NICE_CANDIDATE_TYPE_HOST: A host candidate
 * @NICE_CANDIDATE_TYPE_SERVER_REFLEXIVE: A server reflexive candidate
 * @NICE_CANDIDATE_TYPE_PEER_REFLEXIVE: A peer reflexive candidate
 * @NICE_CANDIDATE_TYPE_RELAYED: A relay candidate
 *
 * An enum represneting the type of a candidate
 */
typedef enum
{
  NICE_CANDIDATE_TYPE_HOST,
  NICE_CANDIDATE_TYPE_SERVER_REFLEXIVE,
  NICE_CANDIDATE_TYPE_PEER_REFLEXIVE,
  NICE_CANDIDATE_TYPE_RELAYED,
} x_candidate_type;

/**
 * @ingroup SESSION_API
 *
 * x_candidate_transport_proto:
 * @NICE_CANDIDATE_TRANSPORT_UDP: UDP transport
 *
 * An enum representing the type of transport to use
 */
typedef enum
{
  NICE_CANDIDATE_TRANSPORT_UDP,
} x_candidate_transport_proto;

/**
 * @ingroup SESSION_API
 *
 * relay_type_t:
 * @NICE_RELAY_TYPE_TURN_UDP: A TURN relay using UDP
 * @NICE_RELAY_TYPE_TURN_TCP: A TURN relay using TCP
 * @NICE_RELAY_TYPE_TURN_TLS: A TURN relay using TLS over TCP
 *
 * An enum representing the type of relay to use
 */
typedef enum
{
  NICE_RELAY_TYPE_TURN_UDP,
  NICE_RELAY_TYPE_TURN_TCP,
  NICE_RELAY_TYPE_TURN_TLS,
} relay_type_t;

/* Maximum and default sizes for ICE attributes,
 * last updated from ICE ID-19
 * (the below sizes include the terminating NULL):
 */
#define NICE_STREAM_MAX_UFRAG   256 + 1  /* ufrag + NULL */
#define NICE_STREAM_MAX_UNAME   256 * 2 + 1 + 1 /* 2*ufrag + colon + NULL */
#define NICE_STREAM_MAX_PWD     256 + 1  /* pwd + NULL */
#define NICE_STREAM_DEF_UFRAG   4 + 1    /* ufrag + NULL */
#define NICE_STREAM_DEF_PWD     22 + 1   /* pwd + NULL */

#define NICE_CANDIDATE_MAX_FOUNDATION                32+1
#define NICE_CANDIDATE_PAIR_MAX_FOUNDATION        NICE_CANDIDATE_MAX_FOUNDATION*2

/**
 * @ingroup SESSION_API
 *
 * STUN_MAX_MESSAGE_SIZE:
 *
 * The Maximum size of a STUN message
 */
#define STUN_MAX_MESSAGE_SIZE 65552

/**
 * common_network_address_t:
 * @addr: Generic sockaddr address
 * @ip4: IPv4 sockaddr address
 * @ip6: IPv6 sockaddr address
 *
 * @ingroup SESSION_API
 *
 * The #common_network_address_t structure that represents an IPv4 or IPv6 address.
 */
typedef struct common_network_address
{
  union
  {
    struct sockaddr addr;
    struct sockaddr_in ip4;
    struct sockaddr_in6 ip6;
  } s;
} common_network_address_t;

/**
 * turn_server_info_t:
 * @server: The #common_network_address_t of the TURN server
 * @username: The TURN username
 * @password: The TURN password
 * @type: The #relay_type_t of the server
 *
 * @ingroup SESSION_API
 *
 * A structure to store the TURN relay settings
 */
typedef struct _turn_server_info
{
  common_network_address_t server; /**< TURN server address */
  char *username; /**< TURN username */
  char *password; /**< TURN password */
  relay_type_t type; /**< TURN type */
} turn_server_info_t;

/**
 * x_candidate:
 * @type The type of candidate
 * @transport The transport being used for the candidate
 * @addr The #common_network_address_t of the candidate
 * @base_addr The #common_network_address_t of the base address used by the candidate
 * @priority The priority of the candidate <emphasis> see note </emphasis>
 * @stream_id The ID of the stream to which belongs the candidate
 * @component_id The ID of the component to which belongs the candidate
 * @foundation The foundation of the candidate
 * @username The candidate-specific username to use (overrides the one set
 * by nice_agent_set_local_credentials() or nice_agent_set_remote_credentials())
 * @password The candidate-specific password to use (overrides the one set
 * by nice_agent_set_local_credentials() or nice_agent_set_remote_credentials())
 * @turn The #turn_server_info_t settings if the candidate is
 * of type %NICE_CANDIDATE_TYPE_RELAYED
 * @sockptr The underlying socket
 *
 * @ingroup SESSION_API
 *
 * A structure to represent an ICE candidate
 * The @priority is an integer as specified in the ICE draft 19. If you are
 * using the MSN or the GOOGLE compatibility mode (which are based on ICE
 * draft 6, which uses a floating point qvalue as priority), then the @priority
 * value will represent the qvalue multiplied by 1000.
 */
typedef struct xcandidate
{
  x_candidate_type type;
  x_candidate_transport_proto transport;
  common_network_address_t addr;
  common_network_address_t base_addr;
  uint32_t priority;
  uint32_t stream_id;
  uint32_t component_id;
  char foundation[NICE_CANDIDATE_MAX_FOUNDATION];
  char *username; /* pointer to a NULL-terminated username string */
  char *password; /* pointer to a NULL-terminated password string */
  turn_server_info_t *turn;
  void *sockptr;
} x_candidate;

typedef enum
{
  NICE_CHECK_WAITING = 1, /**< waiting to be scheduled */
  NICE_CHECK_IN_PROGRESS, /**< conn. checks started */
  NICE_CHECK_SUCCEEDED, /**< conn. succesfully checked */
  NICE_CHECK_FAILED, /**< no connectivity, retransmissions ceased */
  NICE_CHECK_FROZEN, /**< waiting to be scheduled to WAITING */
  NICE_CHECK_CANCELLED, /**< check cancelled */
  NICE_CHECK_DISCOVERED
/**< a valid candidate pair not on check list */
} candidate_connection_state;

/**
 * @ingroup SESSION_API
 *
 * x_candidate_pair_t:
 *
 */
struct x_candidate_pair
{
  uint32_t stream_id;
  uint32_t component_id;
  x_candidate *local;
  x_candidate *remote;
  char foundation[NICE_CANDIDATE_PAIR_MAX_FOUNDATION];
  candidate_connection_state state;
  uint32_t nominated;
  uint32_t controlling;
  uint32_t timer_restarted;
  uint64_t priority;
  /*  GTimeVal next_tick;  next tick timestamp */
  /*  StunTimer timer;*/
  uint8_t stun_buffer[STUN_MAX_MESSAGE_SIZE];
/*  StunMessage stun_message;*/
};

/**
 * I/O stream object class. aka 'channel'
 * @ingroup SESSION_API
 *
 */
#define MAX_CONN_PAIRS 256

#define ICE_STUN_RELAXATION_COUNT 16

#define ICE_FLAG_RTP_SUCCEEDED 0x4L

#define PAYLOAD_CACHE_SIZE 127
    
typedef struct xiostream
{
  x_object xobj;
  struct
  {
    int state;
    volatile uint32_t cr0;
  } regs;
  int sockv4;
  int sockv4_ctl;
  int sockv6;
  int sockv6_ctl;
  unsigned int data_port;
  unsigned int ctl_port;
  CS_DECL(_worker_lock);
  stun_hdr_t *request;
  char *otherpwd;
  char *otherufrag;
  char *ownpwd;
  char *ownufrag;
#ifndef __DISABLE_MULTITHREAD__
  /** input thread id */
  THREAD_T input_tid;
  xevent_dom_t *eventloop;
#endif
  struct ev_timer iowatcher_timer;
  unsigned int timer_counter;
  struct ev_io iowatcher_ipv4;
  struct ev_io iowatcher_ipv4_ctl;
  turn_server_info_t turn_srv;

  /* discovered network addresses */
  CS_DECL(_c_pairs_lock);

  struct x_candidate_pair *c_pairs[MAX_CONN_PAIRS];
  struct x_candidate_pair **c_pairs_top;
  struct x_candidate_pair *c_pair_last_success;

  x_candidate *locals[MAX_CONN_PAIRS];
  x_candidate **locals_top;

  x_candidate *remotes[MAX_CONN_PAIRS];
  x_candidate **remotes_top;
  /* discovered network addresses--- */

  /**
   * Media object / payload acceptor
   * It should route packets by their payload-type
   */
//  x_object *media_owner;
  x_object *media_owner_cache[PAYLOAD_CACHE_SIZE];

  /** Array of circular buffers */
//  struct x_circbuf circbuf[3];

} x_io_stream;

/**
 * @ingroup SESSION_API
 *
 * read callback type
 */
typedef struct xio
{
  int sysfd;
  x_object *owner;
  void *io_data;
  x_session_write_cb_f *write_up;
  x_session_write_cb_f *write_down;
  x_session_read_cb_f *read_up;
  x_session_read_cb_f *read_down;
} x_io;

enum
{
  SESSION_NEW = 0,
  SESSION_WAITING,
  SESSION_ACTIVE,
  SESSION_TIMED_OUT,
};

enum
{
  CHANNEL_MODE_IN = 0,
  CHANNEL_MODE_OUT,
  CHANNEL_MODE_BOTH,
};

typedef struct xcommonsession
{
  x_object xobj;
  int state;
  x_io ioset[MAX_FDS_PER_SESSION];
  int ionum;
  /*struct ev_loop *eventloop;*/
  struct ev_io watcher;
  struct ev_periodic qwartz;
  struct ev_timer expiration_timer;
  /** remote URI */
  char *uri;
  /** TODO Make document notes about this */
#ifndef __DISABLE_MULTITHREAD__
  THREAD_T tid;
#endif
} x_common_session;
/**
 * @}
 */

/**
 * @defgroup SESSION_API Sessions API
 *
 * @{
 */
#define MEDIA_CLASS_STR "$media"
#define MEDIA_IN_CLASS_STR "in$media"
#define MEDIA_OUT_CLASS_STR "out$media"
#define CAMERA_CLASS_STR "$camera"
#define DISPLAY_CLASS_STR "dataplayer"

#define MEDIA_IO_MODE_IN 0
#define MEDIA_IO_MODE_OUT 1

EXPORT_SYMBOL x_common_session *x_session_open(
    const char *jid_remote, x_object *context, x_obj_attr_t *hints, int flags);
EXPORT_SYMBOL int x_session_close(
    const char *jid_remote, x_object *context, x_obj_attr_t *hints, int flags);
EXPORT_SYMBOL x_object *x_session_channel_open(
    x_object *ctx, const char *rjid, const char *sid,const char *ch_name);
EXPORT_SYMBOL x_object *x_session_channel_open2(x_object *sesso, const char *ch_name);
EXPORT_SYMBOL void x_session_channel_destroy_str (const char *sid, const char *ch_name);
EXPORT_SYMBOL void x_session_channel_destroy_xobj(x_object *ch);
EXPORT_SYMBOL int x_session_channel_read(x_object *ch, char *buf, size_t siz);
EXPORT_SYMBOL int x_session_channel_write(x_object *ch, char *buf, size_t siz);
EXPORT_SYMBOL int x_session_channel_set_transport_profile_ns(x_object *cho, const x_string_t tname, const x_string_t ns, x_obj_attr_t *hints);
EXPORT_SYMBOL int x_session_channel_set_media_profile_ns(x_object *cho, const x_string_t mname, const x_string_t ns);
EXPORT_SYMBOL void x_session_channel_add_transport_candidate(x_object *cho, x_object *cand);
EXPORT_SYMBOL int x_session_add_payload_to_channel (
    x_object *cho, const x_string_t ptype,
    const x_string_t ptname, int io_mod, x_obj_attr_t *hints);
EXPORT_SYMBOL int
x_session_channel_map_flow_twin(
        x_object *cho, const x_string_t src,
        const x_string_t dst);
EXPORT_SYMBOL int x_session_set_channel_io_mode(x_object *cho, int mode);
EXPORT_SYMBOL void x_session_start(x_object *sess);
/**
 * @}
 */

/**
 * Convert JID string to SID string and place it
 * to buffer allocated in stack with alloca()
 *
 * @ingroup SESSION_API
 */
#define JID_TO_SID_A(sidbuf,jid) \
  sidbuf = alloca(strlen("msg_") + strlen(jid) + 1); \
  sprintf(sidbuf, "msg_%s", jid); \
  str_swap_chars(sidbuf, '/', '_');

/**
 * Convert JID string to SID string and place it
 * to buffer allocated in heap with malloc()
 *
 * @ingroup SESSION_API
 */
#define JID_TO_SID_M(sidbuf,jid) \
  sidbuf = malloc(strlen("msg_") + strlen(jid) + 1); \
  sprintf(sidbuf, "msg_%s", jid); \
  str_swap_chars(sidbuf, '/', '_');

#ifdef __cplusplus
  }
#endif

#endif /* SESSIONS_H_ */
