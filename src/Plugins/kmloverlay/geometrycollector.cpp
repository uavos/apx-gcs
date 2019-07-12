#include "geometrycollector.h"

#include <QDebug>

GeometryCollector::GeometryCollector()
{

}

void GeometryCollector::VisitPolygon(const kmldom::PolygonPtr &element)
{
    qDebug() << "===============POLYGON!================";
    auto boundary = element->get_outerboundaryis();
    auto linearring = boundary->get_linearring();
    auto coordinates = linearring->get_coordinates();
    for(size_t i = 0; i < coordinates->get_coordinates_array_size(); i++)
        qDebug() << coordinates->get_coordinates_array_at(i).get_latitude() <<
                    coordinates->get_coordinates_array_at(i).get_longitude();
}
