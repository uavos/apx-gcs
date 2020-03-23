#ifndef GeoMapReply_H
#define GeoMapReply_H
#include <QtLocation/private/qgeotiledmapreply_p.h>
class GeoTileFetcher;
//=============================================================================
class GeoMapReply : public QGeoTiledMapReply
{
    Q_OBJECT
public:
    GeoMapReply(const QGeoTileSpec &spec, GeoTileFetcher *fetcher);
    ~GeoMapReply();
    void abort();

private slots:
    void tileLoaded(quint64 uid, QByteArray data);
    void tileError(quint64 uid, QString errorString);

private:
    quint64 _uid{0};

    QString getImageFormat(const QByteArray &image);
};
//=============================================================================
#endif
