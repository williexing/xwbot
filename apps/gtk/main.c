#undef DEBUG_PRFX
#define DEBUG_PRFX "[xwrcpt] "
#include <xwlib/x_types.h>
#include <xwlib/x_obj.h>
//#include <xmppagent.h>

#include <dirent.h>
#include <dlfcn.h>

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

#include <config.h>
#include "config.h"
#include "ui/treemodel.h"

#include <plugins/sessions_api/sessions.h>

typedef struct
{
  void
  (*func)(void *data);
  void *data;
} xWorker;

pthread_mutex_t wrkpool_mx = PTHREAD_MUTEX_INITIALIZER;

xWorker worker_stack[128];
xWorker *worker_stack_top = &worker_stack[0];

void
worker_push(void
(*func)(void *data), void *data)
{
  xWorker *wrkr;

  if (pthread_mutex_trylock(&wrkpool_mx))
    return; // don't push if busy

  if ((long long) worker_stack_top > (long long) &worker_stack[128])
    {
      /* init to tail */
      worker_stack_top = &worker_stack[128];
      goto WRKPUSHDONE;
    }
  wrkr = worker_stack_top++;
  wrkr->data = data;
  wrkr->func = func;
  WRKPUSHDONE: pthread_mutex_unlock(&wrkpool_mx);
}

xWorker *
worker_pop(void)
{
  xWorker *wrkr = NULL;
  pthread_mutex_lock(&wrkpool_mx);

  if (((long long) worker_stack_top > (long long) &worker_stack[0]))
    wrkr = --worker_stack_top;

  pthread_mutex_unlock(&wrkpool_mx);
  return wrkr;
}

/**
 * Load shared plugins
 */
static void
gobee_load_plugins(const char *_dir)
{
  char libname[64];
  DIR *dd;
  struct dirent *de;
  char *dir = (char *) _dir;

  TRACE("Loading plugins from '%s' ....\n", _dir);

  if ((dd = opendir(dir)) == NULL)
    {
      TRACE("ERROR");
      return;
    }

  while ((de = readdir(dd)))
    {
      snprintf(libname, sizeof(libname), "%s/%s", dir, de->d_name);
      if (!dlopen(libname, RTLD_LAZY | RTLD_GLOBAL))
        {
          TRACE("ERROR loading plugins: %s...\n", dlerror());
        }
      else
        TRACE("OK! loaded %s\n", libname);
    }
}

/**
 *
 */EXPORT_SYMBOL x_object *
gobee_init(const char *jid, const char *pw)
{
  x_object * _bus;
  x_obj_attr_t attrs =
    { 0, 0, 0 };

  ENTER;

  /* load all additional plugins from plugins dir */
  gobee_load_plugins(PLUGINS_DIR);
  gobee_load_plugins("./");

  /* instantiate '#xbus' object from 'gobee' namespace */
  _bus = _NEW("#xbus", "gobee");
  BUG_ON(!_bus);

  /* two ways on setting xbus attributes */
  setattr("pword", pw, &attrs);
  setattr("jid", jid, &attrs);
  _ASGN(_bus, &attrs);

  EXIT;
  return _bus;
}

EXPORT_SYMBOL void
gobee_connect(x_object *_bus)
{
  /* start xbus */
  _bus->objclass->finalize(_bus);
}

struct mainloop_params
{
  const char *p1;
  const char *p2;
};

x_object *bus;

static void *
x_main_loop(void *_params)
{
  struct mainloop_params *params = (struct mainloop_params *) _params;

  TRACE("\n'%s' =====>>>>> '%s'\n\n", params->p1, params->p2);

  gobee_connect(bus);

  x_free((void *) params->p1);
  x_free((void *) params->p2);
  x_free((void *) params);

  return NULL;
}

