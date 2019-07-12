#ifndef GEOMETRYCOLLECTOR_H
#define GEOMETRYCOLLECTOR_H

#include "kml/dom.h"

class GeometryCollector: public kmldom::Visitor
{
public:
    GeometryCollector();

    void VisitPolygon(const kmldom::PolygonPtr& element) override;
};

#endif // GEOMETRYCOLLECTOR_H
