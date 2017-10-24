TARGET = telemetry
TEMPLATE = lib
CONFIG += plugin

include( ../../../gcs.pri )

SOURCES += TelemetryFrame.cpp \
    TelemetryPlugin.cpp \
    TelemetryPlot.cpp \
    Player.cpp

HEADERS += TelemetryFrame.h \
    TelemetryPlugin.h \
    TelemetryPlot.h \
    Player.h \
    ../../lib/ClickableLabel.h

FORMS += TelemetryFrame.ui \
    Filter.ui \
    Player.ui

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

