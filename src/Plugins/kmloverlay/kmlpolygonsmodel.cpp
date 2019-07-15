#include "kmlpolygonsmodel.h"

#include <QGeoCoordinate>
#include <QDebug>

KmlPolygonsModel::KmlPolygonsModel()
{

}

void KmlPolygonsModel::setPolygons(const QList<QPolygonF> &polygons)
{
    m_allPolygons = polygons;
    updateViewPolygons();
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
    if(row >= 0 && row < m_viewPolygons.size() && role == Polygon)
    {
        QVariantList result;
        const QPolygonF &polygon = m_viewPolygons[row];
        for(int i = 0; i < polygon.size(); i++)
            result.append(QVariant::fromValue(QGeoCoordinate(polygon[i].x(), polygon[i].y())));

        return QVariant::fromValue(result);
    }
    else
        return QVariant();
}

QHash<int, QByteArray> KmlPolygonsModel::roleNames() const
{
    QHash<int, QByteArray> rolenames = {
        {Polygon, "polygon"}
    };
    return rolenames;
}

void KmlPolygonsModel::updateViewPolygons()
{
    beginResetModel();
    m_viewPolygons.clear();
    for(auto p: m_allPolygons)
    {
        QPolygonF newp = p.intersected(m_bb);
        if(!newp.isEmpty())
        {
            m_viewPolygons.append(newp);
        }
    }
    endResetModel();
}
