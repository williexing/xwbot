#ifndef MESSAGEFORM_H
#define MESSAGEFORM_H

#include <QWidget>

namespace Ui {
class MessageForm;
}

class MessageForm : public QWidget
{
    Q_OBJECT
    
public:
    explicit MessageForm(QWidget *parent = 0);
    ~MessageForm();
    
private:
    Ui::MessageForm *ui;
};

#endif // MESSAGEFORM_H
