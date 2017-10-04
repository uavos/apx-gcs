TARGET = serial
TEMPLATE = lib
CONFIG += plugin

include( ../../../gcs.pri )

SOURCES += \
    SerialPlugin.cpp \
    SerialForm.cpp
HEADERS += \
    SerialPlugin.h \
    SerialForm.h

FORMS += \
    SerialForm.ui
