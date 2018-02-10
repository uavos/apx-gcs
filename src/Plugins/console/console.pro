TARGET = console
TEMPLATE = lib
CONFIG += plugin

include( ../../../common.pri )

SOURCES += ConsolePlugin.cpp \
    Console.cpp

HEADERS += ConsolePlugin.h \
    Console.h

QT += quick
