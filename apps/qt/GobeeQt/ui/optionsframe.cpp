#include <Qt/qwidget.h>
#include <QDebug>
#include <QMessageBox>

#include "optionsframe.h"
#include "ui_optionsframe.h"

OptionsFrame::OptionsFrame(QWidget *parent) :
    QDialog(parent),gbconn(NULL),
    ui(new Ui::OptionsFrame)
{
    ui->setupUi(this);
    QObject::connect(ui->stunLineEdit, SIGNAL(editingFinished()),
                     this, SLOT(onStunEdited()));
    QObject::connect(ui->hostLineEdit, SIGNAL(editingFinished()),
                     this, SLOT(onHostEdited()));
    QObject::connect(ui->pwEdit, SIGNAL(editingFinished()),
                     this, SLOT(onPwEdited()));
    QObject::connect(ui->jidLineEdit, SIGNAL(editingFinished()),
                     this, SLOT(onJidEdited()));

    QObject::connect(ui->testVideoButton, SIGNAL(clicked()),
                     this, SLOT(runVideoTest()));
}

OptionsFrame::~OptionsFrame()
{
    delete ui;
}

void
OptionsFrame::on_settingsButtonBox_accepted()
{
}

void
OptionsFrame::onStunEdited()
{
    QString qs = ui->stunLineEdit->text();
    this->gbconn->setStunServer(
                qs.toStdString());

    QMessageBox *box = new QMessageBox();
    box->setWindowTitle(QString("Stun"));
    box->setText(QString("Stun server changed to:\"" + qs + "\""));
    box->show();
}

void
OptionsFrame::onHostEdited()
{
    QString qs = ui->hostLineEdit->text();
    this->gbconn->setStunServer(
                qs.toStdString());

    QMessageBox *box = new QMessageBox();
    box->setWindowTitle(QString("Stun"));
    box->setText(QString("Hostname changed to:\"" + qs + "\""));
    box->show();
}

void
OptionsFrame::onJidEdited()
{
    QString qs = ui->jidLineEdit->text();
    this->gbconn->setJid(
                qs.toStdString());

    QMessageBox *box = new QMessageBox();
    box->setWindowTitle(QString("Stun"));
    box->setText(QString("Jid to:\"" + qs + "\""));
    box->show();
}

void
OptionsFrame::onPwEdited()
{
    QString qs = ui->pwEdit->text();
    this->gbconn->setStunServer(
                qs.toStdString());

    QMessageBox *box = new QMessageBox();
    box->setWindowTitle(QString("Stun"));
    box->setText(QString("Password changed to:\"" + qs + "\""));
    box->show();
}

void
OptionsFrame::showEvent(QShowEvent *evt)
{
    if (gbconn)
    {
        emit ui->jidLineEdit->setText(QString(gbconn->getJid().c_str()));
        ui->pwEdit->setText(QString("1"));
        ui->stunLineEdit->setText(QString(gbconn->getStunServer().c_str()));
        ui->hostLineEdit->setText(QString(gbconn->getHost().c_str()));
    }
}

void
OptionsFrame::frameClientEndCall(std::string rjid, std::string sid)
{

}

void
OptionsFrame::setConnection(GobeeConnection *con)
{
    qDebug() << __FUNCTION__ << ": " << con ;
    this->gbconn = con;
    emit ui->jidLineEdit->setText(QString(gbconn->_jid.c_str()));
    emit ui->pwEdit->setText(QString("1"));
    emit ui->stunLineEdit->setText(QString(gbconn->_stunserver.c_str()));
    //    emit ui->lineEdit_3->setText(QString(gbconn->_host));
    emit ui->hostLineEdit->setText(QString(gbconn->_stunserver.c_str()));
}

void
OptionsFrame::runVideoTest()
{
    x_obj_attr_t hints = {0,0,0};
    gobee::__x_object *rootobj = new ("gobee:media","evtdomain") gobee::__x_object();
    gobee::__x_object *camobj = new ("gobee:media","camera") gobee::__x_object();
    gobee::__x_object *player_obj = new ("gobee:media","video_player") gobee::__x_object();

    gbconn->getBus()->add_child(rootobj);
    rootobj->add_child(camobj);
    rootobj->add_child(player_obj);

    setattr("stride","320",&hints);
    setattr("width","320",&hints);
    setattr("height","240",&hints);

    *camobj+=&hints;
    *player_obj+=&hints;

    x_object_add_follower(X_OBJECT(camobj),X_OBJECT(player_obj),0);

    attr_list_clear(&hints);
}
