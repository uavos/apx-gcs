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
