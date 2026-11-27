#pragma once

#include <Fact/Fact.h>

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QGeoPolygon>
#include <QtCore>

#include "NavaiMapGrid.h"

// The plan of a map to build on board: a zone drawn on the map and the
// settings of navai's map downloader. Nothing is downloaded here; the plan is
// saved to a JSON file with every tile centre, to be taken to the board.
class NavaiMapPlan : public Fact
{
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel *pointsModel READ pointsModel CONSTANT)
    Q_PROPERTY(QGeoPolygon polygon READ polygon NOTIFY polygonChanged)
    Q_PROPERTY(QGeoCoordinate labelCoordinate READ labelCoordinate NOTIFY polygonChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY summaryChanged)

public:
    explicit NavaiMapPlan(Fact *parent);

    Fact *f_scales;
    Fact *f_camera;
    Fact *f_fov;
    Fact *f_overlap;
    Fact *f_points;

    Fact *f_zone;
    Fact *f_remove;
    Fact *f_save;

    Q_INVOKABLE void setZone(const QVariantList &coordinates);
    Q_INVOKABLE void addPoint(QGeoCoordinate c, int n = -1);
    Q_INVOKABLE void removePoint(int n);

    QAbstractListModel *pointsModel() const { return m_pointsModel; }
    QGeoPolygon polygon() const { return m_polygon; }
    // Lower right corner of the zone, the text is written under it.
    QGeoCoordinate labelCoordinate() const;
    QString summary() const { return m_summary; }

signals:
    void zoneRequested();
    void polygonChanged();
    void summaryChanged();

private slots:
    void updateCamera();
    void updatePolygon();
    void updatePlan();
    void save();

private:
    QJsonObject toPlanJson() const;

    QAbstractListModel *m_pointsModel;
    QGeoPolygon m_polygon;
    QString m_summary;

    QVector<NavaiScalePlan> _scales;
    qint64 _tilesCount = 0;
};
