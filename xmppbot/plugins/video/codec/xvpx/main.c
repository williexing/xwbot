/*
 * gobee_translator.cpp
 *
 *  Created on: Sep 3, 2011
 *      Author: artemka
 */

#undef DEBUG_PRFX
#define DEBUG_PRFX "[ xvpx_main ] "
#include <xwlib/x_types.h>
#include <xwlib/x_obj.h>
#include <xwlib/x_utils.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "mynet.h"
#include "x_vp8.h"

static int displayW = VIDEO_WIDTH;
static int displayH = VIDEO_HEIGHT;
static unsigned short remotePort = REMOTE_PORT;
static unsigned short localPort = LOCAL_PORT;
static char *remoteIp = REMOTE_IP;
static char *localIp = LOCAL_IP;

int qvfb_protocol = 0;

#if 1
typedef enum
{
  UI_EVT_NOTHING = -1,
  UI_EVT_DELETE = 0,
  UI_EVT_DESTROY = 1,
  UI_EVT_EXPOSE = 2,
  UI_EVT_MOTION_NOTIFY = 3,
  UI_EVT_BUTTON_PRESS = 4,
  UI_EVT_2BUTTON_PRESS = 5,
  UI_EVT_3BUTTON_PRESS = 6,
  UI_EVT_BUTTON_RELEASE = 7,
  UI_EVT_KEY_PRESS = 8,
  UI_EVT_KEY_RELEASE = 9,
  UI_EVT_ENTER_NOTIFY = 10,
  UI_EVT_LEAVE_NOTIFY = 11,
  UI_EVT_FOCUS_CHANGE = 12,
  UI_EVT_CONFIGURE = 13,
  UI_EVT_MAP = 14,
  UI_EVT_UNMAP = 15,
  UI_EVT_PROPERTY_NOTIFY = 16,
  UI_EVT_SELECTION_CLEAR = 17,
  UI_EVT_SELECTION_REQUEST = 18,
  UI_EVT_SELECTION_NOTIFY = 19,
  UI_EVT_PROXIMITY_IN = 20,
  UI_EVT_PROXIMITY_OUT = 21,
  UI_EVT_DRAG_ENTER = 22,
  UI_EVT_DRAG_LEAVE = 23,
  UI_EVT_DRAG_MOTION = 24,
  UI_EVT_DRAG_STATUS = 25,
  UI_EVT_DROP_START = 26,
  UI_EVT_DROP_FINISHED = 27,
  UI_EVT_CLIENT_EVENT = 28,
  UI_EVT_VISIBILITY_NOTIFY = 29,
  UI_EVT_NO_EXPOSE = 30,
  UI_EVT_SCROLL = 31,
  UI_EVT_WINDOW_STATE = 32,
  UI_EVT_SETTING = 33,
  UI_EVT_OWNER_CHANGE = 34,
  UI_EVT_GRAB_BROKEN = 35,
  UI_EVT_DAMAGE = 36,
  UI_EVT_EVENT_LAST
/* helper variable for decls */
} MouseEventType;
#endif

static char keymap[256] =
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'a', 'b', 'c', 'd', 'e',
      'f', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0 /* 101 */, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0 /* 201 */, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, };

static void
my_udp_callback(void *data, int len, void *cb_data);

#ifdef WIN32
static void *
#else
static void *
#endif
    listener_thread(void *dat);

static int
_x_tst_net_connect(const char *host, int port)
{
  int sock;
  struct sockaddr_in saddr;
  struct sockaddr_in myaddr;

  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0)
    {
      printf("Sock = %d\n", sock);
      perror("\nSOCK: ");
      return -1;
    }

  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = inet_addr(host);
  saddr.sin_port = htons(port);

  //  if (connect(sock, (struct sockaddr *) &saddr, sizeof(saddr)))
  // return -1;

  memset(&myaddr, 0, sizeof(saddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = INADDR_ANY;
  myaddr.sin_port = htons(localPort);
  bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr));

  printf("Binded on %d\n", ntohs(myaddr.sin_port));
  return sock;
}

