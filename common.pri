# -------------------------------------------------
# UAV Groundstation
# Please see our website at <http://uavos.com>
# Maintainer:
# Aliaksei Stratsilatau <sa@uavos.com>
# Copyright (C) UAVOS Inc - All Rights Reserved
# Unauthorized copying of this package, via any medium is strictly prohibited
# Proprietary and confidential
# -------------------------------------------------

QMAKE_PROJECT_DEPTH = 0 # undocumented qmake flag to force absolute paths in make files

linux {
    linux-g++ | linux-g++-64 | linux-g++-32 | linux-clang {
        message("Linux build")
        CONFIG += LinuxBuild
        DEFINES += __STDC_LIMIT_MACROS
        linux-clang {
            message("Linux clang")
            QMAKE_CXXFLAGS += -Qunused-arguments -fcolor-diagnostics
        }
    } else : linux-rasp-pi2-g++ {
        message("Linux R-Pi2 build")
        CONFIG += LinuxBuild
        DEFINES += __STDC_LIMIT_MACROS __rasp_pi2__
    } else : android-g++ {
        CONFIG += AndroidBuild MobileBuild
        DEFINES += __android__
        DEFINES += __STDC_LIMIT_MACROS
        target.path = $$DESTDIR
        equals(ANDROID_TARGET_ARCH, x86)  {
            CONFIG += Androidx86Build
            DEFINES += __androidx86__
            message("Android x86 build")
        } else {
            message("Android Arm build")
        }
    } else {
        error("Unsuported Linux toolchain, only GCC 32- or 64-bit is supported")
    }
} else : win32 {
    win32-msvc2010 | win32-msvc2012 | win32-msvc2013 | win32-msvc2015 {
        message("Windows build")
        CONFIG += WindowsBuild
        DEFINES += __STDC_LIMIT_MACROS
    } else {
        error("Unsupported Windows toolchain, only Visual Studio 2010, 2012, and 2013 are supported")
    }
} else : macx {
    macx-clang | macx-llvm {
        message("Mac build")
        CONFIG += MacBuild
        DEFINES += __macos__
        CONFIG += x86_64
        CONFIG -= x86
        QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
        #-- Not forcing anything. Let qmake find the latest, installed SDK.
        #QMAKE_MAC_SDK = macosx10.12
        #QMAKE_CXXFLAGS += -fvisibility=hidden
    } else {
        error("Unsupported Mac toolchain, only 64-bit LLVM+clang is supported")
    }
} else : ios {
    !equals(QT_MAJOR_VERSION, 5) | !greaterThan(QT_MINOR_VERSION, 4) {
        error("Unsupported Qt version, 5.5.x or greater is required for iOS")
    }
    message("iOS build")
    CONFIG  += iOSBuild MobileBuild app_bundle NoSerialBuild
    CONFIG  -= bitcode
    DEFINES += __ios__
    DEFINES += NO_SERIAL_LINK
    QMAKE_IOS_DEPLOYMENT_TARGET = 8.0
    QMAKE_APPLE_TARGETED_DEVICE_FAMILY = 1,2 # Universal
    QMAKE_LFLAGS += -Wl,-no_pie
} else {
    error("Unsupported build platform, only Linux, Windows, Android and Mac (Mac OS and iOS) are supported")
}

# Enable ccache where we can
linux|macx|ios {
    system(which ccache) {
        message("Found ccache, enabling")
        !ios {
            QMAKE_CXX = ccache $$QMAKE_CXX
            QMAKE_CC  = ccache $$QMAKE_CC
        } else {
            QMAKE_CXX = $$PWD/tools/iosccachecc.sh
            QMAKE_CC  = $$PWD/tools/iosccachecxx.sh
        }
    }
}

MobileBuild {
    DEFINES += __mobile__
}

# set version from git
GIT_TOP = $$system(git rev-parse --show-toplevel)

