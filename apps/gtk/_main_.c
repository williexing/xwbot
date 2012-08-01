/*
 * main.c
 *
 *  Created on: Jul 7, 2011
 *      Author: artemka
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkpixbuf.h>
#include <glade/glade.h>

// #include <xw/x_obj.h>
#include "mynet.h"

#include "config.h"

#ifndef SHARE_DIR
#define SHARE_DIR ""
#endif

// #include "x_queue.h"

// #define USE_THEORA
#define USE_VPX

#if 1
#include "x_theora.h"
#elif defined(USE_VPX)
#include "x_vp8.h"
#endif

static int displayW = VIDEO_WIDTH;
static int displayH = VIDEO_HEIGHT;
static unsigned short remotePort = REMOTE_PORT;
static unsigned short localPort = LOCAL_PORT;
static char *remoteIp = REMOTE_IP;
static char *localIp = LOCAL_IP;

static GladeXML *xml;
static GtkWidget *drwble;

struct sockaddr_in cli;
static int sock = -1;
static int sock2 = -1;

/*
 EXPORT_SYMBOL x_objectclass *
 x_obj_get_class(const char *name, const char *ns, x_obj_attr_t *attrs)
 {
 return NULL;
 }
 */

typedef void
(udp_callback_fn_t)(void *data, int len, void *cb_data);

int
udp_listen(const char *host, int port, udp_callback_fn_t *cb_func,
    void *cb_data);


static void *
rtpthread(void *dat)
{
  drwble = glade_xml_get_widget(xml, "mainwin");
  if (!drwble)
    exit(-2);

#if 1
  udp_listen(localIp, localPort, on_theora_packet_received, dat);
#elif defined(USE_VPX)
  udp_listen(localIp, localPort, on_vp8_packet_received, dat);
#endif
  return 0;
}

extern int
xgl_render_scene(GtkWidget *widget, GdkEvent *event, void *data);
extern int
xgl_configure(GtkWidget *widget, GdkEvent *event, void *data);
extern int
xgl_start(GtkWidget *widget, void *data);

static void
__invalidate(GtkWidget *widget, GdkEvent *arg1, gpointer user_data)
{
  gdk_window_invalidate_rect(widget->window, &widget->allocation, FALSE);
  gdk_window_process_updates(widget->window, TRUE);
}

static gboolean
on_mainwin_frame_event(GtkWindow *window, GdkEvent *arg1, gpointer user_data)
{
  int res = 0;
  static pointer_data_t pdat;
  static pointer_seqno;

  switch (arg1->type)
    {
  case GDK_MOTION_NOTIFY:
    pdat.control = htonl(GDK_MOTION_NOTIFY);
    pdat.x = htonl(((GdkEventMotion *) arg1)->x);
    pdat.y = htonl(((GdkEventMotion *) arg1)->y);
    // __invalidate(GTK_WIDGET(window), arg1, user_data);
    res = 1;
    break;
  case GDK_BUTTON_PRESS:
    pdat.control = htonl(GDK_BUTTON_PRESS);
    pdat.x = htonl(((GdkEventButton *) arg1)->x);
    pdat.y = htonl(((GdkEventButton *) arg1)->y);
    res = 1;
    break;
  case GDK_BUTTON_RELEASE:
    pdat.control = htonl(GDK_BUTTON_RELEASE);
    pdat.x = htonl(((GdkEventButton *) arg1)->x);
    pdat.y = htonl(((GdkEventButton *) arg1)->y);
    res = 1;
    break;
  case GDK_KEY_PRESS:
    pdat.control = htonl(GDK_KEY_PRESS);
    pdat.x = htonl(((GdkEventKey *) arg1)->keyval);
    pdat.y = htonl(((GdkEventKey *) arg1)->is_modifier);
    res = 1;
    break;
  case GDK_KEY_RELEASE:
    pdat.control = htonl(GDK_KEY_RELEASE);
    pdat.x = htonl(((GdkEventKey *) arg1)->keyval);
    pdat.y = htonl(((GdkEventKey *) arg1)->is_modifier);
    res = 1;
    break;
  case GDK_EXPOSE:
    xgl_render_scene(GTK_WIDGET(window), arg1, user_data);
    res = 1;
    break;
  case GDK_MAP:
    printf("GDK_MAP !!!!!!! %s:%d\n", __FUNCTION__, __LINE__);
    xgl_start(GTK_WIDGET(window), user_data);
    res = 1;
    break;
  case GDK_CONFIGURE:
    xgl_configure(GTK_WIDGET(window), arg1, user_data);
    res = 1;
    break;
  default:
    break;
    }

  if (res)
    {
      //  printf("Sending %d event at [%dx%d]\n", arg1->type, ntohl(pdat.x), ntohl(pdat.y));
      if (sock < 0)
        {
          printf("connecting to socket\n");
          sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
          if (sock < 0)
            return -1;

          memset(&cli, 0, sizeof(cli));
          cli.sin_family = AF_INET;
          cli.sin_addr.s_addr = inet_addr(remoteIp);
          cli.sin_port = htons(remotePort);

          if (connect(sock, (struct sockaddr *) &cli, sizeof(cli)))
            return -1;
        }
//      rtp_send(sock, &pdat, sizeof(pdat), (struct sockaddr *) &cli, sizeof(cli),
//          0, POINTER_PTYPE, pointer_seqno++);
    }

  return 1;
}

