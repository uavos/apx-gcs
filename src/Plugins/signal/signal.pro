TARGET = signal
TEMPLATE = lib
CONFIG += plugin

include( ../../../gcs.pri )

SOURCES += SignalFrame.cpp \
    SignalPlugin.cpp \
    SignalPlotter.cpp
HEADERS += SignalFrame.h \
    SignalPlugin.h \
    SignalPlotter.h
FORMS += SignalFrame.ui
