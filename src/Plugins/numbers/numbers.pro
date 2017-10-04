TARGET = numbers
TEMPLATE = lib
CONFIG += plugin

include( ../../../gcs.pri )


QT += quick svg opengl quickcontrols2

SOURCES += \
    NumbersPlugin.cpp \
    NumbersForm.cpp

HEADERS  += \
    NumbersPlugin.h \
    NumbersForm.h

#RESOURCES += ../shared/qml/qml.qrc

FORMS += \
    NumbersForm.ui
