#pragma once

#include <Fact/Fact.h>

#include "OverlayTileServer.h"
#include "OverlayTileModel.h"

#include <QGeoCoordinate>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QtCore>

class QNetworkReply;

class Overlay : public Fact
{
    Q_OBJECT

    Q_PROPERTY(QString promptText READ promptText WRITE setPromptText NOTIFY promptTextChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString lastAiText READ lastAiText NOTIFY lastAiTextChanged)
    Q_PROPERTY(QString lastTelemetryText READ lastTelemetryText NOTIFY lastTelemetryTextChanged)
    Q_PROPERTY(QString answerLogText READ answerLogText NOTIFY answerLogTextChanged)
    Q_PROPERTY(bool telemetryActive READ telemetryActive NOTIFY telemetryActiveChanged)
    Q_PROPERTY(QAbstractListModel *overlayTileModel READ overlayTileModel CONSTANT)

    Q_PROPERTY(QString overlayTileUrlTemplate READ overlayTileUrlTemplate NOTIFY overlayTileServerChanged)
    Q_PROPERTY(bool overlayTileServerReady READ overlayTileServerReady NOTIFY overlayTileServerChanged)

public:
    explicit Overlay(Fact *parent = nullptr);
    ~Overlay() override = default;

    QString promptText() const { return _promptText; }
    QString statusText() const { return _statusText; }
    QString lastAiText() const { return _lastAiText; }
    QString lastTelemetryText() const { return _lastTelemetryText; }
    QString answerLogText() const { return _answerLogText; }
    bool telemetryActive() const { return _active; }

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

    Q_INVOKABLE void setPromptText(const QString &value);
    Q_INVOKABLE void startMonitoring();
    Q_INVOKABLE void stopMonitoring();
    Q_INVOKABLE void toggleMonitoring();

signals:
    void promptTextChanged();
    void statusTextChanged();
    void lastAiTextChanged();
    void lastTelemetryTextChanged();
    void answerLogTextChanged();
    void telemetryActiveChanged();

    void overlayTileServerChanged();

private slots:
    void telemetryPulse();
    void handleTelemetryReply(QNetworkReply *reply);

private:
    QString pluginsDir() const;
    QString uiDir() const;

    void postToGcsConsole(const QString &text);
    void appendLog(const QString &text);

    void setStatusTextValue(const QString &value);
    void setLastAiTextValue(const QString &value);
    void setLastTelemetryTextValue(const QString &value);

    QString tagValue(const QString &xml, const QString &tag) const;
    QString tagAny(const QString &xml, const QStringList &tags) const;

    bool readDouble(const QString &text, double *value) const;

    QString sourceText(
        double energy,
        bool inFlight,
        double altitude
    ) const;

    bool shouldUseSample(
        double lat,
        double lon,
        double altitude,
        double airspeed,
        const QString &source
    ) const;

    double footprintMeters(double altitude) const;

    bool shouldDrawFootprint(
        double lat,
        double lon,
        double altitude,
        double airspeed
    ) const;

    QTimer _timer;
    QNetworkAccessManager _network;

    OverlayTileModel _overlayTileModel;
    OverlayTileServer _tileServer;

    QGeoCoordinate _lastDrawCoord;
    bool _hasLastDrawCoord = false;

    bool _active = false;
    bool _busy = false;
    int _sample = 0;

    QString _promptText = "est.pos.vspeed - est.air.vse";
    QString _statusText = "Ready";
    QString _lastAiText = "Energy overlay ready";
    QString _lastTelemetryText;
    QString _answerLogText;
};