static void
gobee_start_call(const char *jid)
{
  x_object *_sess_;
  x_object *_chan_;
  x_obj_attr_t hints =
    { NULL, NULL, NULL, };

  if (jid)
    {
      _sess_ = (x_object *) (void *) x_session_open(jid, X_OBJECT(bus), &hints,
          X_CREAT);
    }

  if (!_sess_)
    {
      BUG();
    }

  /* open/create audio channel */
  _chan_ = x_session_channel_open2(X_OBJECT(_sess_), "m.V");

  attr_list_clear(&hints);
//  setattr(_XS("mtype"), _XS("video"), &hints);
  setattr(_XS("mtype"), _XS("audio"), &hints);
  _ASGN(_chan_, &hints);

  x_session_channel_set_media_profile(X_OBJECT(_chan_), _XS("__rtppldctl"));

  //  setattr("pwd", "MplayerAudio_pwd", &hints);
  //  setattr("ufrag", "todo", &hints);
  x_session_channel_set_transport_profile(X_OBJECT(_chan_), _XS("__icectl"),
      &hints);

  /* add channel payloads */
//  setattr("clockrate", "90000", &hints);
//  x_session_add_payload_to_channel(_chan_, _XS("96"), _XS("THEORA"), &hints);
  setattr("clockrate", "16000", &hints);
  x_session_add_payload_to_channel(_chan_, _XS("110"), _XS("SPEEX"), &hints);

  /* clean attribute list */
  attr_list_clear(&hints);


#if 0
  /* open/create video channel */
  _chan_ = x_session_channel_open2(X_OBJECT(_sess_), "m.V");

  //  setattr("pwd", "MplayerVideo_pwd", &hints);
  //  setattr("ufrag", "todo", &hints);
  x_session_channel_set_transport_profile(X_OBJECT(_chan_), _XS("__icectl"),
      &hints);
  x_session_channel_set_media_profile(X_OBJECT(_chan_), _XS("__rtppldctl"));

  /* add channel payloads */
  x_session_add_payload_to_channel(_chan_, _XS("96"), _XS("THEORA"), &hints);
#endif

}

/*
 *****************************************
 *****************************************
 *****************************************
 */

GtkWidget *jidw;
GtkWidget *qp;
GtkWidget *app_win;
GtkWidget *roster_win;
GtkWidget *login_win;

static struct _call_dlg_data
{
  GtkEntry *peer_entry;
  GtkCheckButton *video_chk_btn;
  GtkCheckButton *voice_chk_btn;
} current_call_dlg_data;

static void
show_roster_widget(void)
{
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;
  GtkBuilder *builder;
  CustomList *customlist;
  GtkTreeView *tview;

  builder = gtk_builder_new();

  if (!gtk_builder_add_from_file(builder, "xw_roster.glade", NULL))
    {
      return;
    }

  roster_win = GTK_WIDGET( gtk_builder_get_object( builder, "rosterWin" ) );

  gtk_builder_connect_signals(builder, NULL);

  customlist = custom_list_new();

  custom_list_set_xobject(customlist, X_OBJECT(bus));

//  fill_model(customlist);

  tview = GTK_TREE_VIEW( gtk_builder_get_object( builder, "roster_tree" ) );
  gtk_tree_view_set_reorderable(tview, TRUE);
  gtk_tree_view_set_model(tview, GTK_TREE_MODEL(customlist));
//  g_object_unref(customlist);

  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new();

  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text",
      CUSTOM_LIST_COL_NAME);
  gtk_tree_view_column_set_title(col, "Name");
  gtk_tree_view_append_column(GTK_TREE_VIEW(tview), col);

  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text",
      CUSTOM_LIST_COL_YEAR_BORN);
  gtk_tree_view_column_set_title(col, "Year Born");
  gtk_tree_view_append_column(GTK_TREE_VIEW(tview), col);

  g_object_unref(G_OBJECT(builder));

  gtk_widget_show(roster_win);
}

gboolean
winpool_checker(gpointer data)
{
  xWorker *wrkr;
//  TRACE("Checking winpool...\n");
  if ((wrkr = worker_pop()))
    {
      if (wrkr->func)
        wrkr->func(wrkr->data);
    }
  return TRUE;
}

