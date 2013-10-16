#undef DEBUG_PRFX

//#define DEBUG_PRFX "[TREE_MODEL] "
#include <xwlib/x_types.h>

#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include "treemodel.h"

/* boring declarations of local functions */

static void
custom_list_init(CustomList *pkg_tree);

static void
custom_list_class_init(CustomListClass *klass);

static void
custom_list_tree_model_init(GtkTreeModelIface *iface);

static void
custom_list_finalize(GObject *object);

static GtkTreeModelFlags
custom_list_get_flags(GtkTreeModel *tree_model);

static gint
custom_list_get_n_columns(GtkTreeModel *tree_model);

static GType
custom_list_get_column_type(GtkTreeModel *tree_model, gint index);

static gboolean
custom_list_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter,
    GtkTreePath *path);

static GtkTreePath *
custom_list_get_path(GtkTreeModel *tree_model, GtkTreeIter *iter);

static void
custom_list_get_value(GtkTreeModel *tree_model, GtkTreeIter *iter, gint column,
    GValue *value);

static gboolean
custom_list_iter_next(GtkTreeModel *tree_model, GtkTreeIter *iter);

static gboolean
custom_list_iter_children(GtkTreeModel *tree_model, GtkTreeIter *iter,
    GtkTreeIter *parent);

static gboolean
custom_list_iter_has_child(GtkTreeModel *tree_model, GtkTreeIter *iter);

static gint
custom_list_iter_n_children(GtkTreeModel *tree_model, GtkTreeIter *iter);

static gboolean
custom_list_iter_nth_child(GtkTreeModel *tree_model, GtkTreeIter *iter,
    GtkTreeIter *parent, gint n);

static gboolean
custom_list_iter_parent(GtkTreeModel *tree_model, GtkTreeIter *iter,
    GtkTreeIter *child);

static GObjectClass *parent_class = NULL; /* GObject stuff - nothing to worry about */

static struct xobjectclass __treemodel_objclass;

static int
treemodel_on_src_event(x_object *to, const x_object *from, const x_object *o)
{
  ListXObject *lobj = (ListXObject *) to;
  CustomList *cl = lobj->tmodel;
  GtkTreePath *path;
  GtkTreeIter iter;
  int id;

  TRACE("\n");

  x_string_t subj;

  subj = _AGET(o,_XS("subject"));
  if (EQ(_XS("child-append"), subj))
    {
//      cl
      id = atoi(_AGET(o,_XS("child-index")));
      TRACE("'%s' Added to '%s' index=%d\n",
          _AGET(o,"child-name"), _GETNM(from), id);
      path = gtk_tree_path_new();
      gtk_tree_path_append_index(path, id);

      custom_list_get_iter(GTK_TREE_MODEL(cl), &iter, path);
      TRACE("Notifying TreeView...\n");
      gtk_tree_model_row_inserted(GTK_TREE_MODEL(cl), path, &iter);
      gtk_tree_model_row_has_child_toggled(GTK_TREE_MODEL(cl), path, &iter);
      gtk_tree_path_free(path);
    }
  else if (EQ(_XS("child-remove"), subj))
    {
      TRACE("'%s' Removed from '%s'\n", _AGET(o,"child-name"), _GETNM(from));
      id = atoi(_AGET(o,_XS("child-index")));
      path = gtk_tree_path_new();
      gtk_tree_path_append_index(path, id);
      gtk_tree_model_row_deleted(GTK_TREE_MODEL(cl), path);
      gtk_tree_path_free(path);
    }
}

void
custom_list_set_xobject(CustomList *custom_list, x_object *xobj)
{
  TRACE("Setting treemodel to %p\n", xobj);
  custom_list->rootobj = xobj;
  TRACE("Subscribing %p to %p\n", &custom_list->xobj->xobj, xobj);
  _SBSCRB(custom_list->rootobj, &custom_list->xobj->xobj);
}

/*****************************************************************************
 *
 *  custom_list_get_type: here we register our new type and its interfaces
 *                        with the type system. If you want to implement
 *                        additional interfaces like GtkTreeSortable, you
 *                        will need to do it here.
 *
 *****************************************************************************/

