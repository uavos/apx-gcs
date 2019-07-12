#include "kmloverlay.h"
#include <ApxApp.h>

#include <kml/dom.h>

KmlOverlay::KmlOverlay(Fact *parent)
    : Fact(parent, "kml", tr("KML Overlay"), tr("KML objects overlay"), Group)
{
    ApxApp::instance()->engine()->loadQml("qrc:/kml/KmlOverlayPlugin.qml");

    QFile file("/home/pavel/belzhd.kml");
    file.open(QIODevice::ReadOnly);

//    kmldom::Visitor;
    std::string errors;
    kmldom::ElementPtr element = kmldom::Parse(QString::fromUtf8(file.readAll()).toStdString(), &errors);

    qDebug() << QString::fromStdString(errors);

    // Convert the type of the root element of the parse.
    const kmldom::PlacemarkPtr placemark = kmldom::AsPlacemark(element);
    if(placemark)
        qDebug() << "BINGO";
    else
        qDebug() << "FAIL";
}
