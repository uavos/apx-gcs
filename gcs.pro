SUBDIRS = src
TEMPLATE = subdirs

#TRANSLATIONS = $$PWD/localization/ru.ts \
#               $$PWD/localization/by.ts

message(Qt version $$[QT_VERSION])
!equals(QT_MAJOR_VERSION, 5) | !greaterThan(QT_MINOR_VERSION, 7) {
    error("Unsupported Qt version, 5.8+ is required")
}
