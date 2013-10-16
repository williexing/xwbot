#include <QStringList>
#include <QPixmap>

#include <iostream>

#include <xwlib++/x_objxx.h>

#include "gobeerostermodel.h"

static void __on_x_path_event(void *obj);
static gobee::__path_listener cmd_bus_listener;
static std::map<gobee::__x_object *,GobeeRosterModel*> xobj2model;

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

    virtual int rx(/*gobee::__x_object *to, */const  gobee::__x_object *from,
                   const  gobee::__x_object *msg)
    {
        std::string subj = const_cast<gobee::__x_object *>(msg)->__getattr("subject");
        if (subj == "presence-update")
        {
            ::printf("Presence from > '%s'\n",
                     const_cast<gobee::__x_object *>(msg)->__getattr("jid"));

            GobeeRosterModel *bmodel =
                    GobeeRosterModel::getModelForXObject(const_cast<gobee::__x_object *>(from));

            if (bmodel)
            {
                bmodel->add(const_cast<gobee::__x_object *>(msg)->__getattr("jid"));
                bmodel->refreshModel();
            }
            else
            {
                ::printf("Error! Refreshing tree\n");
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
        std::cout << "[presenceListener:GobeeRosterModel] invalid argument\n";
    else
    {
        //        std::cout << "[presenceListener:GobeeRosterModel] ok\n";
        //        std::cout << "[presenceListener:GobeeRosterModel] '" << obj->__get_name() << "'\n";
        _SBSCRB(X_OBJECT(obj),X_OBJECT(preslsr));
    }

    EXIT;
}


void
GobeeRosterModel::add(std::string jid)
{
    QMutexLocker ml(const_cast<QMutex *>(&lockme));
    presents.push_back(jid);
}

void
GobeeRosterModel::remove(std::string jid)
{
    int pos;
    QMutexLocker ml(const_cast<QMutex *>(&lockme));

    for (pos = 0; pos < presents.size(); pos++)
    {
        if (presents[pos]==jid)
        {
//            presents.erase(presents.pos);
            break;
        }
    }
}

GobeeRosterModel::GobeeRosterModel(QObject *parent) :
    QAbstractItemModel(parent), root(NULL)
{
}

void
GobeeRosterModel::setRootNode(gobee::__x_object *rootObj)
{
    if (rootObj)
    {
        root = rootObj;
        // register ourself
        xobj2model[root->get_bus()]=this;
    }
    else if (root)
    {
        xobj2model.erase(root->get_bus());
        root = NULL;
    }
}

void
GobeeRosterModel::refreshModel()
{
    QMutexLocker ml(const_cast<QMutex *>(&lockme));
    ::printf("Refreshing tree\n");
    dataChanged(createIndex(0,0),createIndex(presents.size(),1));
    reset();
    //    rowsInserted(createIndex(0,0),presents.size()-1,presents.size());
}

GobeeRosterModel *
GobeeRosterModel::getModelForXObject(gobee::__x_object *xobj)
{
    return xobj2model[xobj->get_bus()];
}

QVariant GobeeRosterModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int col = index.column();

    QMutexLocker ml(const_cast<QMutex *>(&lockme));

    if (role == Qt::DisplayRole)
    {
        if(col == 0)
        {
            int id = index.internalId();
            QString qqq(presents[id].c_str());
            return QString(qqq.left(8)+".. "+qqq.right(3));
        }
    }
    else
        if(role == Qt::DecorationRole)
        {
            int id = index.internalId();
            return QPixmap(":/icons/disk_jockey_48.png").
                    scaled(48,48,Qt::KeepAspectRatio);
        }
        else
            if(role == Qt::ToolTipRole)
            {
                int id = index.internalId();
                return QString(presents[id].c_str());
            }
            else
                if(role == Qt::UserRole)
                {
                    int id = index.internalId();
                    return QString(presents[id].c_str());
                }

    return QVariant();
}

Qt::ItemFlags GobeeRosterModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant GobeeRosterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        return QString("objname");
    }
    return QVariant();
}

QModelIndex GobeeRosterModel::index(int row, int column, const QModelIndex &parent) const
{
    return createIndex(row,column,row);
}

QModelIndex GobeeRosterModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

int GobeeRosterModel::rowCount(const QModelIndex &parent) const
{
    QMutexLocker ml(const_cast<QMutex *>(&lockme));
    return presents.size();
}

int GobeeRosterModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

bool GobeeRosterModel::hasChildren(const QModelIndex &parent) const
{
    if(!parent.isValid())
        return true;
    return false;
}
