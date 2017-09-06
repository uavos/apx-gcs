TEMPLATE = app
include( ../../gcs.pri )
include( ../../deploy.pri )

LIBS =
QT =

PAWNC_LIB_SRCS += \
  sc.h \
  sc1.c \
  sc2.c \
  sc3.c \
  sc4.c \
  sc5.c \
  sc6.c \
  sc7.c \
  sci18n.c \
  sclist.c \
  scmemfil.c \
  scstate.c \
  scvars.c \
  lstring.c \
  memfile.c \


PAWNC_LIB_SRCS += ../linux/binreloc.c

PAWNC_LIB_SRCS += ../linux/getch.c
PAWNC_LIB_SRCS += ../amx/keeloq.c
PAWNC_LIB_SRCS += scexpand.c


#DEFINES += AMX_NO_MACRO_INSTR
#DEFINES += AMX_NO_PACKED_OPC
#DEFINES += NDEBUG
#DEFINES += NO_MAIN
#DEFINES += NO_DEFINE
DEFINES += PAWN_CELL_SIZE=32
#DEFINES += PAWN_LIGHT
#DEFINES += PAWN_NO_CODEPAGE
#DEFINES += PAWN_NO_UTF8
DEFINES += FLOATPOINT


DEFINES += LINUX
DEFINES += ENABLE_BINRELOC


DEFINES += HAVE_UNISTD_H
DEFINES += HAVE_INTTYPES_H
DEFINES += HAVE_STDINT_H
DEFINES += HAVE_ALLOCA_H
#DEFINES += HAVE_ENDIAN_H
DEFINES += HAVE_STRLCPY
DEFINES += HAVE_STRLCAT


INCLUDEPATH += linux compiler

for(file, PAWNC_LIB_SRCS) {
    SOURCES += compiler/$$file
}

