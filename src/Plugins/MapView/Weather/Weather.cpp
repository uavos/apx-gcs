/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2026, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "Weather.h"

#include <App/App.h>
#include <Fleet/Fleet.h>
#include <Fleet/Unit.h>
#include <Mission/MissionGroup.h>
#include <Mission/UnitMission.h>
#include <Mission/Waypoint.h>

#include <cmath>

static constexpr double route_step_m = 100;      // route points spacing
static constexpr double forecast_grid_m = 10000; // forecast points spacing along the route
static constexpr int nodes_max = 60;             // locations per request
static constexpr int rebuild_delay_ms = 500;
static constexpr int forecast_period_ms = 30 * 60 * 1000;
static constexpr int forecast_retry_ms = 60 * 1000;
static constexpr int forecast_days = 2;

// Open-Meteo forecast, several locations per request, no key
static const char *api_url = "https://api.open-meteo.com/v1/forecast";
// pressure levels with wind and geopotential height, surface to ~5.5 km
static const int levels_hpa[] = {1000, 975, 950, 925, 900, 850, 800, 700, 600, 500};
// near surface winds, height above the model terrain
static const int levels_agl[] = {10, 80, 120, 180};

// ForecastNode

int ForecastNode::hourIndex(qint64 epoch) const
{
    if (hours.isEmpty())
        return -1;
    qint64 i = (epoch - hours.first()) / 3600;
    return int(qBound<qint64>(0, i, hours.size() - 1));
}

void ForecastNode::windAt(int hour, double altMsl, double *u, double *v) const
{
    *u = *v = 0;
    if (hour < 0 || hour >= profile.size())
        return;
    const auto &p = profile.at(hour);
    if (p.isEmpty())
        return;
    if (altMsl <= p.first().h) {
        *u = p.first().u;
        *v = p.first().v;
        return;
    }
    for (int i = 1; i < p.size(); ++i) {
        const auto &a = p.at(i - 1);
        const auto &b = p.at(i);
        if (altMsl > b.h)
            continue;
        double k = (b.h > a.h) ? (altMsl - a.h) / (b.h - a.h) : 0;
        *u = a.u + (b.u - a.u) * k;
        *v = a.v + (b.v - a.v) * k;
        return;
    }
    *u = p.last().u;
    *v = p.last().v;
}

// Weather

Weather::Weather(Fact *parent)
    : Fact(parent, QString(PLUGIN_NAME).toLower(), tr("Weather"), tr("Wind along the route"), Group)
{
    setIcon("weather-windy");

    f_enabled = new Fact(this,
                         "use",
                         tr("Enabled"),
                         tr("Request the wind forecast along the mission route"),
                         Bool | PersistentValue,
                         "power");
    f_enabled->setDefaultValue(false);

    _points = new WeatherModel(this);

    _rebuildTimer.setSingleShot(true);
    _rebuildTimer.setInterval(rebuild_delay_ms);
    connect(&_rebuildTimer, &QTimer::timeout, this, &Weather::rebuild);

    _refreshTimer.setInterval(forecast_period_ms);
    connect(&_refreshTimer, &QTimer::timeout, this, &Weather::refresh);

    connect(f_enabled, &Fact::valueChanged, this, &Weather::onEnabledChanged);

    connect(Fleet::instance(), &Fleet::currentChanged, this, &Weather::bindUnit);
    connect(App::instance(), &App::loadingFinished, this, [this]() {
        bindUnit();
        onEnabledChanged();
    });

    loadQml("qrc:/" PLUGIN_NAME "/WeatherMapLayer.qml"); // route points on the map
}

void Weather::setStatus(const QString &status)
{
    if (_status == status)
        return;
    _status = status;
    setDescr(_status.isEmpty() ? tr("Wind along the route") : _status);
    emit statusChanged();
}

void Weather::setSelected(int row)
{
    if (_selected == row)
        return;
    _selected = row;
    emit selectedChanged();
}

int Weather::stride() const
{
    return _points->stride();
}

void Weather::setStride(int v)
{
    if (_points->stride() == qMax(1, v))
        return;
    setSelected(-1);
    _points->setStride(v);
    emit strideChanged();
}