#include <stdlib.h>
#include <getopt.h>

#include <ev.h>

struct ev_periodic cam_qwartz;
struct ev_periodic xgl_qwartz;
struct ev_io io_watcher;
struct ev_loop *io_loop;

struct capture_data
{
  int vfd;
  unsigned char *framebuf;
  int w;
  int h;
};

static void
__capture_periodic_cb(struct ev_loop *loop, struct ev_periodic *w, int revents)
{
  static int granule;
  struct capture_data *cdata = w->data;

  if (!(granule++ % 200))
    {
      gobee_theora_resize(cdata->w, cdata->h, cdata->w, sock,
          (struct sockaddr *) &cli, sizeof(cli));
    }
  if (v4l2_next_frame(cdata->vfd) == 0)
    {
      gobee_theora_push_frame(cdata->framebuf, sock, (struct sockaddr *) &cli,
          sizeof(cli));
    }
}

static void
__session_periodic_cb(struct ev_loop *loop, struct ev_periodic *w, int revents)
{
  GtkWidget *wgt = w->data;
  printf("%s:%d\n", __FUNCTION__, __LINE__);
  while (gdk_events_pending())
    {
      GdkEvent *ev = gdk_event_get();
      if (ev)
        {
          on_mainwin_frame_event(GTK_WINDOW(wgt), ev, NULL);
          gdk_event_free(ev);
        }
    }
  __invalidate(wgt, NULL, wgt);
}

static void
x_main_loop(void *data)
{
  struct capture_data cam_data;

  printf("%s:%d\n", __FUNCTION__, __LINE__);

  // init capture
  cam_data.w = 320;
  cam_data.h = 240;
  cam_data.vfd = v4l2_capture_init(&cam_data.framebuf, &cam_data.w,
      &cam_data.h);

  gobee_theora_resize(cam_data.w, cam_data.h, cam_data.w, sock,
      (struct sockaddr *) &cli, sizeof(cli));

  io_loop = ev_default_loop(EVFLAG_AUTO);

  xgl_qwartz.data = data;
  printf("%s:%d\n", __FUNCTION__, __LINE__);
  write(1, "qwe\n", sizeof("qwe\n"));
  ev_periodic_init(&xgl_qwartz, __session_periodic_cb, 0., 0.0333, 0);
  ev_periodic_start(io_loop, &xgl_qwartz);

  cam_qwartz.data = &cam_data;
  ev_periodic_init(&cam_qwartz, __capture_periodic_cb, 0., 0.06667, 0);
  ev_periodic_start(io_loop, &cam_qwartz);

  ev_loop(io_loop, 0);
}

void
parse_cmd_line_params(int argc, char *argv[])
{
  char c;
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

  for (;;)
    {
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

int
main(int argc, char *argv[])
{
//  int major;
//  int minor;
  GtkWidget *app_win;
  pthread_t tid;

  parse_cmd_line_params(argc, argv);
  printf("\n width: %d"
      "\n height: %d"
      "\n remote ip: %s"
      "\n remote port: %d"
      "\n local ip: %s"
      "\n local port: %d"
      "\n", displayW, displayH, remoteIp, remotePort, localIp, localPort);

  gtk_init(&argc, &argv);

  printf("%s:%d\n", __FUNCTION__, __LINE__);

  /* load the interface */
  xml = glade_xml_new(/* SHARE_DIR */"xwrcptex.glade", NULL, NULL);
  /* connect the signals in the interface */
  glade_xml_signal_autoconnect(xml);
  app_win = glade_xml_get_widget(xml, "mainwin");

  printf("%s:%d\n", __FUNCTION__, __LINE__);

  g_signal_connect(G_OBJECT(app_win), "event",
      G_CALLBACK(on_mainwin_frame_event), NULL);

#if defined(USE_THEORA)
  x_theora_init();
#endif

  printf("%s:%d\n", __FUNCTION__, __LINE__);

  if (!g_thread_supported())
    g_thread_init(NULL);
  gdk_threads_init();
  pthread_create(&tid, NULL, rtpthread, NULL);

  /* start the event loop */
  // gtk_main();
  printf("%s:%d\n", __FUNCTION__, __LINE__);
  x_main_loop((void *) app_win);

  getchar();

  return 0;
}
