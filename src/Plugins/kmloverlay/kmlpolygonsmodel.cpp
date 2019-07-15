#include "kmlpolygonsmodel.h"

#include <QDebug>

KmlPolygonsModel::KmlPolygonsModel()
{

}

void KmlPolygonsModel::setPolygons(const QList<QGeoPolygon> &polygons)
{
    beginResetModel();
    m_polygons = polygons;
    endResetModel();
}

int KmlPolygonsModel::rowCount(const QModelIndex &index) const
{
    return m_polygons.size();
}

int KmlPolygonsModel::columnCount(const QModelIndex &index) const
{
    return 1;
}

QVariant KmlPolygonsModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row >= 0 && row < m_polygons.size() && role == Polygon)
    {
        QVariantList result;
        for(int i = 0; i < m_polygons[row].size(); i++)
            result.append(QVariant::fromValue(m_polygons[row].coordinateAt(i)));

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
