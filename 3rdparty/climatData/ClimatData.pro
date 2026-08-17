#-------------------------------------------------
#
# Project created by QtCreator 2017-08-15T10:13:40
#
#-------------------------------------------------

QT       -= core gui
QT -= qt

#CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++98
#QMAKE_CXXFLAGS -= -std=gnu++11

CONFIG(debug,debug|release) {
    DEBUG_SFX = d
} else {
    DEBUG_SFX =
}


TARGET = ClimatData$$DEBUG_SFX
VERSION = 0.1.11

TEMPLATE = lib
#CONFIG += staticlib

unix:
OBJECTS_DIR = .obj

#DEFINES += CLIMATDATA_LIBRARY

SOURCES += climatdata.cpp \
    climatdataprivate.cpp

HEADERS += climatdata.h\
        climatdata_global.h \
        string \
    climatdataprivate.h

unix:DESTDIR = ../_lib
win32:DESTDIR = $$OUT_PWD/..
#DISTFILES = $${HEADERS}
#unix {
#    target.path = /usr/lib
#    INSTALLS += target
#}
#target.path = ../lib
#INSTALLS += target

