#ifndef KMLOVERLAY_H
#define KMLOVERLAY_H

#include "kmlpolygonsmodel.h"
#include <Fact/Fact.h>
#include <QtCore>
#include <QtLocation>

class KmlOverlay : public Fact
{
    Q_OBJECT
    Q_PROPERTY(KmlPolygonsModel *kmlPolygons READ getKmlPolygons CONSTANT);
    Q_PROPERTY(QGeoCoordinate center READ getCenter NOTIFY centerChanged)
public:
    explicit KmlOverlay(Fact *parent = nullptr);

    Fact *f_open;
    Fact *f_visible;
    Fact *f_opacity;

    KmlPolygonsModel *getKmlPolygons() const;
    QGeoCoordinate getCenter() const;

    Q_INVOKABLE void updateKmlModels(const QGeoShape &shape);

private:
    KmlParser m_parser;
    QGeoCoordinate m_center;
    KmlPolygonsModel *m_kmlPolygons;
    QPointF gc2p(const QGeoCoordinate &c);
    QGeoCoordinate p2gc(const QPointF &p);

private slots:
    void onOpenTriggered();
    void onOverlayVisibleValueChanged();

signals:
    void centerChanged();
};

#endif //KMLOVERLAY_H
