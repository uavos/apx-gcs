TARGET = gcu-server

TEMPLATE = app
include( ../../gcs.pri )
include( ../../deploy.pri )

QT += core multimedia
QT -= gui
QT -= widgets script

CONFIG += console
CONFIG -= app_bundle
CONFIG += c++11

SOURCES += main.cpp \
    ../shared/DatalinkServer.cpp \
    ../shared/Serial.cpp \
    ../../../lib/comm.cpp \


HEADERS += \
    ../shared/DatalinkServer.h \
    ../shared/Serial.h \
    ../../../lib/comm.h \


#target.path = $$INSTALLBASE_BIN
#INSTALLS = target
