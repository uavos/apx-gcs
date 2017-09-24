TEMPLATE = app
TARGET = qgc

include( ../../gcs.pri )
include( ../../deploy.pri )

CONFIG += c++11 silent

QT += qml quick widgets svg multimedia script quickcontrols2 quickwidgets
QT += opengl

INCLUDEPATH += \
    ../shared

RESOURCES += ../qml/qml.qrc $$RES_DIR/fonts.qrc

RESOURCES += \
    qgc.qrc

OTHER_FILES += \
    main.qml

SOURCES += main.cpp \
    ../../../lib/Mandala.cpp \
    ../../../lib/MandalaCore.cpp \
    ../../../lib/MatrixMath.cpp \
    ../libgcs/QMandalaItem.cpp \
    ../libgcs/QMandalaField.cpp \
    ../libgcs/QMandala.cpp \
    ../shared/DatalinkServer.cpp \
    ../libgcs/FlightDataFile.cpp \
    ../shared/svgimageprovider.cpp \
    ../shared/QmlView.cpp \
    ../shared/QmlApp.cpp \
    ../shared/QmlWidget.cpp \
    ../shared/SoundEffects.cpp


HEADERS += \
    ../../../lib/Mandala.h \
    ../../../lib/MandalaCore.h \
    ../../../lib/MandalaVars.h \
    ../../../lib/MatrixMath.h \
    ../../../lib/preprocessor.h \
    ../../../lib/node.h \
    ../libgcs/QMandalaItem.h \
    ../libgcs/QMandalaField.h \
    ../libgcs/QMandala.h \
    ../shared/DatalinkServer.h \
    ../libgcs/FlightDataFile.h \
    ../shared/svgimageprovider.h \
    ../shared/QmlView.h \
    ../shared/QmlApp.h \
    ../shared/QmlWidget.h \
    ../shared/SoundEffects.h

