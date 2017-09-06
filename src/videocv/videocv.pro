TARGET = $$qtLibraryTarget(videocv)
include( ../../gcu_plugin.pri )

QT += core multimedia

CONFIG += c++11

SOURCES += \
    VideocvPlugin.cpp \
    videothread.cpp \
    ffmpegplayer.cpp

HEADERS  += \
    VideocvPlugin.h \
    videothread.h \
    ffmpegplayer.h

LIBS += -lavcodec -lavformat -lavutil -lswscale -lavdevice -lavfilter