void Weather::onEnabledChanged()
{
    bool v = f_enabled->value().toBool();
    setActive(v);
    if (v) {
        _refreshTimer.start();
        scheduleRebuild();
        return;
    }
    _refreshTimer.stop();
    _rebuildTimer.stop();
    if (_reply)
        _reply->abort();
    clear();
    setStatus({});
}

void Weather::clear()
{
    _legs.clear();
    _points->reset({});
    setSelected(-1);
}

// mission reference altitude (home) of the current unit
Fact *Weather::refFact() const
{
    if (!_unit || !_unit->f_mandala)
        return nullptr;
    return _unit->f_mandala->fact(mandala::est::nav::ref::hmsl::uid);
}

// the mission of the current unit: rebuild on edits
void Weather::bindUnit()
{
    if (_watcher) {
        delete _watcher;
        _watcher = nullptr;
    }
    _unit = Fleet::instance()->current();
    if (!_unit || !_unit->f_mission) {
        scheduleRebuild();
        return;
    }
    _watcher = new QObject(this);
    auto wps = _unit->f_mission->f_wp;
    connect(wps, &MissionGroup::distanceChanged, _watcher, [this]() { scheduleRebuild(); });
    // waypoints added or removed: connect to the new ones
    // (queued: bindUnit deletes the watcher, not from its own slot)
    connect(wps, &Fact::sizeChanged, _watcher, [this]() {
        QMetaObject::invokeMethod(this, &Weather::bindUnit, Qt::QueuedConnection);
    });
    for (int i = 0; i < wps->size(); ++i) {
        auto wp = qobject_cast<Waypoint *>(wps->child(i));
        if (!wp)
            continue;
        for (auto f : {wp->f_altitude, wp->f_amsl, wp->f_atrack})
            connect(f, &Fact::valueChanged, _watcher, [this]() { scheduleRebuild(); });
    }
    if (auto ref = refFact())
        connect(ref, &Fact::valueChanged, _watcher, [this]() { scheduleRebuild(); });
    scheduleRebuild();
}

void Weather::scheduleRebuild()
{
    if (!f_enabled->value().toBool())
        return;
    _rebuildTimer.start();
}

// legs from the mission waypoints: path and altitude
bool Weather::buildRoute()
{
    _legs.clear();
    _refValid = false;
    _refHmsl = 0;
    if (!_unit || !_unit->f_mission)
        return false;
    if (auto ref = refFact()) {
        _refHmsl = ref->value().toDouble();
        _refValid = std::isfinite(_refHmsl) && _refHmsl != 0;
    }
    auto wps = _unit->f_mission->f_wp;
    for (int i = 0; i < wps->size(); ++i) {
        auto wp = qobject_cast<Waypoint *>(wps->child(i));
        if (!wp)
            continue;
        Leg leg;
        leg.path = wp->geoPath(); // from the previous waypoint (or the start point) to this one
        leg.alt = wp->f_altitude->value().toDouble();
        leg.amsl = wp->f_amsl->value().toBool();
        leg.atrack = wp->f_atrack->value().toBool();
        _legs.append(leg);
    }
    return !_legs.isEmpty();
}

// forecast points every grid along the route and the route end
QList<QGeoCoordinate> Weather::forecastNodes() const
{
    QList<QGeoCoordinate> nodes;
    double total = 0;
    for (const auto &leg : _legs) {
        const auto &p = leg.path.path();
        for (int i = 1; i < p.size(); ++i)
            total += p.at(i - 1).distanceTo(p.at(i));
    }
    double grid = forecast_grid_m;
    if (total / grid > nodes_max - 1)
        grid = total / (nodes_max - 1);

    double dist = 0;
    double next = 0;
    QGeoCoordinate last;
    for (const auto &leg : _legs) {
        const auto &p = leg.path.path();
        for (int i = 1; i < p.size(); ++i) {
            const auto &a = p.at(i - 1);
            const auto &b = p.at(i);
            double len = a.distanceTo(b);
            double az = a.azimuthTo(b);
            while (next <= dist + len) {
                nodes.append(a.atDistanceAndAzimuth(next - dist, az));
                next += grid;
            }
            dist += len;
            last = b;
        }
    }
    if (last.isValid() && (nodes.isEmpty() || nodes.last().distanceTo(last) > grid / 4))
        nodes.append(last);
    if (nodes.isEmpty()) {
        // no path (a single waypoint without the start point): the waypoints themselves
        for (const auto &leg : _legs) {
            if (!leg.path.path().isEmpty())
                nodes.append(leg.path.path().last());
        }
    }
    return nodes;
}

