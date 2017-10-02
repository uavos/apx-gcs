TEMPLATE = app
TARGET = qgc-map

include( ../../gcs.pri )
include( ../../deploy.pri )

QT += qml quick widgets svg multimedia script quickcontrols2 quickwidgets
QT += opengl

INCLUDEPATH += \
    ../shared

RESOURCES += ../qml/qml.qrc $$RES_DIR/fonts.qrc

RESOURCES += \
    qml/map-qml.qrc \
    qml/map-icons.qrc


SOURCES += main.cpp \
    MissionItem.cpp \
    MissionItemArea.cpp \
    MissionItemAreaPoint.cpp \
    MissionItemCategory.cpp \
    MissionItemField.cpp \
    MissionItemObject.cpp \
    MissionItemPi.cpp \
    MissionItemRw.cpp \
    MissionItemTw.cpp \
    MissionItemWp.cpp \
    MissionModel.cpp \
    MissionPath.cpp \
    QmlMap.cpp \
    QmlMapPath.cpp \
    QmlMissionModel.cpp \
    QmlMapTiles.cpp \
    QmlMapTileDownloader.cpp \
    QmlMapTileLoader.cpp

HEADERS += \
    MissionItem.h \
    MissionItemArea.h \
    MissionItemAreaPoint.h \
    MissionItemCategory.h \
    MissionItemField.h \
    MissionItemObject.h \
    MissionItemPi.h \
    MissionItemRw.h \
    MissionItemTw.h \
    MissionItemWp.h \
    MissionModel.h \
    MissionPath.h \
    QmlMap.h \
    QmlMapPath.h \
    QmlMissionModel.h \
    QmlMapTiles.h \
    QmlMapTileDownloader.h \
    QmlMapTileLoader.h

#QMAKE_LFLAGS += -Wl,--rpath=\\\$\$ORIGIN




# lib
SOURCES += \
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


DISTFILES += \
    ../android/AndroidManifest.xml


#!android {
#  target.path = $$INSTALLBASE_BIN
#  INSTALLS = target
#}
