#TRANSLATIONS = $$PWD/localization/ru.ts \
#               $$PWD/localization/by.ts

CONFIG(release, debug|release) {
    TRANSLATION_TARGET_DIR = $${OUT_PWD}/localization/$$TARGET
    LANGUPD_OPTIONS = -locations relative -no-ui-lines
    LANGREL_OPTIONS = -compress -nounfinished -removeidentical
} else {
    TRANSLATION_TARGET_DIR = $${OUT_PWD}/localization/$$TARGET
    LANGUPD_OPTIONS =
    LANGREL_OPTIONS = #-markuntranslated "MISS_TR "
}

isEmpty(QMAKE_LUPDATE) {
    win32:LANGUPD = $$[QT_INSTALL_BINS]\lupdate.exe
    else:LANGUPD = $$[QT_INSTALL_BINS]/lupdate
}

isEmpty(QMAKE_LRELEASE) {
    win32:LANGREL = $$[QT_INSTALL_BINS]\lrelease.exe
    else:LANGREL = $$[QT_INSTALL_BINS]/lrelease
}

lupdate_only{
SOURCES += \
    ../shared/qml/*.qml \
    ../shared/qml/comm/*.qml \
    ../shared/qml/components/*.qml \
    ../shared/qml/hdg/*.qml \
    ../shared/qml/nav/*.qml \
    ../shared/qml/pfd/*.qml \
    $$OBJECTS_DIR/QMandalaStrings.h
}

TRANSLATION_TARGET_DIR = $$DESTDIR/../localization/$$TARGET
TS_FILES = $$PWD/localization/*.ts

trstrings.target = $$OBJECTS_DIR/QMandalaStrings.h
trstrings.commands = gcc -E -dD $$PWD/src/shared/QMandalaStrings.h > $$trstrings.target
QMAKE_EXTRA_TARGETS += trstrings

translations.input = TS_FILES
translations.output = $$TRANSLATION_TARGET_DIR/${QMAKE_FILE_BASE}.qm
translations.commands = \
  echo \"** translations **\" && \
  $(MKDIR) \"$$shell_path($$TRANSLATION_TARGET_DIR)\" && \
  $$LANGUPD $$LANGUPD_OPTIONS  $$PWD/*.pro -ts ${QMAKE_FILE_IN} && \
  $$LANGREL $$LANGREL_OPTIONS ${QMAKE_FILE_IN} -qm $$TRANSLATION_TARGET_DIR/${QMAKE_FILE_BASE}.qm && \
  $(COPY_FILE) \"$$shell_path(${QMAKE_FILE_IN})\" \"$$shell_path($$TRANSLATION_TARGET_DIR/)\"
translations.CONFIG += no_link target_predeps
translations.depends += $$trstrings.target
QMAKE_EXTRA_COMPILERS += translations
PRE_TARGETDEPS += compiler_translations_make_all


