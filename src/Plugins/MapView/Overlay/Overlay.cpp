#include "Overlay.h"

#include <App/AppNotify.h>
#include <Fleet/Fleet.h>
#include <Fleet/Unit.h>
#include <Mandala/Mandala.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>

#include <cmath>

static constexpr double FLIGHT_AIRSPEED_THRESHOLD = 5.0;
static constexpr double MIN_ALTITUDE_FOR_TILES = 30.0;
static constexpr double DEFAULT_MAX_VALUE = 5.0;

Overlay::Overlay(Fact *parent)
    : Fact(parent,
           "overlay",
           tr("Overlay"),
           tr("APX telemetry overlay"),
           Group,
           "chart-line")
{
    _maxFact =
        new Fact(
            this,
            "max",
            tr("Max"),
            tr("Maximum value for red lift color"),
            Float,
            "tune"
        );

    _maxFact->setValue(DEFAULT_MAX_VALUE);

    const QString mapPluginPath =
        uiDir() + "/OverlayMapPlugin.qml";

    if (QFileInfo::exists(mapPluginPath))
        loadQml(mapPluginPath);

    _timer.setInterval(200);
    _timer.setTimerType(Qt::CoarseTimer);

    _fadeTimer.setInterval(1000);
    _fadeTimer.setTimerType(Qt::CoarseTimer);

    connect(
        &_timer,
        &QTimer::timeout,
        this,
        &Overlay::telemetryPulse
    );

    connect(
        &_fadeTimer,
        &QTimer::timeout,
        this,
        &Overlay::fadePulse
    );

    connect(
        &_tileServer,
        &OverlayTileServer::changed,
        this,
        &Overlay::overlayTileServerChanged
    );

    auto *fleet =
        Fleet::instance();

    if (fleet) {
        connect(
            fleet,
            &Fleet::unitSelected,
            this,
            [this](Unit *unit) {
                bindUnit(unit);
            }
        );

        bindUnit(fleet->current());
    }

    _tileServer.setZoomRange(12, 16);

    if (_tileServer.startServer(9292)) {
        postToGcsConsole(
            "Overlay tile server started: " +
            _tileServer.urlTemplate()
        );
    } else {
        postToGcsConsole(
            "Overlay tile server failed to start"
        );
    }

    postToGcsConsole(
        "Overlay ready: smooth fading gradient line layer"
    );

    startMonitoring();
}

QString Overlay::uiDir() const
{
    const QString docs =
        QStandardPaths::writableLocation(
            QStandardPaths::DocumentsLocation
        );

    const QStringList candidates = {
        docs + "/UAVOS/Plugins/overlay-ui",
        QCoreApplication::applicationDirPath()
            + "/../share/gcs/plugins/overlay-ui",
        QCoreApplication::applicationDirPath()
            + "/../plugins/overlay-ui",
        QCoreApplication::applicationDirPath()
            + "/overlay-ui"
    };

    for (const QString &path : candidates) {
        if (QDir(path).exists())
            return path;
    }

    return candidates.first();
}

void Overlay::bindUnit(Unit *unit)
{
    _vspeedFact = nullptr;
    _vseFact = nullptr;
    _airspeedFact = nullptr;
    _altitudeFact = nullptr;
    _latFact = nullptr;
    _lonFact = nullptr;

    _hasLastDrawCoord = false;
    _lastDrawValue = 0.0;

    if (!unit || !unit->f_mandala) {
        postToGcsConsole("No current unit mandala");
        return;
    }

    _vspeedFact =
        unit->f_mandala->fact(
            mandala::est::nav::pos::vspeed::uid
        );

    _vseFact =
        unit->f_mandala->fact(
            mandala::est::nav::air::vse::uid
        );

    _airspeedFact =
        unit->f_mandala->fact(
            mandala::est::nav::air::airspeed::uid
        );

    _altitudeFact =
        unit->f_mandala->fact(
            mandala::est::nav::pos::hmsl::uid
        );

    _latFact =
        unit->f_mandala->fact(
            mandala::est::nav::pos::lat::uid
        );

    _lonFact =
        unit->f_mandala->fact(
            mandala::est::nav::pos::lon::uid
        );

    postToGcsConsole("Mandala facts bound");
}

bool Overlay::readFactDouble(
    Fact *fact,
    double *value) const
{
    if (!fact || !value)
        return false;

    bool ok = false;

    double number =
        fact->value().toDouble(&ok);

    if (!ok || !std::isfinite(number)) {
        number =
            fact->valueText().toDouble(&ok);
    }

    if (!ok || !std::isfinite(number))
        return false;

    *value = number;
    return true;
}

double Overlay::readMaxValue() const
{
    double maxValue = DEFAULT_MAX_VALUE;

    if (_maxFact) {
        bool ok = false;

        double v =
            _maxFact->value().toDouble(&ok);

        if (!ok || !std::isfinite(v)) {
            v =
                _maxFact->valueText().toDouble(&ok);
        }

        if (ok && std::isfinite(v) && v > 0.0)
            maxValue = v;
    }

    return maxValue;
}

