TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += lib

SUBDIRS += main
main.depends += lib

#CONFIG += GCSPlugins
#CONFIG += GCSUtils

# Plugins
PLUGINS += console
PLUGINS += nodes
PLUGINS += map
PLUGINS += telemetry
PLUGINS += compass
PLUGINS += servos
PLUGINS += numbers
PLUGINS += serial
PLUGINS += systree

#PLUGINS += signal
#PLUGINS += sim

# Utilities
GCSUtils {
    SUBDIRS += pawncc
}

GCSPlugins {
    for(subdir, PLUGINS) {
        SUBDIRS += Plugins/$$subdir
        Plugins/$$subdir.depends += lib
    }
}

export(SUBDIRS)
