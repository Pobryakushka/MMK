QT += core gui widgets network sql

TARGET   = TileDownloaderApp
TEMPLATE = app
CONFIG  += c++14

INCLUDEPATH += . core downloader ui

SOURCES += \
    main.cpp \
    core/tilemath.cpp \
    core/tilerangeiterator.cpp \
    core/mbtiles.cpp \
    downloader/tiledownloader.cpp \
    ui/mainwindow.cpp

HEADERS += \
    core/tileid.h \
    core/tilemath.h \
    core/tilerangeiterator.h \
    core/mbtiles.h \
    downloader/tilesource.h \
    downloader/tiledownloader.h \
    ui/mainwindow.h