void Overlay::startMonitoring()
{
    if (_active)
        return;

    _active = true;
    _sample = 0;
    _hasLastDrawCoord = false;
    _lastDrawValue = 0.0;

    _overlayTileModel.startNewSession();

    _tileServer.setSessionDir(
        _overlayTileModel.sessionDirPath()
    );

    _tileServer.setZoomRange(
        _overlayTileModel.minZoomLevel(),
        _overlayTileModel.nativeZoomLevel()
    );

    emit overlayTileServerChanged();

    _timer.start();
    _fadeTimer.start();

    postToGcsConsole("Overlay monitor started.");

    emit telemetryActiveChanged();
}

void Overlay::stopMonitoring()
{
    if (!_active)
        return;

    _active = false;

    _timer.stop();
    _fadeTimer.stop();

    _overlayTileModel.flush();

    postToGcsConsole("Overlay monitor stopped.");

    emit telemetryActiveChanged();
}

void Overlay::toggleMonitoring()
{
    if (_active)
        stopMonitoring();
    else
        startMonitoring();
}

void Overlay::telemetryPulse()
{
    if (!_active)
        return;

    ++_sample;

    if (!_vspeedFact || !_vseFact) {
        auto *fleet =
            Fleet::instance();

        if (fleet)
            bindUnit(fleet->current());
    }

    double vspeed = 0.0;
    double vse = 0.0;
    double airspeed = 0.0;
    double altitude = 0.0;
    double lat = 0.0;
    double lon = 0.0;

    const bool okVspeed =
        readFactDouble(_vspeedFact, &vspeed);

    const bool okVse =
        readFactDouble(_vseFact, &vse);

    const bool okAirspeed =
        readFactDouble(_airspeedFact, &airspeed);

    const bool okAltitude =
        readFactDouble(_altitudeFact, &altitude);

    const bool okLat =
        readFactDouble(_latFact, &lat);

    const bool okLon =
        readFactDouble(_lonFact, &lon);

    if (!okVspeed || !okVse) {
        if (_sample % 10 == 0) {
            postToGcsConsole(
                QString("No energy values: vspeed=%1 vse=%2")
                    .arg(okVspeed ? "ok" : "missing")
                    .arg(okVse ? "ok" : "missing")
            );
        }

        return;
    }

    const bool inFlight =
        okAirspeed
            ? airspeed >= FLIGHT_AIRSPEED_THRESHOLD
            : true;

    const double value =
        vspeed - vse;

    const double maxValue =
        readMaxValue();

    if (okLat &&
        okLon &&
        okAltitude &&
        okAirspeed &&
        inFlight &&
        shouldUseSample(
            lat,
            lon,
            altitude,
            airspeed
        )) {

        if (shouldDrawFootprint(
                lat,
                lon,
                altitude,
                airspeed)) {

            if (_hasLastDrawCoord) {
                _overlayTileModel.addSegment(
                    _lastDrawCoord.latitude(),
                    _lastDrawCoord.longitude(),
                    lat,
                    lon,
                    altitude,
                    _lastDrawValue,
                    value,
                    maxValue
                );
            } else {
                _overlayTileModel.addSample(
                    lat,
                    lon,
                    altitude,
                    value,
                    maxValue
                );
            }

            _lastDrawCoord =
                QGeoCoordinate(lat, lon);

            _lastDrawValue = value;
            _hasLastDrawCoord = true;
        }
    }

    if (_sample % 10 == 0) {
        const QString line =
            QString("sample=%1 value=%2 max=%3 norm=%4 "
                    "vspeed=%5 vse=%6 airspeed=%7 altitude=%8 lat=%9 lon=%10")
                .arg(_sample)
                .arg(QString::number(value, 'f', 2))
                .arg(QString::number(maxValue, 'f', 2))
                .arg(QString::number(qBound(0.0, value / maxValue, 1.0), 'f', 2))
                .arg(vspeed, 0, 'f', 2)
                .arg(vse, 0, 'f', 2)
                .arg(airspeed, 0, 'f', 2)
                .arg(altitude, 0, 'f', 2)
                .arg(lat, 0, 'f', 7)
                .arg(lon, 0, 'f', 7);

        postToGcsConsole(line);
    }
}

void Overlay::fadePulse()
{
    if (!_active)
        return;

    _overlayTileModel.fade();
}

bool Overlay::shouldUseSample(
    double lat,
    double lon,
    double altitude,
    double airspeed) const
{
    if (lat == 0.0 || lon == 0.0)
        return false;

    if (airspeed < FLIGHT_AIRSPEED_THRESHOLD)
        return false;

    if (altitude < MIN_ALTITUDE_FOR_TILES)
        return false;

    return true;
}

bool Overlay::shouldDrawFootprint(
    double lat,
    double lon,
    double altitude,
    double airspeed) const
{
    Q_UNUSED(altitude)
    Q_UNUSED(airspeed)

    if (!_hasLastDrawCoord)
        return true;

    const QGeoCoordinate current(
        lat,
        lon
    );

    const double distance =
        _lastDrawCoord.distanceTo(current);

    return distance >= 8.0;
}

void Overlay::postToGcsConsole(const QString &text)
{
    if (text.trimmed().isEmpty())
        return;

    const QString message =
        QString("[OVERLAY] %1").arg(text);

    QMetaObject::invokeMethod(
        QCoreApplication::instance(),
        [message]() {
            AppNotify::instance()->notification(
                message,
                "OVERLAY",
                AppNotify::Info,
                nullptr
            );
        },
        Qt::QueuedConnection
    );
}