GType
custom_list_get_type(void)
{
  static GType custom_list_type = 0;

  TRACE("\n");

  /* Some boilerplate type registration stuff */
  if (custom_list_type == 0)
    {
      static const GTypeInfo custom_list_info =
        { sizeof(CustomListClass), NULL, /* base_init */
        NULL, /* base_finalize */
        (GClassInitFunc) custom_list_class_init, NULL, /* class finalize */
        NULL, /* class_data */
        sizeof(CustomList), 0, /* n_preallocs */
        (GInstanceInitFunc) custom_list_init };
      static const GInterfaceInfo tree_model_info =
        { (GInterfaceInitFunc) custom_list_tree_model_init, NULL, NULL };

      /* First register the new derived type with the GObject type system */
      custom_list_type = g_type_register_static(G_TYPE_OBJECT, "CustomList",
          &custom_list_info, (GTypeFlags) 0);

      /* Now register our GtkTreeModel interface with the type system */
      g_type_add_interface_static(custom_list_type, GTK_TYPE_TREE_MODEL,
          &tree_model_info);
    }

  return custom_list_type;
}

/*****************************************************************************
 *
 *  custom_list_class_init: more boilerplate GObject/GType stuff.
 *                          Init callback for the type system,
 *                          called once when our new class is created.
 *
 *****************************************************************************/

static void
custom_list_class_init(CustomListClass *klass)
{
  GObjectClass *object_class;

  TRACE("\n");

  parent_class = (GObjectClass*) g_type_class_peek_parent(klass);
  object_class = (GObjectClass*) klass;

  object_class->finalize = custom_list_finalize;

  __treemodel_objclass.cname = _XS("__gtk_treemodel");
  __treemodel_objclass.psize = (unsigned int) (sizeof(ListXObject)
      - sizeof(x_object));
  __treemodel_objclass.rx = &treemodel_on_src_event;

  x_class_register_ns(__treemodel_objclass.cname, &__treemodel_objclass,
      _XS("gobee"));

}

/*****************************************************************************
 *
 *  custom_list_tree_model_init: init callback for the interface registration
 *                               in custom_list_get_type. Here we override
 *                               the GtkTreeModel interface functions that
 *                               we implement.
 *
 *****************************************************************************/

static void
custom_list_tree_model_init(GtkTreeModelIface *iface)
{
  iface->get_flags = custom_list_get_flags;
  iface->get_n_columns = custom_list_get_n_columns;
  iface->get_column_type = custom_list_get_column_type;
  iface->get_iter = custom_list_get_iter;
  iface->get_path = custom_list_get_path;
  iface->get_value = custom_list_get_value;
  iface->iter_next = custom_list_iter_next;
  iface->iter_children = custom_list_iter_children;
  iface->iter_has_child = custom_list_iter_has_child;
  iface->iter_n_children = custom_list_iter_n_children;
  iface->iter_nth_child = custom_list_iter_nth_child;
  iface->iter_parent = custom_list_iter_parent;
}

/*****************************************************************************
 *
 *  custom_list_init: this is called everytime a new custom list object
 *                    instance is created (we do that in custom_list_new).
 *                    Initialise the list structure's fields here.
 *
 *****************************************************************************/

static void
custom_list_init(CustomList *custom_list)
{
  custom_list->n_columns = CUSTOM_LIST_N_COLUMNS;

  custom_list->column_types[0] = G_TYPE_POINTER; /* CUSTOM_LIST_COL_RECORD    */
  custom_list->column_types[1] = G_TYPE_STRING; /* CUSTOM_LIST_COL_NAME      */
  custom_list->column_types[2] = G_TYPE_UINT; /* CUSTOM_LIST_COL_YEAR_BORN */

  g_assert(CUSTOM_LIST_N_COLUMNS == 3);

  custom_list->num_rows = 0;
  custom_list->rows = NULL;

  custom_list->stamp = g_random_int(); /* Random int to check whether an iter belongs to our model */

}

/*****************************************************************************
 *
 *  custom_list_finalize: this is called just before a custom list is
 *                        destroyed. Free dynamically allocated memory here.
 *
 *****************************************************************************/

static void
custom_list_finalize(GObject *object)
{
  /*  CustomList *custom_list = CUSTOM_LIST(object); */

  /* free all records and free all memory used by the list */
#warning IMPLEMENT

  /* must chain up - finalize parent */
  (*parent_class->finalize)(object);
}

/*****************************************************************************
 *
 *  custom_list_get_flags: tells the rest of the world whether our tree model
 *                         has any special characteristics. In our case,
 *                         we have a list model (instead of a tree), and each
 *                         tree iter is valid as long as the row in question
 *                         exists, as it only contains a pointer to our struct.
 *
 *****************************************************************************/

