TEMPLATE = lib
TARGET = gcs

!mac {
TARGET = $$qtLibraryTarget($$TARGET)
}

include( ../../gcs.pri )

DESTDIR = $$DESTDIR_LIB

SOURCES += \
    $${APX_TOP}/lib/Mandala.cpp \
    $${APX_TOP}/lib/MandalaCore.cpp \
    $${APX_TOP}/lib/Comm.cpp \
    QMandalaField.cpp \
    QMandalaItem.cpp \
    QMandala.cpp \
    FlightDataFile.cpp \
    Serial.cpp \
    EscReader.cpp \

HEADERS += \
    $${APX_TOP}/lib/MandalaVars.h \
    $${APX_TOP}/lib/Mandala.h \
    $${APX_TOP}/lib/MandalaCore.h \
    $${APX_TOP}/lib/Comm.h \
    QMandalaField.h \
    QMandalaItem.h \
    QMandala.h \
    FlightDataFile.h \
    Serial.h \
    EscReader.h \

QT += network script
QT += serialport

target.path = $$INSTALLBASE_LIB/lib
