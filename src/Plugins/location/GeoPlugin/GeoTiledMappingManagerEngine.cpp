#include "GeoTiledMappingManagerEngine.h"
#include "GeoTileFetcher.h"
#include "GeoPlugin.h"
#include "TileLoader.h"

#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtLocation/private/qgeomaptype_p.h>
#include <QtLocation/private/qgeotiledmap_p.h>
#include <QtLocation/private/qgeofiletilecache_p.h>

#include <QDir>
#include <QStandardPaths>
#include <QtQml>
//=============================================================================
GeoTiledMap::GeoTiledMap(QGeoTiledMappingManagerEngine *engine, QObject *parent)
    : QGeoTiledMap(engine, parent)
{
    setPrefetchStyle(PrefetchTwoNeighbourLayers); //PrefetchTwoNeighbourLayers);//NoPrefetching);
}
//=============================================================================
GeoTiledMappingManagerEngine::GeoTiledMappingManagerEngine(const QVariantMap &parameters,
                                                           QGeoServiceProvider::Error *error,
                                                           QString *errorString)
    : QGeoTiledMappingManagerEngine()
{
    Q_UNUSED(parameters)

    //qmlRegisterUncreatableType<GeoTiledMap>("APX.Map", 1, 0, "GeoTiledMap", "Reference only");

    QGeoCameraCapabilities cameraCaps;
    cameraCaps.setMinimumZoomLevel(2.0);
    cameraCaps.setMaximumZoomLevel(MAX_MAP_ZOOM);
    cameraCaps.setSupportsBearing(true);
    cameraCaps.setSupportsTilting(true);
    cameraCaps.setSupportsRolling(true);
    cameraCaps.setOverzoomEnabled(true);
    cameraCaps.setMinimumFieldOfView(5);
    cameraCaps.setMaximumFieldOfView(75);
    cameraCaps.setMinimumTilt(0);
    cameraCaps.setMaximumTilt(85);
    setCameraCapabilities(cameraCaps);

    setTileSize(QSize(256, 256));
    setCacheHint(QAbstractGeoTileCache::MemoryCache);

    const QByteArray pname("uavos");
    //QGeoCameraCapabilities ccaps;
    QList<QGeoMapType> mapTypes;
#if QT_VERSION >= 0x050A00
    mapTypes << QGeoMapType(QGeoMapType::HybridMap,
                            "Hybrid Map",
                            "hybrid map",
                            false,
                            false,
                            TileLoader::GoogleHybrid,
                            pname,
                            cameraCaps);
    mapTypes << QGeoMapType(QGeoMapType::SatelliteMapDay,
                            "Satellite Map",
                            "satellite map",
                            false,
                            false,
                            TileLoader::GoogleSatellite,
                            pname,
                            cameraCaps);
#else
    mapTypes << QGeoMapType(QGeoMapType::HybridMap,
                            "Hybrid Map",
                            "hybrid map",
                            false,
                            false,
                            TileLoader::GoogleHybrid,
                            pname);
    mapTypes << QGeoMapType(QGeoMapType::SatelliteMapDay,
                            "Satellite Map",
                            "satellite map",
                            false,
                            false,
                            TileLoader::GoogleSatellite,
                            pname);
#endif
    setSupportedMapTypes(mapTypes);

    setTileFetcher(new GeoTileFetcher(this));

    *error = QGeoServiceProvider::NoError;
    errorString->clear();
}
//=============================================================================
GeoTiledMappingManagerEngine::~GeoTiledMappingManagerEngine() {}
//=============================================================================
QGeoMap *GeoTiledMappingManagerEngine::createMap()
{
    return new GeoTiledMap(this);
}
//=============================================================================