// the loaded forecast still covers the route: every node is close to a loaded one
bool Weather::nodesCover(const QList<QGeoCoordinate> &nodes) const
{
    if (_nodes.isEmpty())
        return false;
    for (const auto &n : nodes) {
        double best = 1e12;
        for (const auto &f : _nodes)
            best = qMin(best, f.coordinate.distanceTo(n));
        if (best > forecast_grid_m * 0.75)
            return false;
    }
    return true;
}

void Weather::rebuild()
{
    if (!f_enabled->value().toBool())
        return;
    if (!buildRoute()) {
        clear();
        setStatus(_unit ? tr("No mission") : tr("No unit"));
        return;
    }
    auto nodes = forecastNodes();
    auto now = QDateTime::currentMSecsSinceEpoch();
    bool expired = now - _forecastTimeMs > forecast_period_ms;
    if (!_reply && (expired || !nodesCover(nodes)) && now >= _retryMs)
        requestForecast(nodes);
    compute();
}

void Weather::refresh()
{
    if (!f_enabled->value().toBool())
        return;
    _forecastTimeMs = 0;
    _retryMs = 0;
    rebuild();
}

void Weather::requestForecast(const QList<QGeoCoordinate> &nodes)
{
    if (nodes.isEmpty())
        return;
    QStringList lat, lon;
    for (const auto &c : nodes) {
        lat.append(QString::number(c.latitude(), 'f', 4));
        lon.append(QString::number(c.longitude(), 'f', 4));
    }
    QStringList vars;
    for (int h : levels_agl)
        vars << QString("wind_speed_%1m").arg(h) << QString("wind_direction_%1m").arg(h);
    for (int p : levels_hpa)
        vars << QString("wind_speed_%1hPa").arg(p) << QString("wind_direction_%1hPa").arg(p)
             << QString("geopotential_height_%1hPa").arg(p);

    QUrlQuery q;
    q.addQueryItem("latitude", lat.join(','));
    q.addQueryItem("longitude", lon.join(','));
    q.addQueryItem("hourly", vars.join(','));
    q.addQueryItem("wind_speed_unit", "ms");
    q.addQueryItem("forecast_days", QString::number(forecast_days));
    q.addQueryItem("timezone", "UTC");
    QUrl url(api_url);
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "apx-gcs");
    req.setTransferTimeout(30000);
    _requested = nodes;
    _reply = _net.get(req);
    connect(_reply, &QNetworkReply::finished, this, &Weather::onReply);
    setProgress(0);
}

static QList<double> jsonDoubles(const QJsonObject &hourly, const QString &key, int n)
{
    QList<double> list;
    auto arr = hourly.value(key).toArray();
    for (int i = 0; i < n; ++i) {
        auto v = arr.at(i);
        list.append(v.isDouble() ? v.toDouble() : qQNaN());
    }
    return list;
}

