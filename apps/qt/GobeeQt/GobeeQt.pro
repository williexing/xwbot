#-------------------------------------------------
#
# Project created by QtCreator 2012-08-17T00:44:19
#
#-------------------------------------------------

QT       += core gui

TARGET = GobeeQt
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp displayframe.cpp gobee/gobeedisplay.cpp \
    gobeebusmodel.cpp

TRANSLATIONS = resources/translations/rus.ts \
    resources/translations/eng.ts

HEADERS  += mainwindow.h displayframe.h gobeewrap.h gobee/gobeedisplay.h \
    gobeebusmodel.h

FORMS    += \
    ui/mainwindow.ui ui/displayframe.ui

OTHER_FILES += \
    CMakeLists.txt

RESOURCES += \
    resources/resources.qrc

INCLUDEPATH += ../../../xmppbot

LIBS += -lxw++ -lxw
