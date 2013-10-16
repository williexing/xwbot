#ifndef GOBEEGRAPHICSSCENE_H
#define GOBEEGRAPHICSSCENE_H

#include <QtCore>
#include <QtGui>
#include <QGraphicsScene>

class GobeeGraphicsScene : public QGraphicsScene
{

    /* scene interface */
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);

public:
    GobeeGraphicsScene();

};

#endif // GOBEEGRAPHICSSCENE_H
