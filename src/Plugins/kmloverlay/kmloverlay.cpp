#include "kmloverlay.h"
#include <ApxApp.h>

#include <kml/dom.h>
#include "geometrycollector.h"

KmlOverlay::KmlOverlay(Fact *parent)
    : Fact(parent, "kml", tr("KML Overlay"), tr("KML objects overlay"), Group)
{
    ApxApp::instance()->engine()->loadQml("qrc:/kml/KmlOverlayPlugin.qml");

    QFile file("/home/pavel/belzhd.kml");
    file.open(QIODevice::ReadOnly);

    std::string errors;
    kmldom::ElementPtr element = kmldom::Parse(QString::fromUtf8(file.readAll()).toStdString(), &errors);
    if(!errors.empty())
        qDebug() << QString::fromStdString(errors);

    GeometryCollector c;
    kmldom::SimplePreorderDriver(&c).Visit(element);
}
