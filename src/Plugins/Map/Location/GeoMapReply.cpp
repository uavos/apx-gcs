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
    , fetcher(fetcher)
{
    _uid = TileLoader::uid(spec.mapId(), spec.zoom(), spec.x(), spec.y());
    connect(fetcher->loader, &TileLoader::tileLoaded, this, &GeoMapReply::tileLoaded);
    connect(fetcher->loader, &TileLoader::tileError, this, &GeoMapReply::tileError);
    fetcher->loader->loadTile(_uid);
}
//=============================================================================
GeoMapReply::~GeoMapReply()
{
    fetcher->loader->loadCancel(_uid);
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
    fetcher->loader->loadCancel(_uid);
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
