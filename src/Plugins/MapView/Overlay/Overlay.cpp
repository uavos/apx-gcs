#include "Overlay.h"

#include <App/AppNotify.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>

static constexpr double ENERGY_THRESHOLD = 0.5;
static constexpr double FLIGHT_AIRSPEED_THRESHOLD = 5.0;
static constexpr double MIN_ALTITUDE_FOR_TILES = 30.0;

Overlay::Overlay(Fact *parent)
    : Fact(parent,
           "overlay",
           tr("Energy Monitor"),
           tr("APX energy overlay monitor"),
           Group,
           "chart-line")
{
    const QString pagePath =
        uiDir() + "/AiVoice.qml";

    const QString mapPluginPath =
        uiDir() + "/OverlayMapPlugin.qml";

    if (QFileInfo::exists(pagePath))
        setOpt("page", QUrl::fromLocalFile(pagePath).toString());

    if (QFileInfo::exists(mapPluginPath))
        loadQml(mapPluginPath);

    _timer.setInterval(300);

    connect(
        &_timer,
        &QTimer::timeout,
        this,
        &Overlay::telemetryPulse
    );

    connect(
        &_network,
        &QNetworkAccessManager::finished,
        this,
        &Overlay::handleTelemetryReply
    );

    connect(
        &_tileServer,
        &OverlayTileServer::changed,
        this,
        &Overlay::overlayTileServerChanged
    );

    _tileServer.setZoomRange(12, 16);

    if (_tileServer.startServer(9292)) {
        postToGcsConsole(
            "Energy tile server started: " +
            _tileServer.urlTemplate()
        );
    } else {
        postToGcsConsole(
            "Energy tile server failed to start"
        );
    }

    postToGcsConsole(
        "Energy overlay ready: transparent map tile layer"
    );

    startMonitoring();
}

