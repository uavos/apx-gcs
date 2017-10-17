#include( common.pri )

plugin {
GCS_TOP = ../../..
} else {
GCS_TOP = ../..
}


APX_TOP = $${GCS_TOP}/..

SRC_DIR = $$GCS_TOP/src
LIB_DIR = $$SRC_DIR/lib

#message($$GCS_TOP)

#CONFIG += silent

exists($${OUT_PWD}/*.pro) {
    error("You must use shadow build (e.g. mkdir build; cd build; qmake ../gcs.pro).")
}


plugin:!mac {
  TARGET = $$qtLibraryTarget($$TARGET)
}

# Directories and paths

INCLUDEPATH += \
    $${APX_TOP}/ \
    $${APX_TOP}/lib \
    $${LIB_DIR} \
    $${LIB_DIR}/comm \
    $${LIB_DIR}/FactSystem \
    $${LIB_DIR}/AppSettings \
    $${LIB_DIR}/Mandala \

BUILD_DIR = $${GCS_TOP} #/build

RES_DIR = $${GCS_TOP}/resources

OBJECTS_DIR = $$BUILD_DIR/obj/$$TEMPLATE/$$TARGET

plugin {
  DESTDIR = $$BUILD_DIR/Plugins/gcs
  #GCS_TOP = $$GCS_TOP
  HEADERS += ../../lib/plugin_interface.h
  LIBS += -lgcs

} else {
  DESTDIR = $$BUILD_DIR/bin

  # make symbols available for plugins
  #mac: QMAKE_LFLAGS += -flat_namespace -undefined suppress

}

DESTDIR_LIB = $$BUILD_DIR/lib

LIBS += -Wl,-L$$DESTDIR_LIB

UI_DIR = $$OBJECTS_DIR
MOC_DIR = $$OBJECTS_DIR
RCC_DIR = $$OBJECTS_DIR

# Make much smaller libraries (and packages) by removing debugging informations
QMAKE_CFLAGS_RELEASE -= -g
QMAKE_CXXFLAGS_RELEASE -= -g

QT += network xml widgets script

# VERSION DEFINITION
#unix:!mac{
#  isEmpty(VERSION){
#    exists( ../debian/version ){
#      VERSION = $$system(cat ../debian/version)
#    }
#  }
#  isEmpty(BRANCH){
#    exists( ../debian/branch ){
#      BRANCH = $$system(cat ../debian/branch)
#    }
#  }
#}

unix {
  isEmpty(VERSION){
    GIT_DESCRIBE = $$system(git describe --tags --match=\'v*.*\')
    VERSION      = $$replace(GIT_DESCRIBE, "v", "")
    VERSION      = $$replace(VERSION, "-", ".")
    VERSION      = $$section(VERSION, ".", 0, 3)
  }
  isEmpty(BRANCH){
    BRANCH = $$system(git rev-parse --abbrev-ref HEAD)
  }
}

DEFINES += VERSION=$$VERSION
DEFINES += BRANCH=$$BRANCH

mac {
  CONFIG -= app_bundle
}

#android {
#  include(android.pri)
#}


include( deploy.pri )
