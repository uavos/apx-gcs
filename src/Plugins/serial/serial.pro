TARGET = serial
TEMPLATE = lib
CONFIG += plugin

include( ../../../common.pri )

SOURCES += \
    SerialPlugin.cpp \
    SerialForm.cpp
HEADERS += \
    SerialPlugin.h \
    SerialForm.h

FORMS += \
    SerialForm.ui
