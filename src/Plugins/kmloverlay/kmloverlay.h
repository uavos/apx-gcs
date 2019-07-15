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

private:
    KmlPolygonsModel *m_kmlPolygons;
};

#endif //KMLOVERLAY_H
