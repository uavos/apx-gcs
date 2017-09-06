TEMPLATE = app
include( ../../gcs.pri )
include( ../../deploy.pri )

CONFIG += c++11 silent

QT += qml quick widgets svg multimedia script quickcontrols2 quickwidgets
QT += opengl

INCLUDEPATH += \
    ../shared

RESOURCES += \
    ../shared/qml/qml.qrc \
    ../shared/fonts.qrc \
    qgc.qrc

OTHER_FILES += \
    main.qml

SOURCES += main.cpp \
    ../../../lib/Mandala.cpp \
    ../../../lib/MandalaCore.cpp \
    ../../../lib/MatrixMath.cpp \
    ../shared/QMandalaItem.cpp \
    ../shared/QMandalaField.cpp \
    ../shared/QMandala.cpp \
    ../shared/DatalinkServer.cpp \
    ../shared/FlightDataFile.cpp \
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
    ../shared/QMandalaItem.h \
    ../shared/QMandalaField.h \
    ../shared/QMandala.h \
    ../shared/DatalinkServer.h \
    ../shared/FlightDataFile.h \
    ../shared/svgimageprovider.h \
    ../shared/QmlView.h \
    ../shared/QmlApp.h \
    ../shared/QmlWidget.h \
    ../shared/SoundEffects.h

