TARGET = console
TEMPLATE = lib
CONFIG += plugin

include( ../../../gcs.pri )

SOURCES += ConsolePlugin.cpp \
    Console.cpp

HEADERS += ConsolePlugin.h \
    Console.h

QT += quick
