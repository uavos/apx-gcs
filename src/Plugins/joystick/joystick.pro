TARGET = $$qtLibraryTarget(joystick)
include( ../../gcu_plugin.pri )
SOURCES += JoystickFrame.cpp \
    JoystickPlugin.cpp \
    position2dwidget.cpp \
    flowlayout.cpp
HEADERS += JoystickFrame.h \
    JoystickPlugin.h \
    position2dwidget.h \
    flowlayout.h
FORMS += JoystickFrame.ui

QT += script
