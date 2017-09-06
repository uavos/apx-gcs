TEMPLATE = app
include( ../../gcs.pri )
include( ../../deploy.pri )

#CONFIG += c++11

LIBS -= -lgcu

QT += qml quick widgets svg multimedia script quickcontrols2 quickwidgets
QT += opengl

INCLUDEPATH += \
    ../shared

RESOURCES += \
    ../shared/qml/qml.qrc \
    ../shared/fonts.qrc \
    ../map/icons.qrc \
    qml/map-qml.qrc \
    qml/map-icons.qrc


SOURCES += main.cpp \
    ../shared/svgimageprovider.cpp \
    ../shared/QmlView.cpp \
    ../shared/SoundEffects.cpp \
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
    ../shared/svgimageprovider.h \
    ../shared/QmlView.h \
    ../shared/SoundEffects.h \
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




# libgcu
SOURCES += \
    ../../../lib/Mandala.cpp \
    ../../../lib/MandalaCore.cpp \
    ../../../lib/MatrixMath.cpp \
    ../libgcu/QMandalaItem.cpp \
    ../libgcu/QMandalaField.cpp \
    ../libgcu/QMandala.cpp \
    ../shared/DatalinkServer.cpp \
    ../libgcu/FlightDataFile.cpp \

HEADERS += \
    ../../../lib/Mandala.h \
    ../../../lib/MandalaCore.h \
    ../../../lib/MandalaVars.h \
    ../../../lib/MatrixMath.h \
    ../../../lib/preprocessor.h \
    ../../../lib/node.h \
    ../libgcu/QMandalaItem.h \
    ../libgcu/QMandalaField.h \
    ../libgcu/QMandala.h \
    ../shared/DatalinkServer.h \
    ../libgcu/FlightDataFile.h \

DISTFILES += \
    ../android/AndroidManifest.xml


#!android {
#  target.path = $$INSTALLBASE_BIN
#  INSTALLS = target
#}
