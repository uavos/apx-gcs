#include "kmloverlay.h"

#include <QFileDialog>
#include "ApxApp.h"
#include "kmlparser.h"

KmlOverlay::KmlOverlay(Fact *parent)
    : Fact(parent, "kmloverlay", tr("KML Overlay"), tr("KML objects overlay"), Group),
      m_kmlPolygons(new KmlPolygonsModel())
{
    setIcon("google-earth");

    f_open = new Fact(this, "open", tr("Open..."), tr("Open KML file"));
    f_open->setIcon("import");

    f_overlayVisible = new Fact(this, "overlay_visible", tr("Show overlay"), "", Fact::Bool);
    f_overlayVisible->setIcon("check");
    f_overlayVisible->setValue(true);

    connect(f_open, &Fact::triggered, this, &KmlOverlay::onOpenTriggered);
    connect(f_overlayVisible, &Fact::valueChanged, this, &KmlOverlay::onOverlayVisibleValueChanged);

    ApxApp::instance()->engine()->loadQml("qrc:/kmloverlay/KmlOverlayPlugin.qml");
}

KmlPolygonsModel *KmlOverlay::getKmlPolygons() const
{
    return m_kmlPolygons;
}

QGeoCoordinate KmlOverlay::getCenter() const
{
    return m_center;
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

QGeoCoordinate KmlOverlay::p2gc(const QPointF &p)
{
    return QGeoCoordinate(p.x(), p.y());
}

void KmlOverlay::onOpenTriggered()
{
    QString path = QFileDialog::getOpenFileName(nullptr, tr("Open kml"), QDir::homePath(), "KML (*.kml)");
    if(!path.isEmpty())
    {
        QFile file(path);
        file.open(QIODevice::ReadOnly);

        m_parser.parse(file.readAll());
        file.close();

        m_center = p2gc(m_kmlPolygons->setPolygons(m_parser.getPolygons()));
        emit centerChanged();

        f_overlayVisible->setValue(true);
    }
}

void KmlOverlay::onOverlayVisibleValueChanged()
{
    if(f_overlayVisible->value().toBool())
        m_kmlPolygons->setPolygons(m_parser.getPolygons());
    else
        m_kmlPolygons->setPolygons({});
}
