TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += lib

SUBDIRS += main
main.depends += lib


# Plugins
SUBDIRS += Plugins/console
SUBDIRS += Plugins/nodes
SUBDIRS += Plugins/map
SUBDIRS += Plugins/telemetry
SUBDIRS += Plugins/signal
SUBDIRS += Plugins/compass
SUBDIRS += Plugins/servos
SUBDIRS += Plugins/numbers
SUBDIRS += Plugins/serial
#SUBDIRS += joystick

# Utilities
SUBDIRS += pawncc
#SUBDIRS += qgc
#SUBDIRS += qgc-map
#SUBDIRS += gcs-server

#SUBDIRS += xplane

#for(subdirs, 1) {
#    entries = $$files($$subdirs)
#    for(entry, entries) {
#        name = $$replace(entry, [/\\\\], _)
#        SUBDIRS += $$name
#        eval ($${name}.subdir = $$entry)
#        for(dep, 2):eval ($${name}.depends += $$replace(dep, [/\\\\], _))
#        export ($${name}.subdir)
#        export ($${name}.depends)
#    }
#}
#export (SUBDIRS)
