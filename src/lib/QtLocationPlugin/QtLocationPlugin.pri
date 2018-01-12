
QT  += location-private positioning-private network

INCLUDEPATH += $$QT.location.includes

HEADERS += \
    $$PWD/GeoPlugin.h \
    $$PWD/GeoTiledMappingManagerEngine.h \
    $$PWD/GeoTileFetcher.h \
    $$PWD/GeoMapReply.h \
    $$PWD/TileLoader.h \

SOURCES += \
    $$PWD/GeoPlugin.cpp \
    $$PWD/GeoTiledMappingManagerEngine.cpp \
    $$PWD/GeoTileFetcher.cpp \
    $$PWD/GeoMapReply.cpp \
    $$PWD/TileLoader.cpp \

OTHER_FILES += \
    $$PWD/GeoPlugin.json
