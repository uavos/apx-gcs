TARGET = nodes
TEMPLATE = lib
CONFIG += plugin

include( ../../../common.pri )

QT += quick

SOURCES += NodesFrame.cpp \
    NodesPlugin.cpp \

HEADERS += NodesFrame.h \
    ../../lib/ClickableLabel.h \
    NodesPlugin.h \

FORMS += NodesFrame.ui \

