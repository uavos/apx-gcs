TARGET = compass
TEMPLATE = lib
CONFIG += plugin

include( ../../../gcs.pri )

SOURCES += DrawingArea.cpp \
    CompassFrame.cpp \
    CompassPlugin.cpp
HEADERS += DrawingArea.h \
    CompassFrame.h \
    CompassPlugin.h
FORMS += 
