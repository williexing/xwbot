#include <iostream>

#include "gobeegraphicsscene.h"

GobeeGraphicsScene::GobeeGraphicsScene()
{
//    statuslabel.setPlainText("Gobee Avatar");
//    statuslabel.setPos(10,10);
//    statuslabel.setZValue(500);
//    addItem(&statuslabel);
}

void
GobeeGraphicsScene::keyPressEvent(QKeyEvent *event)
{
//    std::cout << "keyPressEvent()\n";
    event->accept();
}

void
GobeeGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
//    std::cout << "mouseMoveEvent()\n";
//    event->accept();
}

void
GobeeGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
//    std::cout << "mousePressEvent()\n";
    event->accept();
}

void
GobeeGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
//    std::cout << "mouseReleaseEvent()\n";
    event->accept();
}

void
GobeeGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
//    std::cout << "mouseDoubleClickEvent()\n";
    event->accept();
}
