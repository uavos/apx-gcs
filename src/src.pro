TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += libgcs

SUBDIRS += main
main.depends += libgcs


# Plugins
SUBDIRS += console
SUBDIRS += nodes
SUBDIRS += map
SUBDIRS += telemetry
SUBDIRS += signal
SUBDIRS += compass
SUBDIRS += servos
SUBDIRS += numbers
SUBDIRS += serial
#SUBDIRS += joystick

# Utilities
SUBDIRS += pawncc
SUBDIRS += qgc
SUBDIRS += qgc-map
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
