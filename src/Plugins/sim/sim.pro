TARGET = sim
TEMPLATE = lib
CONFIG += plugin

include( ../../../gcs.pri )

SOURCES += \
  SimPlugin.cpp \
  Sim.cpp \

HEADERS += \
  SimPlugin.h \
  Sim.h \

QT += quick