QString Overlay::pluginsDir() const
{
    const QString docs =
        QStandardPaths::writableLocation(
            QStandardPaths::DocumentsLocation
        );

    return docs + "/UAVOS/Plugins";
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

void Overlay::setPromptText(const QString &value)
{
    if (_promptText == value)
        return;

    _promptText = value;
    emit promptTextChanged();
}

void Overlay::startMonitoring()
{
    if (_active)
        return;

    _active = true;
    _busy = false;
    _sample = 0;

    _hasLastDrawCoord = false;

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

    setStatusTextValue("Monitoring adaptive");
    setLastAiTextValue("Overlay monitor started.");
    appendLog("Overlay monitor started.");
    postToGcsConsole("Overlay monitor started.");

    emit telemetryActiveChanged();
}

void Overlay::stopMonitoring()
{
    if (!_active)
        return;

    _active = false;
    _busy = false;

    _timer.stop();

    _overlayTileModel.flush();

    setStatusTextValue("Stopped");
    setLastAiTextValue("Overlay monitor stopped.");
    appendLog("Overlay monitor stopped.");
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
    if (!_active || _busy)
        return;

    _busy = true;

    QNetworkRequest request(
        QUrl("http://127.0.0.1:9280/mandala")
    );

    _network.get(request);
}

void Overlay::handleTelemetryReply(QNetworkReply *reply)
{
    _busy = false;

    if (!_active) {
        reply->deleteLater();
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        const QString errorText =
            QString("mandala request error: %1")
                .arg(reply->errorString());

        reply->deleteLater();

        setStatusTextValue("Mandala error");
        setLastAiTextValue(errorText);
        setLastTelemetryTextValue(errorText);

        if (_sample % 5 == 0) {
            appendLog(errorText);
            postToGcsConsole(errorText);
        }

        return;
    }

    const QString xml =
        QString::fromUtf8(reply->readAll());

    reply->deleteLater();

    ++_sample;

    const QString time =
        QDateTime::currentDateTimeUtc()
            .toString(Qt::ISODate);

    const QString vspeedText = tagAny(xml, {
        "est.pos.vspeed",
        "est.vel.d",
        "est.vspeed",
        "est.climb",
        "est.vario"
    });

    const QString vseText = tagAny(xml, {
        "est.air.vse"
    });

    const QString airspeedText = tagAny(xml, {
        "est.air.airspeed",
        "est.air.speed",
        "est.airspeed",
        "est.cas",
        "est.tas",
        "est.vair"
    });

    const QString altitudeText = tagAny(xml, {
        "est.pos.altitude",
        "est.pos.alt",
        "est.pos.hmsl",
        "est.hmsl",
        "est.alt",
        "est.height",
        "est.pos.h"
    });

    const QString latText = tagAny(xml, {
        "est.pos.lat",
        "est.pos.latitude",
        "est.lat",
        "gps.lat"
    });

    const QString lonText = tagAny(xml, {
        "est.pos.lon",
        "est.pos.longitude",
        "est.lon",
        "gps.lon"
    });

    double vspeed = 0.0;
    double vse = 0.0;
    double airspeed = 0.0;
    double altitude = 0.0;
    double lat = 0.0;
    double lon = 0.0;

    const bool okVspeed =
        readDouble(vspeedText, &vspeed);

    const bool okVse =
        readDouble(vseText, &vse);

    const bool okAirspeed =
        readDouble(airspeedText, &airspeed);

    const bool okAltitude =
        readDouble(altitudeText, &altitude);

    const bool okLat =
        readDouble(latText, &lat);

    const bool okLon =
        readDouble(lonText, &lon);

    if (!okVspeed || !okVse) {
        if (_sample % 5 == 0) {
            const QString line =
                QString("sample=%1 time=%2 source=NO_VALUES "
                        "est.pos.vspeed=%3 est.air.vse=%4")
                    .arg(_sample)
                    .arg(time)
                    .arg(vspeedText.isEmpty() ? "n/a" : vspeedText)
                    .arg(vseText.isEmpty() ? "n/a" : vseText);

            setStatusTextValue("No values");
            setLastAiTextValue(line);
            setLastTelemetryTextValue(line);
            appendLog(line);
            postToGcsConsole(line);
        }

        return;
    }

    const bool inFlight =
        okAirspeed
            ? airspeed >= FLIGHT_AIRSPEED_THRESHOLD
            : true;

    const double safeAltitude =
        okAltitude
            ? altitude
            : 0.0;

    const double energy =
        vspeed - vse;

    const QString source =
        sourceText(
            energy,
            inFlight,
            safeAltitude
        );

    if (okLat &&
        okLon &&
        okAltitude &&
        okAirspeed &&
        shouldUseSample(
            lat,
            lon,
            altitude,
            airspeed,
            source
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
                    source
                );
            } else {
                _overlayTileModel.addSample(
                    lat,
                    lon,
                    altitude,
                    source
                );
            }

            _lastDrawCoord =
                QGeoCoordinate(lat, lon);

            _hasLastDrawCoord = true;
        }
    }

    if (_sample % 5 == 0) {
        const QString line =
            QString("sample=%1 time=%2 flight=%3 source=%4 "
                    "energy=%5 vspeed=%6 vse=%7 "
                    "airspeed=%8 altitude=%9 lat=%10 lon=%11")
                .arg(_sample)
                .arg(time)
                .arg(inFlight ? "yes" : "no")
                .arg(source)
                .arg(QString::number(energy, 'f', 2))
                .arg(vspeedText)
                .arg(vseText)
                .arg(airspeedText.isEmpty() ? "n/a" : airspeedText)
                .arg(altitudeText.isEmpty() ? "n/a" : altitudeText)
                .arg(latText.isEmpty() ? "n/a" : latText)
                .arg(lonText.isEmpty() ? "n/a" : lonText);

        setStatusTextValue(source);
        setLastAiTextValue(line);
        setLastTelemetryTextValue(line);
        appendLog(line);
        postToGcsConsole(line);
    }
}

