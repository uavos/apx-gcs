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
#include "GeoMapReply.h"
#include "GeoTileFetcher.h"
#include "TileLoader.h"
#include <App/AppLog.h>
#include <QtLocation/private/qgeotilespec_p.h>
//=============================================================================
static const unsigned char pngSignature[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00};
static const unsigned char jpegSignature[] = {0xFF, 0xD8, 0xFF, 0x00};
static const unsigned char gifSignature[] = {0x47, 0x49, 0x46, 0x38, 0x00};
//=============================================================================
GeoMapReply::GeoMapReply(const QGeoTileSpec &spec, GeoTileFetcher *fetcher)
    : QGeoTiledMapReply(spec, fetcher)
{
    TileLoader *loader = TileLoader::instance();
    if (!loader) {
        setError(QGeoTiledMapReply::UnknownError, "no loader");
        setFinished(true);
        return;
    }

    _uid = TileLoader::uid(spec.mapId(), spec.zoom(), spec.x(), spec.y());
    connect(loader, &TileLoader::tileLoaded, this, &GeoMapReply::tileLoaded);
    connect(loader, &TileLoader::tileError, this, &GeoMapReply::tileError);
    loader->loadTile(_uid);
}
//=============================================================================
GeoMapReply::~GeoMapReply()
{
    abort();
    //qDebug()<<"~GeoMapReply";
}
//=============================================================================
void GeoMapReply::tileLoaded(quint64 uid, QByteArray data)
{
    if (uid != _uid)
        return;
    setMapImageData(data);
    QString format = getImageFormat(data);
    if (!format.isEmpty()) {
        setMapImageFormat(format);
    }
    setFinished(true);
}
void GeoMapReply::tileError(quint64 uid, QString errorString)
{
    if (uid != _uid)
        return;
    setError(QGeoTiledMapReply::CommunicationError, errorString);
    setFinished(true);
}
//=============================================================================
void GeoMapReply::abort()
{
    if (TileLoader::instance())
        TileLoader::instance()->loadCancel(_uid);
}
//=============================================================================
//=============================================================================
QString GeoMapReply::getImageFormat(const QByteArray &image)
{
    QString format;
    if (image.size() > 2) {
        if (image.startsWith(reinterpret_cast<const char *>(pngSignature)))
            format = "png";
        else if (image.startsWith(reinterpret_cast<const char *>(jpegSignature)))
            format = "jpg";
        else if (image.startsWith(reinterpret_cast<const char *>(gifSignature)))
            format = "gif";
        else {
            apxConsoleW() << "Unknown image signature";
            format = "png";
        }
    }
    return format;
}
//=============================================================================
