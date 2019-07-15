#include "kmlpolygonsmodel.h"

#include <QGeoCoordinate>
#include <QDebug>
#include <algorithm>

KmlPolygonsModel::KmlPolygonsModel()
{

}

QPointF KmlPolygonsModel::setPolygons(const QList<KmlPolygon> &polygons)
{
    QPolygonF allPoints;
    for(auto p: polygons)
        allPoints.append(p.data);

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
    return m_viewPolygons.size();
}

int KmlPolygonsModel::columnCount(const QModelIndex &index) const
{
    return 1;
}

QVariant KmlPolygonsModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row >= 0 && row < m_viewPolygons.size())
    {
        if(role == Polygon)
        {
            QVariantList result;
            const KmlPolygon &polygon = m_viewPolygons[row];
            for(int i = 0; i < polygon.data.size(); i++)
                result.append(QVariant::fromValue(QGeoCoordinate(polygon.data[i].x(), polygon.data[i].y())));

            return QVariant::fromValue(result);
        }
        else if(role == Color)
        {
            return m_viewPolygons[row].color;
        }
    }
    return QVariant();
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
    for(auto p: m_allPolygons)
    {
        if(p.data.intersects(m_bb))
            m_viewPolygons.append(p);
    }
    endResetModel();
}
