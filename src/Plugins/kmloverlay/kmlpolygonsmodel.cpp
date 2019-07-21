#include "kmlpolygonsmodel.h"

#include <QGeoCoordinate>
#include <QGeoPolygon>
#include <QDebug>
#include <algorithm>

KmlPolygonsModel::KmlPolygonsModel()
{

}

QPointF KmlPolygonsModel::setPolygons(const QList<KmlPolygon> &polygons)
{
    QPolygonF allPoints;
    for(auto &p: polygons)
        allPoints.append(toPolygon(p.data));

    auto center = std::accumulate(allPoints.begin(), allPoints.end(), QPointF(0, 0)) / allPoints.size();

    m_allPolygons = polygons;
    updateViewPolygons();

    return center;
}

void KmlPolygonsModel::setBoundingBox(const QRectF &bb)
{
    m_bb = QRectF(bb.x() - bb.width(), bb.y() - bb.height(),
                  bb.width() * 3, bb.height() * 3);
    updateViewPolygons();
}

int KmlPolygonsModel::rowCount(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return m_viewPolygons.size();
}

int KmlPolygonsModel::columnCount(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return 1;
}

QVariant KmlPolygonsModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    int row = index.row();
    if(row >= 0 && row < m_viewPolygons.size())
    {
        if(role == Polygon)
        {
            result = QVariant::fromValue(m_viewPolygons[row].data);
        }
        else if(role == Color)
        {
            result = m_viewPolygons[row].color;
        }
    }
    return result;
}

QHash<int, QByteArray> KmlPolygonsModel::roleNames() const
{
    QHash<int, QByteArray> rolenames = {
        {Polygon, "polygon"},
        {Color, "polygonColor"}
    };
    return rolenames;
}

void KmlPolygonsModel::updateViewPolygons()
{
    beginResetModel();
    m_viewPolygons.clear();
    for(auto &p: m_allPolygons)
    {
        if(toPolygon(p.data).intersects(m_bb))
            m_viewPolygons.append(p);
    }
    endResetModel();
}

QPolygonF KmlPolygonsModel::toPolygon(const QGeoPolygon &geoPolygon)
{
    QPolygonF polygon;
    for(int i = 0; i < geoPolygon.size(); i++)
        polygon.append(QPointF(geoPolygon.coordinateAt(i).latitude(),
                               geoPolygon.coordinateAt(i).longitude()));
    return polygon;
}