static GtkTreeModelFlags
custom_list_get_flags(GtkTreeModel *tree_model)
{
  TRACE("\n");

  g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), (GtkTreeModelFlags) 0);

  return GTK_TREE_MODEL_ITERS_PERSIST;
}

/*****************************************************************************
 *
 *  custom_list_get_n_columns: tells the rest of the world how many data
 *                             columns we export via the tree model interface
 *
 *****************************************************************************/

static gint
custom_list_get_n_columns(GtkTreeModel *tree_model)
{
  TRACE("\n");

  g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), 0);

  return CUSTOM_LIST(tree_model)->n_columns;
}

/*****************************************************************************
 *
 *  custom_list_get_column_type: tells the rest of the world which type of
 *                               data an exported model column contains
 *
 *****************************************************************************/

static GType
custom_list_get_column_type(GtkTreeModel *tree_model, gint index)
{
  TRACE("\n");

  g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), G_TYPE_INVALID);
  g_return_val_if_fail(index < CUSTOM_LIST(tree_model)->n_columns && index >= 0,
      G_TYPE_INVALID);

  return CUSTOM_LIST(tree_model)->column_types[index];
}

/*****************************************************************************
 *
 *  custom_list_get_iter: converts a tree path (physical position) into a
 *                        tree iter structure (the content of the iter
 *                        fields will only be used internally by our model).
 *                        We simply store a pointer to our CustomRecord
 *                        structure that represents that row in the tree iter.
 *
 *****************************************************************************/

static gboolean
custom_list_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter,
    GtkTreePath *path)
{
  x_object *xobj;
  CustomList *custom_list;
  CustomRecord *record;
  gint *indices, n, depth;
  int i;

  ENTER;
  TRACE("\n");

  g_assert(CUSTOM_IS_LIST(tree_model));
  g_assert(path != NULL);

  custom_list = CUSTOM_LIST(tree_model);

  indices = gtk_tree_path_get_indices(path);
  depth = gtk_tree_path_get_depth(path);

  TRACE("PATH: %s, depth=%d\n", gtk_tree_path_to_string(path), depth);

#ifdef _USE_XOBJ_

//  if (!iter->user_data)
  xobj = custom_list->rootobj;
//  else
//    xobj = X_OBJECT(iter->user_data);

  if (!xobj)
    return FALSE;

//  xobj = x_object_get_child_from_index(xobj, n + 1);
//  printf ("%s ",_GETNM(xobj));

  for (i = 0; i < depth && xobj; i++)
    {
      n = indices[i];
      xobj = x_object_get_child_from_index(xobj, n);
//      printf ("=> %s",xobj ? _GETNM(xobj) : "?");
    }
//  printf ("\n");

//  g_assert(xobj != NULL);
  if (!xobj)
    return FALSE;

  iter->stamp = XSTAMP;
  iter->user_data = xobj;
  iter->user_data2 = NULL; /* unused */
  iter->user_data3 = NULL; /* unused */

  TRACE("entry '%s' at index %d\n", _GETNM(xobj), n);

#else
  if (n >= custom_list->num_rows || n < 0)
  return FALSE;

  record = custom_list->rows[n];

  g_assert(record != NULL);
  g_assert(record->pos == n);

  /* We simply store a pointer to our custom record in the iter */
  iter->stamp = custom_list->stamp;
  iter->user_data = record;
  iter->user_data2 = NULL; /* unused */
  iter->user_data3 = NULL; /* unused */
#endif

//  if(!n)
//    gtk_tree_model_row_inserted(tree_model,path,iter);

  return TRUE;
}

/*****************************************************************************
 *
 *  custom_list_get_path: converts a tree iter into a tree path (ie. the
 *                        physical position of that row in the list).
 *
 *****************************************************************************/

static GtkTreePath *
custom_list_get_path(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  int n;
  GtkTreePath *path;
  x_object *xobj;
  CustomRecord *record;
  CustomList *custom_list;

  ENTER;
  TRACE("\n");

  g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), NULL);
  g_return_val_if_fail(iter != NULL, NULL);
  g_return_val_if_fail(iter->user_data != NULL, NULL);

  custom_list = CUSTOM_LIST(tree_model);

  path = gtk_tree_path_new();

#ifdef _USE_XOBJ_

  xobj = X_OBJECT(iter->user_data);
  n = x_object_get_index(xobj);
  gtk_tree_path_append_index(path, n);

  TRACE("entry '%s' at index %d\n", _GETNM(xobj), n);

#else
  record = (CustomRecord*) iter->user_data;
  gtk_tree_path_append_index(path, record->pos);
