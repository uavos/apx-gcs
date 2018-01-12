#include "GeoTileFetcher.h"
#include "GeoMapReply.h"
#include <QtLocation/private/qgeotilespec_p.h>
//=============================================================================
//=============================================================================
TileLoader * GeoTileFetcher::loader=NULL;
GeoTileFetcher::GeoTileFetcher(QGeoTiledMappingManagerEngine *parent)
  : QGeoTileFetcher(parent)
{
  //-- Check internet status every 30 seconds or so
  //connect(&_timer, &QTimer::timeout, this, &GeoTileFetcher::timeout);
  //_timer.setSingleShot(false);
  //_timer.start(30000);

  if(!loader){
    loader=new TileLoader();
  }
}
//=============================================================================
GeoTileFetcher::~GeoTileFetcher()
{
}
//=============================================================================
QGeoTiledMapReply * GeoTileFetcher::getTileImage(const QGeoTileSpec &spec)
{
  return new GeoMapReply(spec,this);
}
//=============================================================================
