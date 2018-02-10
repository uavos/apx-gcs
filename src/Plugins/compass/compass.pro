TARGET = compass
TEMPLATE = lib
CONFIG += plugin

include( ../../../common.pri )

SOURCES += DrawingArea.cpp \
    CompassFrame.cpp \
    CompassPlugin.cpp
HEADERS += DrawingArea.h \
    CompassFrame.h \
    CompassPlugin.h
FORMS += 
