#ifndef OPTIONSFRAME_H
#define OPTIONSFRAME_H

#include <QFrame>

#include "ui_optionsframe.h"
#include "displayframe.h"

#include <gobeewrap.h>

namespace Ui {
    class OptionsFrame;
}

class OptionsFrame : public QDialog, DisplayFrameClient
{
    Q_OBJECT

public:
    explicit OptionsFrame(QWidget *parent = 0);
    ~OptionsFrame();
    void setConnection(GobeeConnection *con);
public:
    virtual void frameClientEndCall(std::string rjid, std::string sid);

private:
    virtual void showEvent(QShowEvent*evt);

private slots:
    void on_settingsButtonBox_accepted();
    void onStunEdited();
    void onHostEdited();
    void onJidEdited();
    void onPwEdited();
    void runVideoTest();

private:
    Ui::OptionsFrame *ui;
    GobeeConnection *gbconn;
};

#endif // OPTIONSFRAME_H
