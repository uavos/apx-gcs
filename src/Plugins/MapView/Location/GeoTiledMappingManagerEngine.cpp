/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "GeoTiledMappingManagerEngine.h"
#include "GeoPlugin.h"
#include "GeoTileFetcher.h"
#include "TileLoader.h"

#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtLocation/private/qgeofiletilecache_p.h>
#include <QtLocation/private/qgeomaptype_p.h>
#include <QtLocation/private/qgeotiledmap_p.h>

#include <QDir>
#include <QStandardPaths>
#include <QtQml>

GeoTiledMap::GeoTiledMap(QGeoTiledMappingManagerEngine *engine, QObject *parent)
    : QGeoTiledMap(engine, parent)
{
    setPrefetchStyle(PrefetchTwoNeighbourLayers); //PrefetchTwoNeighbourLayers);//NoPrefetching);
}

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

    const QByteArray pname("default");
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

GeoTiledMappingManagerEngine::~GeoTiledMappingManagerEngine() {}

QGeoMap *GeoTiledMappingManagerEngine::createMap()
{
    return new GeoTiledMap(this);
}