void Weather::onReply()
{
    auto reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    reply->deleteLater();
    setProgress(-1);

    auto fail = [this](const QString &error) {
        _error = error;
        _retryMs = QDateTime::currentMSecsSinceEpoch() + forecast_retry_ms;
        compute(); // the route without the forecast (or with the previous one)
        QTimer::singleShot(forecast_retry_ms, this, &Weather::scheduleRebuild);
    };

    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() == QNetworkReply::OperationCanceledError && !f_enabled->value().toBool())
            return;
        int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        fail(code ? tr("Forecast: HTTP %1").arg(code)
                  : tr("Forecast: %1").arg(reply->errorString().section(" - ", -1)));
        return;
    }

    auto doc = QJsonDocument::fromJson(reply->readAll());
    QJsonArray results;
    if (doc.isArray())
        results = doc.array();
    else
        results.append(doc.object());

    QList<ForecastNode> nodes;
    for (int ni = 0; ni < results.size(); ++ni) {
        auto r = results.at(ni).toObject();
        if (r.contains("error")) {
            fail(tr("Forecast: %1").arg(r.value("reason").toString()));
            return;
        }
        ForecastNode n;
        n.coordinate = ni < _requested.size()
                           ? _requested.at(ni)
                           : QGeoCoordinate(r.value("latitude").toDouble(), r.value("longitude").toDouble());
        n.elevation = r.value("elevation").toDouble();
        auto hourly = r.value("hourly").toObject();
        auto times = hourly.value("time").toArray();
        int cnt = times.size();
        for (const auto &t : times) {
            auto dt = QDateTime::fromString(t.toString(), Qt::ISODate);
            dt.setTimeZone(QTimeZone::utc());
            n.hours.append(dt.toSecsSinceEpoch());
        }

        // wind profile per hour
        struct Src
        {
            QList<double> h, s, d;
        };
        QList<Src> src;
        for (int h : levels_agl) {
            Src s;
            s.s = jsonDoubles(hourly, QString("wind_speed_%1m").arg(h), cnt);
            s.d = jsonDoubles(hourly, QString("wind_direction_%1m").arg(h), cnt);
            s.h = QList<double>(cnt, n.elevation + h);
            src.append(s);
        }
        for (int p : levels_hpa) {
            Src s;
            s.s = jsonDoubles(hourly, QString("wind_speed_%1hPa").arg(p), cnt);
            s.d = jsonDoubles(hourly, QString("wind_direction_%1hPa").arg(p), cnt);
            s.h = jsonDoubles(hourly, QString("geopotential_height_%1hPa").arg(p), cnt);
            src.append(s);
        }
        for (int hi = 0; hi < cnt; ++hi) {
            QList<ForecastNode::Level> levels;
            for (const auto &s : src) {
                double h = s.h.at(hi), sp = s.s.at(hi), dir = s.d.at(hi);
                if (qIsNaN(h) || qIsNaN(sp) || qIsNaN(dir))
                    continue;
                double rad = qDegreesToRadians(dir);
                levels.append({h, -sp * std::sin(rad), -sp * std::cos(rad)});
            }
            std::sort(levels.begin(), levels.end(), [](const auto &a, const auto &b) {
                return a.h < b.h;
            });
            n.profile.append(levels);
        }
        if (!n.hours.isEmpty())
            nodes.append(n);
    }
    if (nodes.isEmpty()) {
        fail(tr("Forecast: empty reply"));
        return;
    }
    _nodes = nodes;
    _forecastTimeMs = QDateTime::currentMSecsSinceEpoch();
    _error.clear();
    compute();
    // the route moved while the request was in progress
    if (!nodesCover(forecastNodes()))
        scheduleRebuild();
}

bool Weather::windAt(const QGeoCoordinate &c, double altMsl, qint64 epoch, double *u, double *v, QString *time) const
{
    *u = *v = 0;
    if (_nodes.isEmpty())
        return false;
    // two nearest nodes
    int i1 = -1, i2 = -1;
    double d1 = 1e12, d2 = 1e12;
    for (int i = 0; i < _nodes.size(); ++i) {
        double d = _nodes.at(i).coordinate.distanceTo(c);
        if (d < d1) {
            i2 = i1;
            d2 = d1;
            i1 = i;
            d1 = d;
        } else if (d < d2) {
            i2 = i;
            d2 = d;
        }
    }
    const auto &n1 = _nodes.at(i1);
    int h1 = n1.hourIndex(epoch);
    if (h1 < 0)
        return false;
    *time = QDateTime::fromSecsSinceEpoch(n1.hours.at(h1), QTimeZone::utc()).toString("dd.MM HH:mm");
    double u1, v1;
    n1.windAt(h1, altMsl, &u1, &v1);
    if (i2 < 0 || d1 < 1) {
        *u = u1;
        *v = v1;
        return true;
    }
    const auto &n2 = _nodes.at(i2);
    double u2, v2;
    n2.windAt(n2.hourIndex(epoch), altMsl, &u2, &v2);
    // inverse distance weights
    double w1 = 1 / d1, w2 = 1 / d2;
    *u = (u1 * w1 + u2 * w2) / (w1 + w2);
    *v = (v1 * w1 + v2 * w2) / (w1 + w2);
    return true;
}

