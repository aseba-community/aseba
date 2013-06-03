#-------------------------------------------------
#
# Project created by QtCreator 2013-04-02T16:25:05
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qt-gui
TEMPLATE = app

INCLUDEPATH += "../../.."
INCLUDEPATH += "../../../../dashel"

LIBS += "../../../../dashel-build/libdashel.a"
LIBS += "../../../../aseba-build/transport/dashel_plugins/libasebadashelplugins.a"
LIBS += "../../../../aseba-build/compiler/libasebacompiler.a"
LIBS += "../../../../aseba-build/common/libasebacommon.a"

unix:LIBS += -ludev
win32:LIBS += -lwinspool -lws2_32 -lsetupapi -luuid

SOURCES += main.cpp\
        mainwindow.cpp \
    dashelinterface.cpp

HEADERS  += mainwindow.h \
    dashelinterface.h

