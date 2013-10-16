#ifndef INCOMINGCALLDIALOG_H
#define INCOMINGCALLDIALOG_H

#include <QWidget>

namespace Ui {
    class IncomingCallDialog;
}

class IncomingCallDialog : public QWidget
{
    Q_OBJECT

public:
    explicit IncomingCallDialog(QWidget *parent = 0);
    ~IncomingCallDialog();

private:
    Ui::IncomingCallDialog *ui;
};

#endif // INCOMINGCALLDIALOG_H
