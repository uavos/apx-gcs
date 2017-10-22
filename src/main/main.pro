TEMPLATE = app
TARGET = gcs


# make symbols available for plugins
QMAKE_LFLAGS += -rdynamic

include( ../../gcs.pri )
include( ../../localization.pri )

LIBS += -lgcs

RESOURCES += $$RES_DIR/standard-icons.qrc
RESOURCES += $$RES_DIR/styles.qrc

SOURCES += main.cpp \
    MainForm.cpp \
    Config.cpp \
    RunGuard.cpp \

HEADERS += MainForm.h \
    Config.h \
    RunGuard.h \

FORMS += \
    Config.ui

RESOURCES += $${SRC_DIR}/QML/qml.qrc $$RES_DIR/fonts.qrc


# COPY on BUILD
first.depends += $(first)
QMAKE_EXTRA_TARGETS += first

# COPY RESOURCES
resource_dirs = audio templates maps missions nodes scripts telemetry vpn xplane ../src/SDK
copydata.target = $$OBJECTS_DIR/copydata.stamp
copydata.commands = touch $$copydata.target && $(MKDIR) \"$$shell_path($$DESTDIR/../Resources)\" &&
for(a, resource_dirs){
  copydata.commands += $(COPY_DIR) \"$$shell_path($$PWD/$$RES_DIR/$${a})\" \"$$shell_path($$DESTDIR/../Resources)\" &&
}
copydata.commands += true
copydata.depends += $$files($$PWD/$$RES_DIR/*, true)
first.depends += copydata
export(copydata.commands)
QMAKE_EXTRA_TARGETS += copydata

export(first.depends)


# QT
QT += script quick svg opengl multimedia quickcontrols2 quickwidgets
QT += serialport

#######################################
# INSTALL behavior

# Icons and Launchers
desktop.path = $$INSTALLBASE_RES/../applications
desktop.files += ../*.desktop
icon64.path = $$INSTALLBASE_RES/../icons/hicolor/64x64/apps
icon64.files += ../*.png
INSTALLS += desktop icon64


# GCU DATA files
resources_data.files =  $$RES_DIR/*
resources_data.path = $$INSTALLBASE_RES/$$TARGET
uavos-data: INSTALLS += resources_data

# GCU SDK
sdk.files =  ../SDK/*
sdk.path = $$INSTALLBASE_RES/$$TARGET/sdk
sdk_inc.files = $${SHARED_DIR}/*.h $${APX_TOP}/lib/*.h
sdk_inc.files+= $${APX_TOP}/lib/MandalaCore.cpp
sdk_inc.files+= $${APX_TOP}/lib/tcp_*.cpp
sdk_inc.files+= $${APX_TOP}/lib/Mission.cpp
sdk_inc.path = $$INSTALLBASE_RES/$$TARGET/sdk/inc
INSTALLS += sdk sdk_inc

# Translations (install)
localization.files =  localization/*
localization.path = $$INSTALLBASE_LIB/localization/$$TARGET
INSTALLS += localization

DISTFILES += \
    ../qml/images/* \
    $$GCS_TOP/deploy/copy-libs.sh \
    $$GCS_TOP/deploy/launcher-linux.sh
