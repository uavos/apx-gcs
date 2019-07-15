#ifndef KMLPOLYGONSMODEL_H
#define KMLPOLYGONSMODEL_H

#include <QAbstractListModel>
#include <QGeoPolygon>

class KmlPolygonsModel: public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        Polygon = Qt::UserRole + 1
    };
    KmlPolygonsModel();

    void setPolygons(const QList<QGeoPolygon> &polygons);

    int rowCount(const QModelIndex &index) const override;
    int columnCount(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QList<QGeoPolygon> m_polygons;
};

#endif // KMLPOLYGONSMODEL_H
