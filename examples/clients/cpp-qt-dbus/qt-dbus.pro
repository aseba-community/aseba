#-------------------------------------------------
#
# Project created by QtCreator 2015-07-16T15:29:29
#
#-------------------------------------------------

QT += core

QT -= gui


CONFIG += console
CONFIG -= app_bundle
CONFIG += qdbus

TEMPLATE = app

SOURCES += main.cpp \
    dbusinterface.cpp

HEADERS += \
    dbusinterface.h

DESTDIR = ./

TARGET = qt-dbus

# With C++11 support
greaterThan(QT_MAJOR_VERSION, 4)
{
    CONFIG += c++11
} \
else
{
    QMAKE_CXXFLAGS += -std=c++0x
}
