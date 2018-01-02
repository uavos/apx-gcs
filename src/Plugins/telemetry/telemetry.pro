TARGET = telemetry
TEMPLATE = lib
CONFIG += plugin

include( ../../../gcs.pri )

SOURCES += \
    TelemetryFrame.cpp \
    TelemetryPlugin.cpp \
    TelemetryPlot.cpp \
    TelemetryXml.cpp \
    TelemetryPlayer.cpp

HEADERS += \
    TelemetryFrame.h \
    TelemetryPlugin.h \
    TelemetryPlot.h \
    TelemetryXml.h \
    TelemetryPlayer.h

QT += quick


# QWT static includes
QWT_CONFIG = QwtPlot

include( qwt/qwt.pri )

for(file, QWT_SOURCES) {
    SOURCES += qwt/$$file
}
for(file, QWT_HEADERS) {
    HEADERS += qwt/$$file
}
INCLUDEPATH += qwt

