#pragma once

#include <Fact/Fact.h>

#include "OverlayTileModel.h"
#include "OverlayTileServer.h"

#include <QGeoCoordinate>
#include <QTimer>
#include <QtCore>

class Unit;

class Overlay : public Fact
{
    Q_OBJECT

    Q_PROPERTY(bool telemetryActive READ telemetryActive NOTIFY telemetryActiveChanged)
    Q_PROPERTY(QAbstractListModel *overlayTileModel READ overlayTileModel CONSTANT)

    Q_PROPERTY(QString overlayTileUrlTemplate READ overlayTileUrlTemplate NOTIFY overlayTileServerChanged)
    Q_PROPERTY(bool overlayTileServerReady READ overlayTileServerReady NOTIFY overlayTileServerChanged)

public:
    explicit Overlay(Fact *parent = nullptr);
    ~Overlay() override = default;

    bool telemetryActive() const
    {
        return _active;
    }

    QAbstractListModel *overlayTileModel()
    {
        return &_overlayTileModel;
    }

    QString overlayTileUrlTemplate() const
    {
        return _tileServer.urlTemplate();
    }

    bool overlayTileServerReady() const
    {
        return _tileServer.ready();
    }

    Q_INVOKABLE void startMonitoring();
    Q_INVOKABLE void stopMonitoring();
    Q_INVOKABLE void toggleMonitoring();

signals:
    void telemetryActiveChanged();
    void overlayTileServerChanged();

private slots:
    void telemetryPulse();
    void fadePulse();

private:
    QString uiDir() const;

    void bindUnit(Unit *unit);

    bool readFactDouble(
        Fact *fact,
        double *value
    ) const;

    void postToGcsConsole(const QString &text);

    bool shouldUseSample(
        double lat,
        double lon,
        double altitude,
        double airspeed
    ) const;

    bool shouldDrawFootprint(
        double lat,
        double lon,
        double altitude,
        double airspeed
    ) const;

    double readMaxValue() const;

    QTimer _timer;
    QTimer _fadeTimer;

    OverlayTileModel _overlayTileModel;
    OverlayTileServer _tileServer;

    Fact *_maxFact = nullptr;

    Fact *_vspeedFact = nullptr;
    Fact *_vseFact = nullptr;
    Fact *_airspeedFact = nullptr;
    Fact *_altitudeFact = nullptr;
    Fact *_latFact = nullptr;
    Fact *_lonFact = nullptr;

    QGeoCoordinate _lastDrawCoord;
    bool _hasLastDrawCoord = false;

    double _lastDrawValue = 0.0;

    bool _active = false;
    int _sample = 0;
};
