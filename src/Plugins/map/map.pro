TARGET = map
TEMPLATE = lib
CONFIG += plugin

include( ../../../common.pri )


SOURCES += MapView.cpp \
    MapFrame.cpp \
    MapTile.cpp \
    MapTiles.cpp \
    MapPlugin.cpp \
    ItemWpt.cpp \
    ItemUav.cpp \
    ItemText.cpp \
    ItemRw.cpp \
    ItemHome.cpp \
    ItemWind.cpp \
    ItemName.cpp \
    ItemBase.cpp \
    MissionItem.cpp \
    MissionItemRw.cpp \
    MissionModel.cpp \
    MissionItemField.cpp \
    MissionItemWp.cpp \
    MissionItemHome.cpp \
    MissionListView.cpp \
    ItemTw.cpp \
    MissionItemTw.cpp \
    MissionItemPi.cpp \
    ItemPi.cpp \
    ValueEditor.cpp \
    ValueEditorArray.cpp \
    MissionItemArea.cpp \
    MissionItemCategory.cpp \
    MissionItemObject.cpp \
    MissionItemAreaPoint.cpp \
    ItemAreaPoint.cpp \
    ItemArea.cpp
HEADERS += MapView.h \
    MapFrame.h \
    MapTile.h \
    MapTiles.h \
    MapPlugin.h \
    ItemWpt.h \
    ItemUav.h \
    ItemText.h \
    ItemRw.h \
    ItemHome.h \
    ItemWind.h \
    ItemName.h \
    ItemBase.h \
    MissionItem.h \
    MissionItemRw.h \
    MissionModel.h \
    MissionItemField.h \
    MissionItemWp.h \
    MissionItemHome.h \
    MissionListView.h \
    ItemTw.h \
    MissionItemTw.h \
    MissionItemPi.h \
    ItemPi.h \
    ../../lib/ClickableLabel.h \
    ValueEditor.h \
    ValueEditorArray.h \
    MissionItemArea.h \
    MissionItemCategory.h \
    MissionItemObject.h \
    MissionItemAreaPoint.h \
    ItemAreaPoint.h \
    ItemArea.h
FORMS += MapFrame.ui \
    MissionListView.ui

QT += opengl

#QT += sql
QT += quick svg opengl

#RESOURCES += icons.qrc

