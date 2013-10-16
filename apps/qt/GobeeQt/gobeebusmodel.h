#ifndef GOBEEBUSMODEL_H
#define GOBEEBUSMODEL_H

#include <map>
#include <QAbstractItemModel>

#include <xwlib++/x_objxx.h>

class GobeeBusModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit GobeeBusModel(QObject *parent = 0);

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;

    void setRootNode(gobee::__x_object *rootObj);
    void refreshModel();
    static GobeeBusModel *getModelForXObject(gobee::__x_object *xobj);

signals:

public slots:

private:
    gobee::__x_object *root;
};

#endif // GOBEEBUSMODEL_H