QString Overlay::tagValue(
    const QString &xml,
    const QString &tag) const
{
    const QString open =
        "<" + tag + ">";

    const QString close =
        "</" + tag + ">";

    int a =
        xml.indexOf(open);

    if (a < 0)
        return "";

    a += open.size();

    const int b =
        xml.indexOf(close, a);

    if (b < 0)
        return "";

    return xml.mid(a, b - a).trimmed();
}

QString Overlay::tagAny(
    const QString &xml,
    const QStringList &tags) const
{
    for (const QString &tag : tags) {
        const QString value =
            tagValue(xml, tag);

        if (!value.isEmpty())
            return value;
    }

    return "";
}

bool Overlay::readDouble(
    const QString &text,
    double *value) const
{
    bool ok = false;

    const double v =
        text.trimmed().toDouble(&ok);

    if (!ok)
        return false;

    *value = v;

    return true;
}

QString Overlay::sourceText(
    double energy,
    bool inFlight,
    double altitude) const
{
    if (!inFlight)
        return "GROUND";

    if (altitude < MIN_ALTITUDE_FOR_TILES)
        return "TAKEOFF_RUNWAY";

    if (energy > ENERGY_THRESHOLD)
        return "CHARGE";

    if (energy < -ENERGY_THRESHOLD)
        return "ENGINE";

    return "NEUTRAL";
}

bool Overlay::shouldUseSample(
    double lat,
    double lon,
    double altitude,
    double airspeed,
    const QString &source) const
{
    if (source != "CHARGE" &&
        source != "NEUTRAL" &&
        source != "ENGINE")
        return false;

    if (lat == 0.0 || lon == 0.0)
        return false;

    if (airspeed < FLIGHT_AIRSPEED_THRESHOLD)
        return false;

    if (altitude < MIN_ALTITUDE_FOR_TILES)
        return false;

    return true;
}

double Overlay::footprintMeters(double altitude) const
{
    Q_UNUSED(altitude)

    return 20.0;
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

    const QGeoCoordinate current(lat, lon);

    const double distance =
        _lastDrawCoord.distanceTo(current);

    return distance >= 8.0;
}

void Overlay::setStatusTextValue(const QString &value)
{
    if (_statusText == value)
        return;

    _statusText = value;
    emit statusTextChanged();
}

void Overlay::setLastAiTextValue(const QString &value)
{
    if (_lastAiText == value)
        return;

    _lastAiText = value;
    emit lastAiTextChanged();
}

void Overlay::setLastTelemetryTextValue(const QString &value)
{
    if (_lastTelemetryText == value)
        return;

    _lastTelemetryText = value;
    emit lastTelemetryTextChanged();
}

void Overlay::appendLog(const QString &text)
{
    const QString clean =
        text.trimmed();

    if (clean.isEmpty())
        return;

    const QString line =
        QDateTime::currentDateTime()
            .toString("HH:mm:ss")
        + "  "
        + clean;

    _answerLogText += line + "\n";

    QStringList lines =
        _answerLogText.split(
            '\n',
            Qt::SkipEmptyParts
        );

    const int maxLines = 100;

    if (lines.size() > maxLines) {
        lines =
            lines.mid(lines.size() - maxLines);

        _answerLogText =
            lines.join("\n") + "\n";
    }

    emit answerLogTextChanged();
}

void Overlay::postToGcsConsole(const QString &text)
{
    if (text.trimmed().isEmpty())
        return;

    const QString message =
        QString("[ENERGY-MON] %1").arg(text);

    QMetaObject::invokeMethod(
        QCoreApplication::instance(),
        [message]() {
            AppNotify::instance()->notification(
                message,
                "ENERGY-MON",
                AppNotify::Info,
                nullptr
            );
        },
        Qt::QueuedConnection
    );
}