#endif

  return path;
}

/*****************************************************************************
 *
 *  custom_list_get_value: Returns a row's exported data columns
 *                         (_get_value is what gtk_tree_model_get uses)
 *
 *****************************************************************************/

static void
custom_list_get_value(GtkTreeModel *tree_model, GtkTreeIter *iter, gint column,
    GValue *value)
{
  x_object *xobj;
  CustomRecord *record;
  CustomList *custom_list;

  ENTER;
  TRACE("\n");

  g_return_if_fail(CUSTOM_IS_LIST(tree_model));
  g_return_if_fail(iter != NULL);
  g_return_if_fail(column < CUSTOM_LIST(tree_model)->n_columns);

  g_value_init(value, CUSTOM_LIST(tree_model)->column_types[column]);

  custom_list = CUSTOM_LIST(tree_model);

#ifdef _USE_XOBJ_

  xobj = X_OBJECT(iter->user_data);
  if (!xobj)
    return;

//  g_return_if_fail(xobj != NULL);

  switch (column)
    {
  case CUSTOM_LIST_COL_RECORD:
    g_value_set_pointer(value, xobj);
    break;

  case CUSTOM_LIST_COL_NAME:
    g_value_set_string(value, _GETNM(xobj));
    TRACE("entry '%s'\n", _GETNM(xobj));
    break;

  case CUSTOM_LIST_COL_YEAR_BORN:
    g_value_set_uint(value, 12345);
    break;
    }

#else

  record = (CustomRecord*) iter->user_data;

  g_return_if_fail(record != NULL);

  if (record->pos >= custom_list->num_rows)
  g_return_if_reached();

  switch (column)
    {
      case CUSTOM_LIST_COL_RECORD:
      g_value_set_pointer(value, record);
      break;

      case CUSTOM_LIST_COL_NAME:
      g_value_set_string(value, record->name);
      break;

      case CUSTOM_LIST_COL_YEAR_BORN:
      g_value_set_uint(value, record->year_born);
      break;
    }
#endif

}

/*****************************************************************************
 *
 *  custom_list_iter_next: Takes an iter structure and sets it to point
 *                         to the next row.
 *
 *****************************************************************************/

static gboolean
custom_list_iter_next(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  x_object *xobj, *nxt;
  CustomRecord *record, *nextrecord;
  CustomList *custom_list;

  ENTER;
  TRACE("\n");

  g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), FALSE);

  custom_list = CUSTOM_LIST(tree_model);

#ifdef _USE_XOBJ_
  if (iter == NULL || iter->user_data == NULL)
    {
      xobj = _CHLD(custom_list->rootobj,NULL);
    }
  else
    xobj = X_OBJECT(iter->user_data);

  if (!xobj || !(nxt = _SBL(xobj)))
    {
      TRACE("FAIL: xobj=%s, nxt=%p\n", _GETNM(xobj), nxt);
      return FALSE;
    }

  iter->stamp = XSTAMP;
  iter->user_data = nxt;

  TRACE("next '%s' => '%s'\n", _GETNM(xobj), _GETNM(nxt));

#else

  record = (CustomRecord *) iter->user_data;

  /* Is this the last record in the list? */
  if ((record->pos + 1) >= custom_list->num_rows)
  return FALSE;

  nextrecord = custom_list->rows[(record->pos + 1)];

  g_assert(nextrecord != NULL);
  g_assert(nextrecord->pos == (record->pos + 1));

  iter->stamp = custom_list->stamp;
  iter->user_data = nextrecord;
#endif

  return TRUE;
}

/*****************************************************************************
 *
 *  custom_list_iter_children: Returns TRUE or FALSE depending on whether
 *                             the row specified by 'parent' has any children.
 *                             If it has children, then 'iter' is set to
 *                             point to the first child. Special case: if
 *                             'parent' is NULL, then the first top-level
 *                             row should be returned if it exists.
 *
 *****************************************************************************/

static gboolean
custom_list_iter_children(GtkTreeModel *tree_model, GtkTreeIter *iter,
    GtkTreeIter *parent)
{
  x_object *xobj;
  x_object *xobj_chld;
  CustomList *custom_list;

  TRACE("\n");

//  g_return_val_if_fail(parent == NULL || parent->user_data != NULL, FALSE);

  g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), FALSE);

  custom_list = CUSTOM_LIST(tree_model);

