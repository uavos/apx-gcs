#ifndef KMLPOLYGONSMODEL_H
#define KMLPOLYGONSMODEL_H

#include "kmlparser.h"
#include <QAbstractListModel>
#include <QPolygonF>

class KmlPolygonsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles { Polygon = Qt::UserRole + 1, Color };
    KmlPolygonsModel();

    QPointF setPolygons(const QList<KmlPolygon> &kmlPolygons);
    void setBoundingBox(const QRectF &bb);

    int rowCount(const QModelIndex &index) const override;
    int columnCount(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    struct KmlPolygonExtended
    {
        KmlPolygon kmlPolygon;
        QPolygonF polygon;
        bool operator==(const KmlPolygonExtended &other)
        {
            return kmlPolygon.id == other.kmlPolygon.id;
        }
    };

    QPolygonF m_bb;
    QList<KmlPolygonExtended> m_allPolygons;
    QList<KmlPolygonExtended> m_viewPolygons;

    void updateViewPolygons();

    QPolygonF toPolygon(const QGeoPolygon &geoPolygon);
};

#endif // KMLPOLYGONSMODEL_H
