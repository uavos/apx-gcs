#ifndef KMLOVERLAY_H
#define KMLOVERLAY_H

#include <QtCore>
#include <QtLocation>
#include <Fact/Fact.h>
#include "kmlpolygonsmodel.h"

class KmlOverlay : public Fact
{
    Q_OBJECT
    Q_PROPERTY(KmlPolygonsModel *kmlPolygons READ getKmlPolygons);
public:
    explicit KmlOverlay(Fact *parent = nullptr);

    KmlPolygonsModel* getKmlPolygons() const;

    Q_INVOKABLE void updateKmlModels(const QGeoShape &shape);

private:
    KmlPolygonsModel *m_kmlPolygons;
    QPointF gc2p(const QGeoCoordinate &c);
};

#endif //KMLOVERLAY_H