// route points every step and on the waypoints: planned altitude and the wind on it
void Weather::compute()
{
    if (_legs.isEmpty()) {
        _points->reset({});
        return;
    }
    // altitude reference for AGL waypoints: home, or the model terrain at the start
    const double ref = _refValid ? _refHmsl : (_nodes.isEmpty() ? 0 : _nodes.first().elevation);
    const qint64 now = QDateTime::currentSecsSinceEpoch();

    QList<RoutePoint> out;
    double windMin = 1e9, windMax = 0;
    auto addPoint = [&](const QGeoCoordinate &c, int leg, bool waypoint, double dist, double track, double altMsl) {
        RoutePoint rp;
        rp.coordinate = c;
        rp.leg = leg;
        rp.waypoint = waypoint;
        rp.dist = dist;
        rp.track = track;
        rp.altMsl = altMsl;
        rp.altAgl = altMsl - ref;
        double u, v;
        if (windAt(c, altMsl, now, &u, &v, &rp.time)) {
            rp.windSpeed = std::hypot(u, v);
            rp.windDir = std::fmod(qRadiansToDegrees(std::atan2(-u, -v)) + 360.0, 360.0);
            windMin = qMin(windMin, rp.windSpeed);
            windMax = qMax(windMax, rp.windSpeed);
        }
        out.append(rp);
    };

    double dist = 0;
    double next = 0;
    double altPrev = ref; // the start point is on the ground
    for (int li = 0; li < _legs.size(); ++li) {
        const auto &leg = _legs.at(li);
        const auto &p = leg.path.path();
        const double alt = leg.amsl ? leg.alt : ref + leg.alt;
        if (p.size() < 2) {
            // no path to the waypoint: the route starts here
            if (!p.isEmpty())
                addPoint(p.last(), li, true, dist, 0, alt);
            altPrev = alt;
            continue;
        }
        double legLen = 0;
        for (int i = 1; i < p.size(); ++i)
            legLen += p.at(i - 1).distanceTo(p.at(i));
        double legPos = 0;
        // the autopilot commands the waypoint altitude at the leg start,
        // with the altitude tracking it changes linearly along the leg
        auto altAt = [&](double pos) {
            return (leg.atrack && legLen > 0) ? altPrev + (alt - altPrev) * pos / legLen : alt;
        };
        double az = 0;
        for (int i = 1; i < p.size(); ++i) {
            const auto &a = p.at(i - 1);
            const auto &b = p.at(i);
            double len = a.distanceTo(b);
            az = a.azimuthTo(b);
            while (next < dist + len) {
                double s = next - dist;
                addPoint(a.atDistanceAndAzimuth(s, az), li, false, next, az, altAt(legPos + s));
                next += route_step_m;
            }
            dist += len;
            legPos += len;
        }
        // the waypoint itself, with the course of the incoming segment
        addPoint(p.last(), li, true, dist, az, alt);
        if (next < dist + route_step_m / 2)
            next = dist + route_step_m; // no route point right next to the waypoint
        altPrev = alt;
    }
    setSelected(-1);
    _points->reset(out);

    QString status = _error;
    if (status.isEmpty()) {
        if (_nodes.isEmpty())
            status = tr("Route %1 km, no forecast").arg(dist / 1000, 0, 'f', 1);
        else
            status = tr("Route %1 km, wind %2-%3 m/s, forecast %4 UTC")
                         .arg(dist / 1000, 0, 'f', 1)
                         .arg(windMin, 0, 'f', 0)
                         .arg(windMax, 0, 'f', 0)
                         .arg(QDateTime::fromMSecsSinceEpoch(_forecastTimeMs, QTimeZone::utc()).toString("HH:mm"));
    }
    setStatus(status);
}
