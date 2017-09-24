#include( common.pri )

GCS_TOP = ../..
APX_TOP = $${GCS_TOP}/..

#CONFIG += silent


plugin:!mac {
  TARGET = $$qtLibraryTarget($$TARGET)
}

# Directories and paths

INCLUDEPATH += \
    ../libgcs \
    ../shared \
    $${APX_TOP}/ \
    $${APX_TOP}/lib

BUILD_DIR = $${GCS_TOP} #/build

RES_DIR = $${GCS_TOP}/resources

OBJECTS_DIR = $$BUILD_DIR/obj/$$TEMPLATE/$$TARGET

plugin {
  DESTDIR = $$BUILD_DIR/plugins/gcs
  LIBS += -lgcs
  HEADERS += ../shared/plugin_interface.h

} else {
  DESTDIR = $$BUILD_DIR/bin
}

DESTDIR_LIB = $$BUILD_DIR/lib

UI_DIR = $$OBJECTS_DIR
MOC_DIR = $$OBJECTS_DIR
RCC_DIR = $$OBJECTS_DIR

# Make much smaller libraries (and packages) by removing debugging informations
QMAKE_CFLAGS_RELEASE -= -g
QMAKE_CXXFLAGS_RELEASE -= -g

QT += network xml widgets script

LIBS += -Wl,-L$$DESTDIR_LIB

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
