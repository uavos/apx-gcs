#include "kmloverlay.h"

#include "ApxApp.h"
#include "geometrycollector.h"


KmlOverlay::KmlOverlay(Fact *parent)
    : Fact(parent, "kmloverlay", tr("KML Overlay"), tr("KML objects overlay"), Group),
      m_kmlPolygons(new KmlPolygonsModel())
{

    QFile file("/home/pavel/belzhd.kml");
    file.open(QIODevice::ReadOnly);

    GeometryCollector c;
    QByteArray data = file.readAll();
    c.parse(data);

    m_kmlPolygons->setPolygons(c.getPolygons());

    ApxApp::instance()->engine()->loadQml("qrc:/kmloverlay/KmlOverlayPlugin.qml");
}

KmlPolygonsModel *KmlOverlay::getKmlPolygons() const
{
    return m_kmlPolygons;
}

void KmlOverlay::updateKmlModels(const QGeoShape &shape)
{
    QRectF bb(gc2p(shape.boundingGeoRectangle().topLeft()),
              gc2p(shape.boundingGeoRectangle().bottomRight()));
    m_kmlPolygons->setBoundingBox(bb);
}

QPointF KmlOverlay::gc2p(const QGeoCoordinate &c)
{
    return QPointF(c.latitude(), c.longitude());
}
