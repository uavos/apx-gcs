TARGET = servos
TEMPLATE = lib
CONFIG += plugin

include( ../../gcs.pri )

SOURCES += \
    ServosPlugin.cpp \
    ServosForm.cpp
HEADERS += \
    ServosPlugin.h \
    ServosForm.h

FORMS += \
    ServosForm.ui
