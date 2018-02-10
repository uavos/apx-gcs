TARGET = servos
TEMPLATE = lib
CONFIG += plugin

include( ../../../common.pri )

SOURCES += \
    ServosPlugin.cpp \
    ServosForm.cpp
HEADERS += \
    ServosPlugin.h \
    ServosForm.h

FORMS += \
    ServosForm.ui