#ifdef _USE_XOBJ_

  if (!parent)
    xobj = custom_list->rootobj;
  else
    xobj = X_OBJECT(parent->user_data);

  if (!xobj)
    return FALSE;

  /* special case: if parent == NULL, set iter to n-th top-level row */
  /*
   if (custom_list->rootobj->entry.child_count == 0)
   return FALSE;
   */

  TRACE("Gettin children of '%s'\n", _GETNM(xobj));

  xobj_chld = _CHLD(xobj, NULL);
  if (!xobj_chld)
    return FALSE;

  TRACE("Get children of '%s', child(%s)\n", _GETNM(xobj), _GETNM(xobj_chld));

  iter->stamp = XSTAMP;
  iter->user_data = xobj_chld;

#else
  /* No rows => no first row */
  if (custom_list->num_rows == 0)
  return FALSE;

  /* Set iter to first item in list */
  iter->stamp = custom_list->stamp;
  iter->user_data = custom_list->rows[0];
#endif

  return TRUE;
}

/*****************************************************************************
 *
 *  custom_list_iter_has_child: Returns TRUE or FALSE depending on whether
 *                              the row specified by 'iter' has any children.
 *                              We only have a list and thus no children.
 *
 *****************************************************************************/

static gboolean
custom_list_iter_has_child(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
//  gboolean res = FALSE;
  x_object *xobj = NULL;

  TRACE("\n");

  if (!iter || !iter->user_data)
    xobj = CUSTOM_LIST(tree_model)->rootobj;
  else
    xobj = X_OBJECT(iter->user_data);

  if (xobj && _CHLD(xobj,NULL))
    return TRUE;

  return TRUE;
}

/*****************************************************************************
 *
 *  custom_list_iter_n_children: Returns the number of children the row
 *                               specified by 'iter' has. This is usually 0,
 *                               as we only have a list and thus do not have
 *                               any children to any rows. A special case is
 *                               when 'iter' is NULL, in which case we need
 *                               to return the number of top-level nodes,
 *                               ie. the number of rows in our list.
 *
 *****************************************************************************/

static gint
custom_list_iter_n_children(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  int res;
  CustomList *custom_list;
  x_object *xobj = NULL;

  ENTER;
  TRACE("\n");

  g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), -1);
//  g_return_val_if_fail(iter == NULL || iter->user_data != NULL, FALSE);

  custom_list = CUSTOM_LIST(tree_model);

#ifdef _USE_XOBJ_
  if (!iter || !iter->user_data)
    xobj = custom_list->rootobj;
  else
    xobj = X_OBJECT(iter->user_data);

  if (!xobj)
    return 0;

//      XOBJ_CS_LOCK(custom_list->rootobj);
  res = xobj->entry.child_count;
//      XOBJ_CS_UNLOCK(custom_list->rootobj);
  TRACE("NUMBER of entries %s\n", res);
  return res;

#else
  /* special case: if iter == NULL, return number of top-level rows */
  if (!iter)
  return custom_list->num_rows;
#endif
  return 0; /* otherwise, this is easy again for a list */
}

/*****************************************************************************
 *
 *  custom_list_iter_nth_child: If the row specified by 'parent' has any
 *                              children, set 'iter' to the n-th child and
 *                              return TRUE if it exists, otherwise FALSE.
 *                              A special case is when 'parent' is NULL, in
 *                              which case we need to set 'iter' to the n-th
 *                              row if it exists.
 *
 *****************************************************************************/

static gboolean
custom_list_iter_nth_child(GtkTreeModel *tree_model, GtkTreeIter *iter,
    GtkTreeIter *parent, gint n)
{
  x_object *xobj;
  x_object *xobj_chld;

  CustomRecord *record;
  CustomList *custom_list;

  ENTER;
  TRACE("\n");

  g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), FALSE);

  custom_list = CUSTOM_LIST(tree_model);

  /* a list has only top-level rows */

#ifdef _USE_XOBJ_

  if (!parent || !parent->user_data)
    xobj = custom_list->rootobj;
  else
    xobj = X_OBJECT(parent->user_data);

  if (!xobj)
    {
      TRACE("No root object set\n");
      return FALSE;
    }

  /* special case: if parent == NULL, set iter to n-th top-level row */
  /*
   if (n >= custom_list->rootobj->entry.child_count)
   {
   TRACE("Invalid index: n(%d) >= %d\n",
   n, custom_list->rootobj->entry.child_count);
   return FALSE;
   }
   */

  xobj_chld = x_object_get_child_from_index(xobj, n);
  iter->stamp = XSTAMP;
  iter->user_data = xobj_chld;

