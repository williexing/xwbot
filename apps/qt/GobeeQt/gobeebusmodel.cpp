#include <QStringList>

#include <iostream>

#include <xwlib++/x_objxx.h>

#include "gobeebusmodel.h"

static void __on_x_path_event(void *obj);
static gobee::__path_listener cmd_bus_listener;
static std::map<gobee::__x_object *,GobeeBusModel*> xobj2model;

class presenceListener : public gobee::__x_object
{
public:
    presenceListener()
    {
        gobee::__x_class_register_xx(
                    this, sizeof(presenceListener),
                    "gobee:qt", "presencewatcher");

        cmd_bus_listener.lstnr.on_x_path_event = __on_x_path_event;
        x_path_listener_add("presence",
                            (x_path_listener_t *)(void *)&cmd_bus_listener.lstnr);
        std::cout << "[presenceListener] presenceListener()\n";
    }

    virtual int rx(gobee::__x_object *to, const  gobee::__x_object *msg,
                   const  gobee::__x_object *from)
    {
        std::string subj = const_cast<gobee::__x_object *>(msg)->__getattr("subject");
        if (subj == "presence-update")
        {
            ::printf("Presence from '%s'\n",
                     const_cast<gobee::__x_object *>(msg)->__getattr("jid"));
            GobeeBusModel *bmodel =
                    GobeeBusModel::getModelForXObject(const_cast<gobee::__x_object *>(from));
            if (bmodel)
            {
                bmodel->refreshModel();
            }
        }
        return 0;
    }
};

static presenceListener presLsnr;
static gobee::__x_object *preslsr = NULL;

void
__on_x_path_event(void *obj_)
{
    gobee::__x_object *obj = (gobee::__x_object *)obj_;
    ENTER;

    if (!preslsr)
        preslsr = new ("gobee:qt", "presencewatcher") gobee::__x_object();

    if (!obj_)
        std::cout << "[presenceListener:gobeebusmodel] invalid argument\n";
    else
    {
//        std::cout << "[presenceListener:gobeebusmodel] ok\n";
//        std::cout << "[presenceListener:gobeebusmodel] '" << obj->__get_name() << "'\n";
        _SBSCRB(X_OBJECT(obj),X_OBJECT(preslsr));
    }

    EXIT;
}


GobeeBusModel::GobeeBusModel(QObject *parent) :
    QAbstractItemModel(parent), root(NULL)
{
}

void
GobeeBusModel::setRootNode(gobee::__x_object *rootObj)
{
    root = rootObj;
    // register ourself
    xobj2model[root->get_bus()]=this;
}

void
GobeeBusModel::refreshModel()
{
    this->dataChanged(createIndex(0,0),createIndex(100,0));
}

GobeeBusModel *
GobeeBusModel::getModelForXObject(gobee::__x_object *xobj)
{
    return xobj2model[xobj->get_bus()];
}

QVariant GobeeBusModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int col = index.column();

    if (role == Qt::DisplayRole && col == 0)
    {
        //        if (!index.isValid());
        //            return QVariant("<invalid>");

        gobee::__x_object *xobj = (gobee::__x_object *)(void *)index.internalPointer();
        if (xobj)
            return QString(xobj->__get_name());
        else
            return QString("<null>");
    }
    return QVariant();
}

Qt::ItemFlags GobeeBusModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant GobeeBusModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        return QString("objname");
    }
    return QVariant();
}

QModelIndex GobeeBusModel::index(int row, int column, const QModelIndex &parent) const
{
    gobee::__x_object *xobj = (gobee::__x_object *)(void *)parent.internalPointer();

    if(!parent.isValid())
        return createIndex(row, column,(void*)(*root)[row]);

    return createIndex(row,column,(void*)(*xobj)[row]);
}

QModelIndex GobeeBusModel::parent(const QModelIndex &index) const
{
    gobee::__x_object *xobj = (gobee::__x_object *)(void *)index.internalPointer();

    if (!index.isValid())
        return QModelIndex();

    xobj =  xobj->get_parent();
    if(!xobj)
        return QModelIndex();

    return createIndex(x_object_get_index(X_OBJECT(xobj)),0,(void*)xobj);
}

int GobeeBusModel::rowCount(const QModelIndex &parent) const
{
    gobee::__x_object *xobj = (gobee::__x_object *)(void *)parent.internalPointer();

    if (!parent.isValid())
        xobj = root;

    if (!xobj)
        return 0;

    return xobj->xobj.entry.child_count;
}

int GobeeBusModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

bool GobeeBusModel::hasChildren(const QModelIndex &parent) const
{
    gobee::__x_object *xobj = (gobee::__x_object *)(void *)parent.internalPointer();

    if (!parent.isValid())
        xobj = root;
    return true;
    return xobj->get_child(NULL) ? true : false;
}
