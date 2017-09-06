TEMPLATE = lib
include( ../../gcs.pri )
include( ../../deploy.pri )

LIBS =
QT = core network

CONFIG += warn_on plugin release
CONFIG -= exceptions rtti debug

VERSION = 1.0.0

INCLUDEPATH += SDK/CHeaders/XPLM
INCLUDEPATH += SDK/CHeaders/Wrappers
INCLUDEPATH += SDK/CHeaders/Widgets

# Defined to use X-Plane SDK 2.0 capabilities - no backward compatibility before 9.0
DEFINES += XPLM200

win32 {
    DEFINES += APL=0 IBM=1 LIN=0
    LIBS += -LSDK/Libraries/Win
    LIBS += -lXPLM -lXPWidgets
    TARGET = shiva.xpl
}

unix:!macx {
    DEFINES += APL=0 IBM=0 LIN=1
    TARGET = shiva.xpl
    # WARNING! This requires the latest version of the X-SDK !!!!
    QMAKE_CXXFLAGS += -fvisibility=hidden
}

macx {
    DEFINES += APL=1 IBM=0 LIN=0
    TARGET = shiva.xpl
    QMAKE_LFLAGS += -flat_namespace -undefined suppress

    # Build for multiple architectures.
    # The following line is only needed to build universal on PPC architectures.
    # QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
    # The following line defines for which architectures we build.
    CONFIG += x86
    #CONFIG -= x86_64
    #QMAKESPEC = win32-g++

    QMAKE_CXXFLAGS_RELEASE -= -arch x86_64


}

HEADERS += \
    plugin.h \
    ../shared/DatalinkServer.h \

SOURCES += \
    plugin.cpp \
    ../shared/DatalinkServer.cpp \

