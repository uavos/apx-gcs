TARGET = nodes
TEMPLATE = lib
CONFIG += plugin

include( ../../../gcs.pri )

QT += quick

SOURCES += NodesFrame.cpp \
    NodesPlugin.cpp \
    NodesModel.cpp \
    NodesItem.cpp \
    NodesItemNode.cpp \
    NodesItemField.cpp \
    NodesItemGroup.cpp \
    NodesItemNgrp.cpp \
    ValueEditor.cpp \
    ValueEditorArray.cpp \
    ValueEditorScript.cpp \
    PawnScript.cpp \
    SourceEdit.cpp \
    ValueEditorNgrp.cpp \
    FirmwareLoader.cpp \
    NodesView.cpp \
    NodesRequestManager.cpp \
    BlackboxDownload.cpp \

HEADERS += NodesFrame.h \
    ../../lib/ClickableLabel.h \
    NodesPlugin.h \
    NodesModel.h \
    NodesItem.h \
    NodesItemNode.h \
    NodesItemField.h \
    NodesItemGroup.h \
    NodesItemNgrp.h \
    ValueEditor.h \
    ValueEditorArray.h \
    ValueEditorScript.h \
    PawnScript.h \
    SourceEdit.h \
    ValueEditorNgrp.h \
    FirmwareLoader.h \
    NodesView.h \
    NodesRequestManager.h \
    BlackboxDownload.h \

FORMS += NodesFrame.ui \
    ValueEditorScript.ui \
    BlackboxDownload.ui

