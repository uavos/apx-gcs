#ifndef KMLPOLYGONSMODEL_H
#define KMLPOLYGONSMODEL_H

#include <QAbstractListModel>
#include <QPolygonF>
#include "kmlparser.h"

class KmlPolygonsModel: public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        Polygon = Qt::UserRole + 1,
        Color
    };
    KmlPolygonsModel();

    QPointF setPolygons(const QList<KmlPolygon> &polygons);
    void setBoundingBox(const QRectF &bb);

    int rowCount(const QModelIndex &index) const override;
    int columnCount(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QPolygonF m_bb;
    QList<KmlPolygon> m_allPolygons;
    QList<KmlPolygon> m_viewPolygons;

    void updateViewPolygons();
};

#endif // KMLPOLYGONSMODEL_H
