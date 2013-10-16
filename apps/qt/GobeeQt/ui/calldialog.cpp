#include <QtGui/QLineEdit>
#include <QString>
#include "calldialog.h"
#include "ui_calldialog.h"

CallDialog::CallDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CallDialog),
    calltype(-1)
{
    ui->setupUi(this);
}

CallDialog::~CallDialog()
{
    delete ui;
}

QString
CallDialog::getRemoteJid()
{
    return ui->jidValue->text();
}

void
CallDialog::setRemoteJid(QString jid)
{
    return ui->jidValue->setText(jid);
}

void CallDialog::on_toolButton_2_clicked()
{
    this->calltype = 0; // audio
    this->accept();
}

void CallDialog::on_toolButton_3_clicked()
{
    this->calltype = 1; // video
    this->accept();
}

void CallDialog::on_toolButton_4_clicked()
{
    this->calltype = 2; // interactive video
    this->accept();
}

void CallDialog::on_toolButton_clicked()
{
    this->calltype = 3; // chat
    this->accept();
}
