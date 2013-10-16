#ifndef GOBEEROSTERMODEL_H
#define GOBEEROSTERMODEL_H

#include <map>
#include <QAbstractItemModel>
#include <QMutex>

#include <xwlib++/x_objxx.h>
#include <map>
#include <vector>
#include <set>

class GobeeRosterModel : public QAbstractItemModel
{
    Q_OBJECT

    // GobeeSqlConnector *sqlCon;
    std::vector<std::string> presents;
    QMutex lockme;

public:
    explicit GobeeRosterModel(QObject *parent = 0);
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;

    void add(std::string jid);
    void remove(std::string jid);

    void setRootNode(gobee::__x_object *rootObj);
    void refreshModel();
    static GobeeRosterModel *getModelForXObject(gobee::__x_object *xobj);

signals:

public slots:

private:
    gobee::__x_object *root;
};

#endif // GOBEEROSTERMODEL_H
