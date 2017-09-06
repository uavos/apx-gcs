TARGET = numbers
TEMPLATE = lib
CONFIG += plugin

include( ../../gcs.pri )


QT += quick svg opengl quickcontrols2

SOURCES += \
    NumbersPlugin.cpp \
    ../shared/svgimageprovider.cpp \
    ../shared/QmlView.cpp \
    NumbersForm.cpp

HEADERS  += \
    NumbersPlugin.h \
    ../shared/svgimageprovider.h \
    ../shared/QmlView.h \
    NumbersForm.h

#RESOURCES += ../shared/qml/qml.qrc

FORMS += \
    NumbersForm.ui
