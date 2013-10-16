#ifndef GOBEEAPPLICATION_H
#define GOBEEAPPLICATION_H

#include <QApplication>

class GobeeApplication : public QApplication
{
    Q_OBJECT

protected:

public:
    explicit GobeeApplication(int &argc, char **argv);

signals:
    
public slots:
    
};

#endif // GOBEEAPPLICATION_H
