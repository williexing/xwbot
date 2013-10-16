#ifndef CALLDIALOG_H
#define CALLDIALOG_H

#include <QDialog>
#include <string>
#include "ui_calldialog.h"

namespace Ui {
    class CallDialog;
}

class CallDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CallDialog(QWidget *parent = 0);
    ~CallDialog();

    QString getRemoteJid();
    void setRemoteJid(QString jid);
    int getCallType() { return calltype; }

private slots:
    void on_toolButton_2_clicked();

    void on_toolButton_3_clicked();

    void on_toolButton_4_clicked();

    void on_toolButton_clicked();

private:
    int calltype;
    Ui::CallDialog *ui;
};

#endif // CALLDIALOG_H
