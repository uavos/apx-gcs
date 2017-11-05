TEMPLATE = lib
TARGET = gcs

!mac {
TARGET = $$qtLibraryTarget($$TARGET)
}

include( ../../gcs.pri )

DESTDIR = $$DESTDIR_LIB

QT += network
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


# Fact System
SOURCES += \
    FactSystem/FactSystem.cpp \
    FactSystem/FactSystemApp.cpp \
    FactSystem/FactSystemJS.cpp \
    FactSystem/FactTree.cpp \
    FactSystem/FactData.cpp \
    FactSystem/Fact.cpp \

HEADERS += \
    FactSystem/FactSystem.h \
    FactSystem/FactSystemApp.h \
    FactSystem/FactSystemJS.h \
    FactSystem/FactTree.h \
    FactSystem/FactData.h \
    FactSystem/FactValue.h \
    FactSystem/Fact.h \

# Vehicles & Mandala
SOURCES += \
    Vehicles/QMandalaField.cpp \
    Vehicles/QMandalaItem.cpp \
    Vehicles/QMandala.cpp \
    Vehicles/Vehicles.cpp \
    Vehicles/Vehicle.cpp \
    Vehicles/VehicleMandala.cpp \
    Vehicles/VehicleMandalaFact.cpp \
    Vehicles/VehicleNmtManager.cpp \
    Vehicles/VehicleRecorder.cpp \
    Vehicles/VehicleWarnings.cpp \

HEADERS += \
    Vehicles/QMandalaField.h \
    Vehicles/QMandalaItem.h \
    Vehicles/QMandala.h \
    Vehicles/Vehicles.h \
    Vehicles/Vehicle.h \
    Vehicles/VehicleMandala.h \
    Vehicles/VehicleMandalaFact.h \
    Vehicles/MandalaValue.h \
    Vehicles/VehicleNmtManager.h \
    Vehicles/VehicleRecorder.h \
    Vehicles/VehicleWarnings.h \
    Vehicles/Vehicles \

# Nodes
SOURCES += \
    Nodes/Nodes.cpp \
    Nodes/NodeItem.cpp \
    Nodes/NodeData.cpp \
    Nodes/NodeField.cpp \

HEADERS += \
    Nodes/Nodes.h \
    Nodes/NodeItem.h \
    Nodes/NodeData.h \
    Nodes/NodeField.h \


# Communication
SOURCES += \
    Datalink/Datalink.cpp \
    Datalink/DatalinkPorts.cpp \
    Datalink/DatalinkPort.cpp \
    Datalink/DatalinkSocket.cpp \
    Datalink/DatalinkHosts.cpp \
    Datalink/DatalinkHost.cpp \
    Datalink/DatalinkClients.cpp \
    Datalink/DatalinkClient.cpp \
    Datalink/DatalinkStats.cpp \
    Datalink/EscReader.cpp \
    Datalink/HttpService.cpp \
    Datalink/Serial.cpp \

HEADERS += \
    Datalink/Datalink.h \
    Datalink/DatalinkPorts.h \
    Datalink/DatalinkPort.h \
    Datalink/DatalinkSocket.h \
    Datalink/DatalinkHosts.h \
    Datalink/DatalinkHost.h \
    Datalink/DatalinkClients.h \
    Datalink/DatalinkClient.h \
    Datalink/DatalinkStats.h \
    Datalink/EscReader.h \
    Datalink/HttpService.h \
    Datalink/Serial.h \

# settings
SOURCES += \
    AppSettings/AppDirs.cpp \
    AppSettings/AppSettings.cpp \
    AppSettings/AppShortcuts.cpp \
    AppSettings/AppShortcut.cpp \

HEADERS += \
    AppSettings/AppDirs.h \
    AppSettings/AppSettings.h \
    AppSettings/AppShortcuts.h \
    AppSettings/AppShortcut.h \

# other
SOURCES += \
    FlightDataFile.cpp \
    QmlView.cpp \
    SoundEffects.cpp \
    SvgImageProvider.cpp \

HEADERS += \
    FlightDataFile.h \
    QmlView.h \
    SoundEffects.h \
    SvgImageProvider.h \










