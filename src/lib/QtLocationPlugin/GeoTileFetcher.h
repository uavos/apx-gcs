#ifndef GeoTileFetcher_H
#define GeoTileFetcher_H
#include <QtLocation/private/qgeotilefetcher_p.h>
#include "TileLoader.h"
class QGeoTiledMappingManagerEngine;
//=============================================================================
class GeoTileFetcher : public QGeoTileFetcher
{
  Q_OBJECT
public:
  explicit GeoTileFetcher(QGeoTiledMappingManagerEngine *parent = 0);
  ~GeoTileFetcher();

  static TileLoader *loader;

protected:
  QGeoTiledMapReply * getTileImage(const QGeoTileSpec &spec);

};
//=============================================================================
#endif