void
run_vp8(void)
{
  int w = displayW;
  int h = displayH;
  struct sockaddr_in cli;
  struct sockaddr_in myaddr;
  int sock;

  /****************************/
  memset(&cli, 0, sizeof(cli));
  cli.sin_family = AF_INET;
  cli.sin_addr.s_addr = inet_addr(remoteIp);
  cli.sin_port = htons(remotePort);

  memset(&myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = INADDR_ANY;
  myaddr.sin_port = htons(remotePort);
  /****************************/

  printf("Connecting to %s at %d... ", remoteIp, remotePort);
  sock = _x_tst_net_connect(remoteIp, remotePort);
  if (sock >= 0)
    printf("OK\n");
  else
    printf("ERROR\n");

  x_vpx_enc_run(NULL, w, h, sock, &cli);
}

#if 0
unsigned char
clamp(int d)
  {
    if (d < 0)
    return 0;

    if (d > 255)
    return 255;

    return (unsigned char) d;
  }

void
rgb2yuv(unsigned char *rgb, th_ycbcr_buffer *ycbcr, int w, int h)
  {
    int x;
    int y;
    int r;
    int g;
    int b;

    union
      {
        unsigned int raw;
        unsigned char rgba[4];
      }color;

    for (y = 0; y < h; y++)
      {
        for (x = 0; x < w; x++)
          {
            r = clamp(rgb[(y * w + x) * 3 + 0]);
            g = clamp(rgb[(y * w + x) * 3 + 1]);
            b = clamp(rgb[(y * w + x) * 3 + 2]);

            (*ycbcr)[0].data[y * w + x] = clamp(
                ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);
            (*ycbcr)[1].data[(x >> 1) + (y >> 1) * ((*ycbcr)[1].stride)] = clamp(
                ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128);
            (*ycbcr)[2].data[(x >> 1) + (y >> 1) * ((*ycbcr)[2].stride)] = clamp(
                ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);

          }
      }
  }
#endif

#ifdef WIN32
static char __lip[128];
static char __rip[128];

static void parser_cmd (char *argv)
  {
    size_t siz;
    //	char key[128];
    //	char val[128];
    char *key;
    char *val;

    printf("parsing '%s'... ", argv);

    //	sscanf_s(argv,(const char *)"%[^:]:%[^.]",&key[0],&val[0]);
    key = ++argv;
    val = x_strchr(argv,':');
    *val++ = '\0';

    printf("Ok '%s:%s'\n", key,val);

    if(!_stricmp(_T("w"),key))
      {
        displayW = atoi(&val[0]);
      }
    else
    if(!_stricmp(_T("h"),key))
      {
        displayH = atoi(&val[0]);
      }
    else
    if(!_stricmp(_T("lport"),key))
      {
        localPort = atoi(&val[0]);
      }
    else
    if(!_stricmp(_T("rport"),key))
      {
        remotePort = atoi(&val[0]);
      }
    else
    if(!_stricmp(_T("lip"),key))
      {
        localIp = val;
      }
    else
    if(!_stricmp(_T("rip"),key))
      {
        remoteIp = val;
      }
  }

static void
parse_cmd_line_params(int argc, _TCHAR *argv[])
  {
    int c;
    for (c=1;c<argc;c++)
      {
        parser_cmd((char *)argv[c]);
      }
    return;
  }
#else
static void
parse_cmd_line_params(int argc, char *argv[])
{
  char c;
  for (;;)
    {
      int this_option_optind = optind ? optind : 1;
      int option_index = 0;
      static struct option long_options[] =
        {
          { "width", 1, 0, 'w' },
          { "height", 1, 0, 'h' },
          { "remote-ip", 1, 0, 0 },
          { "local-ip", 1, 0, 0 },
          { "remote-port", 1, 0, 0 },
          { "local-port", 1, 0, 0 },
          { 0, 0, 0, 0 } };

      c = getopt_long(argc, argv, "wh:", long_options, &option_index);
      if (c == -1)
        break;

      switch (c)
        {
      case 0:
        switch (option_index)
          {
        case 0:
          displayW = atoi(optarg);
          break;

        case 1:
          displayH = atoi(optarg);
          break;

        case 2:
          remoteIp = optarg;
          break;

        case 3:
          localIp = optarg;
          break;

        case 4:
          remotePort = atoi(optarg);
          break;

        case 5:
          localPort = atoi(optarg);
          break;
          }
        break;

      case 'w':
        displayW = atoi(optarg);
        break;

      case 'h':
        displayH = atoi(optarg);
        break;

        }
    }
}
#endif

#ifdef WIN32
int _tmain(int argc, _TCHAR *argv[])
#else
int
main(int argc, char *argv[])
#endif
{
  int dis = 0;
  int infd;

  parse_cmd_line_params(argc, argv);

  printf("\n width: %d"
    "\n height: %d"
    "\n remote ip: %s"
    "\n remote port: %d"
    "\n local ip: %s"
    "\n local port: %d"
    "\n", displayW, displayH, remoteIp, remotePort, localIp, localPort);

    {
#ifdef WIN32
      WORD wVersionRequested;
      WSADATA wsaData;

      wVersionRequested = MAKEWORD( 2, 2 );
      WSAStartup( wVersionRequested, &wsaData );
      _beginthread(listener_thread, 0, (void *) NULL);
#else
      pthread_t tid, ttid;
      pthread_create(&tid, NULL, listener_thread, (void *) NULL);
#endif
    }

  run_vp8();
}

static void
my_udp_callback(void *data, int len, void *cb_data)
{
  int x, y;
  pointer_data_t *ptd;
  rtp_hdr_t *rtp = (rtp_hdr_t *) data;

  if (rtp && (rtp->octet2.control2.pt & 0x7f) == WIN_EVENT_PTYPE)
    {
      ptd = (pointer_data_t *) &rtp->payload[0];
      x = ntohl(ptd->x);
      y = ntohl(ptd->y);
      printf("gdk event of type %d at [%dx%d]\n", ntohl(ptd->control), x, y);
      if (cb_data)
        {
          switch (ntohl(ptd->control))
            {
          case UI_EVT_MOTION_NOTIFY:
            break;
          case UI_EVT_BUTTON_PRESS:
            //            myGb->mousebuttons |= LeftButton;
            break;
          case UI_EVT_BUTTON_RELEASE:
            //            myGb->mousebuttons &= ~LeftButton;
            break;

          case UI_EVT_KEY_PRESS:
            if (y)
              {
                // myGb->keymodifiers |= x;
              }
            else
              {
                printf("keypress event %d at [%cx%d]\n", ntohl(ptd->control),
                    x, y);
                switch (x)
                  {
                case 13:
                  break;
                default:
                  break;
                  };
              }
            break;
          case UI_EVT_KEY_RELEASE:
            // myGb->keycode = x;
            if (y)
              {
                // myGb->keymodifiers &= ~x;
              }
            break;
            }

        }
    }
}

#ifdef WIN32
static void *
#else
static void *
#endif
listener_thread(void *dat)
{
  printf("Started listener thread\n");
  udp_listen(localIp, localPort, my_udp_callback, dat);
}