exists ($$GIT_TOP/.git) {
    GIT_DESCRIBE = $$system(git --git-dir $$GIT_TOP/.git --work-tree $$GIT_TOP describe --always --tags --match=\'v*.*\')
    GIT_BRANCH   = $$system(git --git-dir $$GIT_TOP/.git --work-tree $$GIT_TOP rev-parse --abbrev-ref HEAD)
    GIT_HASH     = $$system(git --git-dir $$GIT_TOP/.git --work-tree $$GIT_TOP rev-parse --short HEAD)
    GIT_TIME     = $$system(git --git-dir $$GIT_TOP/.git --work-tree $$GIT_TOP show --oneline --format=\"%ci\" -s HEAD)

    isEmpty(VERSION){
        VERSION      = $$replace(GIT_DESCRIBE, "v", "")
        VERSION      = $$replace(VERSION, "-", ".")
        VERSION      = $$section(VERSION, ".", 0, 3)
    }
    isEmpty(BRANCH){
        BRANCH = $$GIT_BRANCH
    }

    MacBuild {
        MAC_VERSION  = $$section(VERSION, ".", 0, 2)
        MAC_BUILD    = $$section(VERSION, ".", 3, 3)
        message(GCS version $${MAC_VERSION} build $${MAC_BUILD} describe $${GIT_VERSION})
    } else {
        message(GCS $${GIT_VERSION})
    }
}

isEmpty(VERSION) {
    error("Out-of-tree build")
    GIT_VERSION     = None
    VERSION         = 0.0.0   # Marker to indicate out-of-tree build
    MAC_VERSION     = 0.0.0
    MAC_BUILD       = 0
    BRANCH          = None
}
message($$VERSION ($$BRANCH))

DEFINES += VERSION=$$VERSION
DEFINES += BRANCH=$$BRANCH


# Installer configuration

installer {
    CONFIG -= debug
    CONFIG -= debug_and_release
    CONFIG += release
    message(Build Installer)
}

# Setup our supported build flavors

CONFIG(debug, debug|release) {
    message(Debug flavor)
    CONFIG += DebugBuild
} else:CONFIG(release, debug|release) {
    message(Release flavor)
    CONFIG += ReleaseBuild
} else {
    error(Unsupported build flavor)
}

#
# Warnings
#

DEFINES += _TTY_NOWARN_

MacBuild | LinuxBuild {
    QMAKE_CXXFLAGS_WARN_ON += -Wall
    WarningsAsErrorsOn {
        QMAKE_CXXFLAGS_WARN_ON += -Werror
    }
    MacBuild {
        QMAKE_CXXFLAGS_WARN_ON += -Wno-return-stack-address
        QMAKE_CXXFLAGS_WARN_ON += -Wno-address-of-packed-member
    }
}

WindowsBuild {
    win32-msvc2015 {
        QMAKE_CFLAGS -= -Zc:strictStrings
        QMAKE_CXXFLAGS -= -Zc:strictStrings
    }
    QMAKE_CFLAGS_RELEASE -= -Zc:strictStrings
    QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO -= -Zc:strictStrings

    QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings
    QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO -= -Zc:strictStrings
    QMAKE_CXXFLAGS_WARN_ON += /W3 \
        /wd4996 \   # silence warnings about deprecated strcpy and whatnot
        /wd4005 \   # silence warnings about macro redefinition
        /wd4290     # ignore exception specifications

    WarningsAsErrorsOn {
        QMAKE_CXXFLAGS_WARN_ON += /WX
    }
}

#
# Build-specific settings
#

ReleaseBuild {
    DEFINES += QT_NO_DEBUG QT_MESSAGELOGCONTEXT
    CONFIG += force_debug_info  # Enable debugging symbols on release builds
    !iOSBuild {
        !AndroidBuild {
            CONFIG += ltcg              # Turn on link time code generation
        }
    }

    WindowsBuild {
        # Enable function level linking and enhanced optimized debugging
        QMAKE_CFLAGS_RELEASE   += /Gy /Zo
        QMAKE_CXXFLAGS_RELEASE += /Gy /Zo
        QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO   += /Gy /Zo
        QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO += /Gy /Zo

        # Eliminate duplicate COMDATs
        QMAKE_LFLAGS_RELEASE += /OPT:ICF
        QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO += /OPT:ICF
    }
}





###############################################################################
#
# Build and directories
#
###############################################################################


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
    $${LIB_DIR}/FactSystem \
    $${LIB_DIR}/AppSettings \
    $${LIB_DIR}/Database \
    $${LIB_DIR}/Datalink \
    $${LIB_DIR}/Vehicles \
    $${LIB_DIR}/Nodes \
    $${LIB_DIR}/Mission \
    $${LIB_DIR}/TreeModel \
    $${LIB_DIR}/QtLocationPlugin \
    $${LIB_DIR} \
    $${APX_TOP}/ \
    $${APX_TOP}/lib \

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

QT += network xml widgets quick sql

