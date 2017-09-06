TEMPLATE = app
TARGET = gcs

# make symbols available for plugins
QMAKE_LFLAGS += -rdynamic

LIBS += -lgcs

include( ../../gcs.pri )

RESOURCES += $$RES_DIR/standard-icons.qrc
RESOURCES += $$RES_DIR/styles.qrc

SOURCES += main.cpp \
    MainForm.cpp \
    Config.cpp \
    RunGuard.cpp \
    ../shared/svgimageprovider.cpp \
    ../shared/QmlView.cpp \
    ../shared/SoundEffects.cpp \
    ../shared/MsgTranslator.cpp

HEADERS += MainForm.h \
    Config.h \
    RunGuard.h \
    ../shared/svgimageprovider.h \
    ../shared/QmlView.h \
    ../shared/SoundEffects.h \
    ../shared/QMandalaStrings.h \
    ../shared/MsgTranslator.h

FORMS += \
    Config.ui

RESOURCES += ../qml/qml.qrc $$RES_DIR/fonts.qrc


# libgcu
SOURCES += \
    ../shared/DatalinkServer.cpp \
    ../shared/HttpService.cpp

HEADERS += \
    ../shared/DatalinkServer.h \
    ../shared/HttpService.h





QT += script quick svg opengl multimedia quickcontrols2 quickwidgets
QT += serialport

# Icons and Launchers
desktop.path = $$INSTALLBASE_RES/../applications
desktop.files += ../*.desktop
icon64.path = $$INSTALLBASE_RES/../icons/hicolor/64x64/apps
icon64.files += ../*.png
INSTALLS += desktop icon64


# GCU DATA files
resources.files =  $$RES_DIR/*
resources.path = $$INSTALLBASE_RES/$$TARGET
uavos-data: INSTALLS += resources

# GCU SDK
gcusdk.files =  ../gcu-sdk/*
gcusdk.path = $$INSTALLBASE_RES/$$TARGET/sdk
gcusdk_inc.files = ../shared/*.h $${APX_TOP}/lib/*.h
gcusdk_inc.files+= $${APX_TOP}/lib/MandalaCore.cpp
gcusdk_inc.files+= $${APX_TOP}/lib/tcp_*.cpp
gcusdk_inc.files+= $${APX_TOP}/lib/Mission.cpp
gcusdk_inc.path = $$INSTALLBASE_RES/$$TARGET/sdk/inc
INSTALLS += gcusdk gcusdk_inc

# Translations (source)
lupdate_only{
SOURCES += \
    ../shared/qml/*.qml \
    ../shared/qml/comm/*.qml \
    ../shared/qml/components/*.qml \
    ../shared/qml/hdg/*.qml \
    ../shared/qml/nav/*.qml \
    ../shared/qml/pfd/*.qml \
    $$OBJECTS_DIR/QMandalaStrings.h
}

isEmpty(QMAKE_LUPDATE) {
    win32|os2:QMAKE_LUPDATE = $$[QT_INSTALL_BINS]lupdate.exe
    else:QMAKE_LUPDATE = $$[QT_INSTALL_BINS]/lupdate
    unix {
        !exists($$QMAKE_LUPDATE) { QMAKE_LUPDATE = lupdate }
    } else {
        !exists($$QMAKE_LUPDATE) { QMAKE_LUPDATE = lupdate }
    }
}
PRO_FILES = $$PWD/$$GCS_TOP/*.pro
updatets.input = PRO_FILES
updatets.output = $$OBJECTS_DIR/${QMAKE_FILE_BASE}.ts.upd
updatets.commands = gcc -E -dD $$PWD/$$GCS_TOP/src/shared/QMandalaStrings.h > $$OBJECTS_DIR/QMandalaStrings.h; $$QMAKE_LUPDATE -noobsolete ${QMAKE_FILE_IN}; touch $$OBJECTS_DIR/${QMAKE_FILE_BASE}.ts.upd
updatets.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += updatets

# Translations
isEmpty(QMAKE_LRELEASE) {
    win32|os2:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
    unix {
        !exists($$QMAKE_LRELEASE) { QMAKE_LRELEASE = lrelease }
    } else {
        !exists($$QMAKE_LRELEASE) { QMAKE_LRELEASE = lrelease }
    }
}
TS_FILES = $$PWD/$$GCS_TOP/localization/*.ts
updateqm.input = TS_FILES
updateqm.output = $$OBJECTS_DIR/${QMAKE_FILE_BASE}.qm.upd
updateqm.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm $$DESTDIR/../localization/${QMAKE_FILE_BASE}.qm; touch $$OBJECTS_DIR/${QMAKE_FILE_BASE}.qm.upd
updateqm.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += updateqm

# Translations (install)
localization.files =  localization/*
localization.path = $$INSTALLBASE_LIB/localization/$$TARGET
INSTALLS += localization

DISTFILES += \
    ../qml/images/* \
    $$GCS_TOP/deploy/copy-libs.sh \
    $$GCS_TOP/deploy/launcher-linux.sh
