#include "GeoTileFetcher.h"
#include "GeoMapReply.h"
#include "TileLoader.h"
#include <QtLocation/private/qgeotilespec_p.h>
//=============================================================================
//=============================================================================
GeoTileFetcher::GeoTileFetcher(QGeoTiledMappingManagerEngine *parent)
    : QGeoTileFetcher(parent)
{
    //-- Check internet status every 30 seconds or so
    //connect(&_timer, &QTimer::timeout, this, &GeoTileFetcher::timeout);
    //_timer.setSingleShot(false);
    //_timer.start(30000);

    loader = TileLoader::instance();
    if (!loader) {
        loader = new TileLoader();
    }
}
//=============================================================================
GeoTileFetcher::~GeoTileFetcher() {}
//=============================================================================
QGeoTiledMapReply *GeoTileFetcher::getTileImage(const QGeoTileSpec &spec)
{
    return new GeoMapReply(spec, this);
}
//=============================================================================