G_MODULE_EXPORT void
ok_clicked(GtkButton *button, gpointer data)
{
  pthread_t tid;
  struct mainloop_params *bparams = x_malloc(sizeof(struct mainloop_params));

  bparams->p1 = strndup(gtk_entry_get_text(GTK_ENTRY(jidw)),
      gtk_entry_get_text_length(GTK_ENTRY(jidw)) + 1);
  bparams->p2 = strndup(gtk_entry_get_text(GTK_ENTRY(qp)),
      gtk_entry_get_text_length(GTK_ENTRY(qp)) + 1);

  bus = gobee_init(bparams->p1, bparams->p2);
  show_roster_widget();

  g_timeout_add(66, &winpool_checker, NULL);

  pthread_create(&tid, NULL, &x_main_loop, (void *) bparams);

  gtk_widget_destroy(login_win);
//  gtk_dialog_response();
}

G_MODULE_EXPORT void
cancel_clicked(GtkButton *button, gpointer data)
{
  exit(0);
}

G_MODULE_EXPORT void
quit_clicked(GtkWidget *wgt, gpointer data)
{
  printf("%s(): FINISHING...\n", __FUNCTION__);
  exit(0);
}

G_MODULE_EXPORT void
print_bus(GtkWidget *wgt, gpointer data)
{
  x_object_print_path(bus, 0);
}

G_MODULE_EXPORT int
gobee_run_call_dlg(GtkWidget *wgt, gpointer data)
{
  GtkBuilder *builder;
  GtkWidget *call_dlg;

  TRACE("Callback\n");

  builder = gtk_builder_new();

  if (!gtk_builder_add_from_file(builder, "xw_call_dlg.glade", NULL))
    {
      return (1);
    }

  gtk_builder_connect_signals(builder, NULL);

  call_dlg = GTK_WIDGET( gtk_builder_get_object( builder, "call_dialog" ) );

  current_call_dlg_data.peer_entry =
      GTK_ENTRY(gtk_builder_get_object( builder, "peer_addr_textbox" ));
  current_call_dlg_data.video_chk_btn =
      GTK_CHECK_BUTTON(gtk_builder_get_object( builder, "checkbutton2" ));
  current_call_dlg_data.voice_chk_btn =
      GTK_CHECK_BUTTON(gtk_builder_get_object( builder, "checkbutton1" ));

  g_object_unref(G_OBJECT(builder));

  return gtk_dialog_run(GTK_DIALOG(call_dlg));
}

G_MODULE_EXPORT int
gobee_call_to(GtkWidget *wgt, gpointer data)
{
//  GtkEntry *peer_addr;

  TRACE("Calling to '%s' ...\n",
      gtk_entry_get_text(current_call_dlg_data.peer_entry));

  gobee_start_call(gtk_entry_get_text(current_call_dlg_data.peer_entry));

  return 0;
}

/*
 *****************************************
 *****************************************
 */

int
main(int argc, char *argv[])
{
  GtkBuilder *builder;

//  GError     *error = NULL;

//  pthread_t tid;


  gtk_init(&argc, &argv);

  if (!g_thread_supported())
    g_thread_init(NULL);

  gdk_threads_init();

  builder = gtk_builder_new();

  if (!gtk_builder_add_from_file(builder, "xw_login.glade", NULL))
    {
      return (1);
    }

  login_win = GTK_WIDGET( gtk_builder_get_object( builder, "loginDialog" ) );
  jidw = GTK_WIDGET( gtk_builder_get_object( builder, "entry1" ) );
  qp = GTK_WIDGET( gtk_builder_get_object( builder, "entry2" ) );

  gtk_builder_connect_signals(builder, NULL);
  g_object_unref(G_OBJECT(builder));

  gtk_dialog_run(GTK_DIALOG(login_win));

//  gdk_threads_enter();
  gtk_main();
//  gdk_threads_leave();

  return 0;

}

