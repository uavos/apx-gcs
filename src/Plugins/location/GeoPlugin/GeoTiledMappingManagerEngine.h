#ifndef GeoTiledMappingManagerEngine_H
#define GeoTiledMappingManagerEngine_H
#include <QtLocation/QGeoServiceProvider>
#include <QtLocation/private/qgeotiledmap_p.h>
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>
#define MAX_MAP_ZOOM (20.0)
//=============================================================================
class GeoTiledMap : public QGeoTiledMap
{
    Q_OBJECT
public:
    GeoTiledMap(QGeoTiledMappingManagerEngine *engine, QObject *parent = nullptr);
};
//=============================================================================
class GeoTiledMappingManagerEngine : public QGeoTiledMappingManagerEngine
{
    Q_OBJECT
public:
    GeoTiledMappingManagerEngine(const QVariantMap &parameters,
                                 QGeoServiceProvider::Error *error,
                                 QString *errorString);
    ~GeoTiledMappingManagerEngine();
    QGeoMap *createMap();
};
//=============================================================================
#endif