#else
  /* special case: if parent == NULL, set iter to n-th top-level row */
  if (n >= custom_list->num_rows)
  return FALSE;
  record = custom_list->rows[n];

  g_assert(record != NULL);
  g_assert(record->pos == n);

  iter->stamp = custom_list->stamp;
  iter->user_data = record;
#endif

  return TRUE;
}

/*****************************************************************************
 *
 *  custom_list_iter_parent: Point 'iter' to the parent node of 'child'. As
 *                           we have a list and thus no children and no
 *                           parents of children, we can just return FALSE.
 *
 *****************************************************************************/

static gboolean
custom_list_iter_parent(GtkTreeModel *tree_model, GtkTreeIter *iter,
    GtkTreeIter *child)
{
  x_object *xo;
  x_object *xobj;

  TRACE("\n");

  if (!child)
    {
      TRACE("No child for parent\n");
      return FALSE;
    }

  xobj = X_OBJECT(child->user_data);

  if (!xobj)
    {
      TRACE("No child object for parent\n");
      return FALSE;
    }

  TRACE("Gettin parent of '%s'\n", _GETNM(xobj));

  xo = _PARNT(xobj);
  if (!xo)
    return FALSE;

  iter->stamp = XSTAMP;
  iter->user_data = xo;
  return TRUE;
}

/*****************************************************************************
 *
 *  custom_list_new:  This is what you use in your own code to create a
 *                    new custom list tree model for you to use.
 *
 *****************************************************************************/

CustomList *
custom_list_new(void)
{
  CustomList *newcustomlist;

  newcustomlist = (CustomList*) g_object_new(CUSTOM_TYPE_LIST, NULL);

  g_assert(newcustomlist != NULL);

  newcustomlist->xobj = _GNEW("__gtk_treemodel","gobee");
  newcustomlist->xobj->tmodel = newcustomlist;
  newcustomlist->rootobj = NULL;

  return newcustomlist;
}

#if 0

/*****************************************************************************
 *
 *  custom_list_append_record:  Empty lists are boring. This function can
 *                              be used in your own code to add rows to the
 *                              list. Note how we emit the "row-inserted"
 *                              signal after we have appended the row
 *                              internally, so the tree view and other
 *                              interested objects know about the new row.
 *
 *****************************************************************************/
void
custom_list_append_record(CustomList *custom_list, const gchar *name,
    guint year_born)
  {
    GtkTreeIter iter;
    GtkTreePath *path;
    CustomRecord *newrecord;
    gulong newsize;
    guint pos;

    TRACE("\n");

    g_return_if_fail(CUSTOM_IS_LIST(custom_list));
    g_return_if_fail(name != NULL);

    pos = custom_list->num_rows;

    custom_list->num_rows++;

    newsize = custom_list->num_rows * sizeof(CustomRecord*);

    custom_list->rows = g_realloc(custom_list->rows, newsize);

    newrecord = g_new0(CustomRecord, 1);

    newrecord->name = g_strdup(name);
    newrecord->name_collate_key = g_utf8_collate_key(name, -1); /* for fast sorting, used later */
    newrecord->year_born = year_born;

    custom_list->rows[pos] = newrecord;
    newrecord->pos = pos;

    /* inform the tree view and other interested objects
     *  (e.g. tree row references) that we have inserted
     *  a new row, and where it was inserted */

    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, newrecord->pos);

    custom_list_get_iter(GTK_TREE_MODEL(custom_list), &iter, path);

    gtk_tree_model_row_inserted(GTK_TREE_MODEL(custom_list), path, &iter);

    gtk_tree_path_free(path);
  }

void
fill_model(CustomList *customlist)
  {
    const gchar *firstnames[] =
      { "Joe", "Jane", "William", "Hannibal", "Timothy", "Gargamel", NULL};
    const gchar *surnames[] =
      { "Grokowich", "Twitch", "Borheimer", "Bork", NULL};
    const gchar **fname, **sname;

    printf("%s()\n", __FUNCTION__);

    for (sname = surnames; *sname != NULL; sname++)
      {
        for (fname = firstnames; *fname != NULL; fname++)
          {
            gchar *name = g_strdup_printf("%s %s", *fname, *sname);

            custom_list_append_record(customlist, name,
                1900 + (guint)(103.0 * rand() / (RAND_MAX + 1900.0)));

            printf("%s() ADD %s\n", __FUNCTION__, name);

            g_free(name);
          }
      }
  }

#endif

