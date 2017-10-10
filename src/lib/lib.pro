TEMPLATE = lib
TARGET = gcs

!mac {
TARGET = $$qtLibraryTarget($$TARGET)
}

include( ../../gcs.pri )

DESTDIR = $$DESTDIR_LIB

QT += network script
QT += serialport
QT += quick quickwidgets quickcontrols2 svg multimedia opengl

target.path = $$INSTALLBASE_LIB/lib


# Core APX shared lib
SOURCES += \
    $${APX_TOP}/lib/Mandala.cpp \
    $${APX_TOP}/lib/MandalaCore.cpp \
    $${APX_TOP}/lib/Comm.cpp \

HEADERS += \
    $${APX_TOP}/lib/Mandala.h \
    $${APX_TOP}/lib/MandalaCore.h \
    $${APX_TOP}/lib/Comm.h \
    $${APX_TOP}/lib/MandalaVars.h \


# GCS Mandala
SOURCES += \
    Mandala/QMandalaField.cpp \
    Mandala/QMandalaItem.cpp \
    Mandala/QMandala.cpp \

HEADERS += \
    Mandala/QMandalaField.h \
    Mandala/QMandalaItem.h \
    Mandala/QMandala.h \

# Fact System
SOURCES += \
    FactSystem/FactTree.cpp \

HEADERS += \
    FactSystem/FactTree.h \

# Communication
SOURCES += \
    comm/DatalinkServer.cpp \
    comm/EscReader.cpp \
    comm/HttpService.cpp \
    comm/Serial.cpp \

HEADERS += \
    comm/DatalinkServer.h \
    comm/EscReader.h \
    comm/HttpService.h \
    comm/Serial.h \

# other
SOURCES += \
    FlightDataFile.cpp \
    QmlApp.cpp \
    QmlView.cpp \
    QmlWidget.cpp \
    SoundEffects.cpp \
    SvgImageProvider.cpp \

HEADERS += \
    FlightDataFile.h \
    QmlApp.h \
    QmlView.h \
    QmlWidget.h \
    SoundEffects.h \
    SvgImageProvider.h \










