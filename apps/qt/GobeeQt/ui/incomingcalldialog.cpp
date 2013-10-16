#include "incomingcalldialog.h"
#include "ui_incomingcalldialog.h"

IncomingCallDialog::IncomingCallDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IncomingCallDialog)
{
    ui->setupUi(this);
}

IncomingCallDialog::~IncomingCallDialog()
{
    delete ui;
}
