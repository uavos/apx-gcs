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
#include "Traffic.h"

#include <App/App.h>
#include <Fleet/Fleet.h>
#include <Fleet/Unit.h>
#include <Mandala/Mandala.h>
#include <Mandala/MandalaFact.h>

#include <cmath>

static constexpr int adsb_period_ms = 1000;
static constexpr int adsb_timeout_ms = 5000;      // inactivity, also the offline detection time
static constexpr int adsb_offline_poll_ms = 5000; // request period while offline
static constexpr int adsb_backoff_max_ms = 120000;
static constexpr int adsb_stale_ms = 30000;     // drop aircraft not updated for this long
static constexpr double adsb_seen_pos_max = 60; // skip aircraft with an older position, s
static constexpr int atc_period_ms = 10 * 60 * 1000;
static constexpr int atc_retry_ms = 30 * 1000; // after a failed download (HTTP errors)
static constexpr int atc_timeout_ms = 15000;   // inactivity, a live download keeps resetting it
// alert zones around the unit (TCAS-like): an aircraft is a threat when it is inside the
// zone (within the radius and the altitude band) or will enter it within the time,
// by the relative position and velocity (ground speed and track, vertical speed)
struct AlertZone
{
    double tauS;    // s
    double radiusM; // m
    double dAltM;   // m, the altitude band above and below the unit
};
static constexpr AlertZone alert_zone{30, 2000, 300}; // red
static constexpr AlertZone prox_zone{60, 4000, 600};  // yellow
static constexpr double kt_to_ms = 0.514444;
// our own aircraft as seen by ADS-B: within this distance of the unit on every axis
static constexpr double own_dist_m = 25;
// the cone of the possible maneuvers of an aircraft (the track and the vertical speed may
// change), narrowing with its speed: the half angle is cone_angle_k / speed (deg, m/s)
// and the vertical speed change is cone_vs_k / speed (m/s), both bounded
static constexpr double cone_angle_k = 1500, cone_angle_min = 10, cone_angle_max = 60;
static constexpr double cone_vs_k = 150, cone_vs_min = 1, cone_vs_max = 5;
static constexpr int cone_steps = 3; // tracks sampled on each side of the reported one

// readsb style traffic around a point, no key
static const char *adsb_url = "https://opendata.adsb.fi/api/v2/lat/%1/lon/%2/dist/%3";
// OpenAIP airspaces export of a country
static const char *atc_url = "https://storage.openaip.net/openaip-system-exports/%1_asp.geojson";

// country codes with an airspaces export on OpenAIP (checked 2026-10), "-" is off
QStringList Traffic::countries() const
{
    static const QStringList list{
        "-",
        "Afghanistan (AF)", "Albania (AL)", "Argentina (AR)", "Armenia (AM)",
        "Australia (AU)", "Austria (AT)", "Azerbaijan (AZ)", "Belarus (BY)",
        "Belgium (BE)", "Bosnia and Herzegovina (BA)", "Brazil (BR)",
        "Bulgaria (BG)", "Canada (CA)", "Chile (CL)", "China (CN)",
        "Croatia (HR)", "Cyprus (CY)", "Czechia (CZ)", "Denmark (DK)",
        "Estonia (EE)", "Finland (FI)", "France (FR)", "Georgia (GE)",
        "Germany (DE)", "Greece (GR)", "Hungary (HU)", "Iceland (IS)",
        "India (IN)", "Indonesia (ID)", "Iran (IR)", "Ireland (IE)",
        "Israel (IL)", "Italy (IT)", "Japan (JP)", "Kazakhstan (KZ)",
        "Latvia (LV)", "Lithuania (LT)", "Luxembourg (LU)", "Malaysia (MY)",
        "Malta (MT)", "Mexico (MX)", "Moldova (MD)", "Mongolia (MN)",
        "Morocco (MA)", "Netherlands (NL)", "New Zealand (NZ)",
        "North Macedonia (MK)", "Norway (NO)", "Oman (OM)", "Pakistan (PK)",
        "Poland (PL)", "Portugal (PT)", "Qatar (QA)", "Romania (RO)",
        "Russia (RU)", "Saudi Arabia (SA)", "Serbia (RS)", "Slovakia (SK)",
        "Slovenia (SI)", "South Africa (ZA)", "Spain (ES)", "Sweden (SE)",
        "Switzerland (CH)", "Tajikistan (TJ)", "Thailand (TH)", "Turkey (TR)",
        "Turkmenistan (TM)", "United Arab Emirates (AE)",
        "United Kingdom (GB)", "United States (US)", "Uzbekistan (UZ)",
    };
    return list;
}

// two letter country code of the airspaces export, empty when off
static QString countryCode(Fact *f)
{
    QString value = f->value().toString().trimmed();
    if (value == "-")
        return {};
    QString cc = value;
    int pos = value.lastIndexOf('(');
    if (pos >= 0 && value.endsWith(')'))
        cc = value.mid(pos + 1, value.size() - pos - 2);
    cc = cc.trimmed().toUpper();
    if (cc.size() != 2)
        return {};
    for (const auto &entry : f->enumStrings()) {
        if (entry.endsWith("(" + cc + ")"))
            return cc.toLower();
    }
    return {};
}

Traffic::Traffic(Fact *parent)
    : Fact(parent, QString(PLUGIN_NAME).toLower(), tr("Traffic"), tr("ADS-B traffic and airspaces"), Group)
{
    setIcon("airplane");

    // "enabled" would shadow the Fact::enabled property of the plugin fact
    f_enabled = new Fact(this,
                         "use",
                         tr("Enabled"),
                         tr("Request traffic and airspaces"),
                         Bool | PersistentValue,
                         "power");
    f_enabled->setDefaultValue(false);

    f_radius = new Fact(this,
                        "radius",
                        tr("Radius"),
                        tr("Request radius around the unit"),
                        Int | PersistentValue,
                        "radius");
    f_radius->setMin(10);
    f_radius->setMax(400);
    f_radius->setUnits("km");
    f_radius->setDefaultValue(100);

    f_country = new Fact(this,
                         "country",
                         tr("Airspaces"),
                         tr("Country of the airspaces (OpenAIP), '-' is off"),
                         Text | PersistentValue,
                         "map-marker-radius");
    f_country->setEnumStrings(countries());
    f_country->setDefaultValue("-");

    connect(f_country, &Fact::valueChanged, this, &Traffic::updateCountryDescr);
    updateCountryDescr();

    _aircraft = new AircraftModel(this);
    _aircraftSorted = new QSortFilterProxyModel(this);
    _aircraftSorted->setSourceModel(_aircraft);
    _aircraftSorted->setSortRole(AircraftModel::SortKeyRole); // threats first, then nearest
    _aircraftSorted->setDynamicSortFilter(true);
    _aircraftSorted->sort(-1); // sorted while the unit has a position only (see onAdsbReply)
    _airspaces = new AirspaceModel(this);

    _timer.setInterval(adsb_period_ms);
    connect(&_timer, &QTimer::timeout, this, &Traffic::onTimer);

    connect(f_enabled, &Fact::valueChanged, this, &Traffic::onEnabledChanged);
    connect(f_country, &Fact::valueChanged, this, [this]() {
        clearAirspaces();
        onTimer();
    });
    connect(f_radius, &Fact::valueChanged, this, [this]() { filterAirspaces(true); });
    connect(&_atcWatcher, &QFutureWatcher<QList<Airspace>>::finished, this, &Traffic::onAtcParsed);

    // start when the app is loaded, or right away (queued, after the QML below is loaded)
    // when the plugin is enabled at runtime from the plugins settings
    if (App::loaded())
        QTimer::singleShot(0, this, &Traffic::onEnabledChanged);
    else
        connect(App::instance(), &App::loadingFinished, this, &Traffic::onEnabledChanged);

    // reachability from the OS when its backend is available, the requests tell otherwise
    if (QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability)) {
        connect(QNetworkInformation::instance(),
                &QNetworkInformation::reachabilityChanged,
                this,
                &Traffic::onReachabilityChanged);
        onReachabilityChanged();
    }

    loadQml("qrc:/" PLUGIN_NAME "/TrafficPlugin.qml");      // traffic list, instruments panel
    loadQml("qrc:/" PLUGIN_NAME "/TrafficMapLayer.qml");    // aircraft and airspaces on the map
    loadQml("qrc:/" PLUGIN_NAME "/TrafficRadarPlugin.qml"); // radar widget, main layout
}

// the country item description when nothing is going on
void Traffic::updateCountryDescr()
{
    QString value = f_country->value().toString().trimmed();
    QString cc = countryCode(f_country);
    if (value == "-" || value.isEmpty()) {
        f_country->setDescr(tr("Airspaces are off"));
        return;
    }
    if (cc.isEmpty()) {
        f_country->setDescr(tr("Select a valid country"));
        return;
    }
    for (const auto &entry : countries()) {
        if (entry.endsWith("(" + cc.toUpper() + ")")) {
            f_country->setDescr(tr("Selected: %1").arg(entry));
            return;
        }
    }
}

// short status for the tools menu: "ADS-B: HTTP 429" or the error text
static QString replyError(const QString &src, QNetworkReply *reply)
{
    int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (code)
        return QString("%1: HTTP %2").arg(src).arg(code);
    if (reply->error() == QNetworkReply::OperationCanceledError)
        return QString("%1: %2").arg(src).arg(QObject::tr("timeout"));
    return QString("%1: %2").arg(src).arg(reply->errorString().section(" - ", -1));
}

void Traffic::updateStatus()
{
    QString s;
    if (_offline) {
        s = tr("OFFLINE");
    } else {
        s = _adsbStatus;
        if (!_atcStatus.isEmpty())
            s += (s.isEmpty() ? "" : "  ") + _atcStatus;
    }
    if (_status == s)
        return;
    _status = s;
    setDescr(_status.isEmpty() ? tr("ADS-B traffic and airspaces") : _status);
    emit statusChanged();
}

// the reply failed below HTTP: no route, DNS, refused, timeout (codes 1..199)
bool Traffic::isNetworkDown(QNetworkReply *reply)
{
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
        return false; // the server answered
    auto e = reply->error();
    return e != QNetworkReply::NoError && e < QNetworkReply::ContentAccessDenied;
}

void Traffic::setRequestDown(bool v)
{
    _reqDown = v;
    if (!v)
        _netDown = false; // a reply came, whatever the OS reachability says
    updateOffline();
}

void Traffic::updateOffline()
{
    bool v = f_enabled->value().toBool() && (_netDown || _reqDown);
    if (_offline == v)
        return;
    _offline = v;
    if (!v) {
        // back online: request now, not after the backoff or the retry period
        _adsbHoldMs = 0;
        if (_atcAll.isEmpty() && !_atcBlocked && !_atcReply && !_atcWatcher.isRunning())
            _atcTimeMs = 0;
    }
    emit offlineChanged();
    updateStatus();
}

void Traffic::onReachabilityChanged()
{
    using R = QNetworkInformation::Reachability;
    auto r = QNetworkInformation::instance()->reachability();
    bool down = r == R::Disconnected || r == R::Local || r == R::Site;
    if (_netDown == down)
        return;
    _netDown = down;
    if (!down)
        _adsbHoldMs = 0; // probe now
    updateOffline();
}

// a transfer timeout also finishes the reply with OperationCanceledError,
// the property tells the aborts of the plugin apart to report the timeouts
void Traffic::abortReply(QNetworkReply *reply)
{
    if (!reply)
        return;
    reply->setProperty("aborted", true);
    reply->abort();
}

void Traffic::onEnabledChanged()
{
    bool v = f_enabled->value().toBool();
    setActive(v);
    // a blocked airspaces download is retried on restart only
    _atcBlocked = false;
    f_country->setEnabled(true);
    if (v) {
        _atcTimeMs = 0;
        _adsbHoldMs = 0;
        _adsbBackoffMs = 0;
        updateOffline();
        onTimer();
        _timer.start();
        return;
    }
    _timer.stop();
    abortReply(_adsbReply);
    _aircraft->clear();
    dropGone();
    clearAirspaces();
    _adsbStatus.clear();
    _reqDown = false;
    updateOffline();
    updateStatus();
}

void Traffic::clearAirspaces()
{
    abortReply(_atcReply);
    _atcReply = nullptr; // finished was emitted, the reply is deleted later
    _atcAll.clear();
    _atcCountry.clear();
    _atcTimeMs = 0;
    _atcShown.clear();
    _atcFilterOrigin = QGeoCoordinate();
    _airspaces->reset({});
    f_country->setProgress(-1);
    updateCountryDescr();
    _atcStatus.clear();
    updateStatus();
}

QGeoCoordinate Traffic::origin() const
{
    auto unit = Fleet::instance()->current();
    if (unit) {
        auto c = unit->coordinate();
        if (c.isValid() && (c.latitude() != 0 || c.longitude() != 0))
            return c;
    }
    return _mapCenter;
}

double Traffic::alertRadiusM()
{
    return alert_zone.radiusM;
}
double Traffic::proxRadiusM()
{
    return prox_zone.radiusM;
}
QString Traffic::unitAltSrc() const
{
    if (qIsNaN(_unit.altM))
        return {};
    return _unit.baro ? "PA" : "HMSL";
}

// the unit altitude is the pressure altitude from the static pressure (sns.baro.mbar) when
// the unit reports it, like the ADS-B baro altitude of the aircraft, HMSL (GNSS) otherwise
Traffic::UnitState Traffic::unitState() const
{
    UnitState s;
    auto unit = Fleet::instance()->current();
    if (!unit)
        return s;
    auto c = unit->coordinate();
    if (c.isValid() && (c.latitude() != 0 || c.longitude() != 0)) {
        s.pos = c;
        s.valid = true;
    }
    auto m = unit->f_mandala;
    if (!m)
        return s;
    // the values as they are (the replay and the simulator do not mark them as received)
    auto value = [m](mandala::uid_t uid) {
        auto f = m->fact(uid);
        return f ? f->value().toDouble() : 0.0;
    };
    double mbar = value(mandala::sns::nav::baro::mbar::uid);
    if (mbar > 0) {
        s.altM = 44330.8 * (1 - std::pow(mbar / 1013.25, 0.190284));
        s.baro = true;
    } else if (s.valid) {
        s.altM = value(mandala::est::nav::pos::hmsl::uid); // with the position only
    }
    s.spdMs = value(mandala::est::nav::pos::speed::uid);
    s.crsDeg = value(mandala::est::nav::pos::bearing::uid);
    s.vsMs = value(mandala::est::nav::pos::vspeed::uid);
    return s;
}

// seconds until the aircraft is inside the zone, assuming straight line motion:
// r is its position relative to the unit (east, north, up: m), v its velocity relative
// to the unit (m/s). 0 when it is inside now, inf when it never is (passes by, diverges).
// An unknown altitude (dz NaN) counts as inside the altitude band
static double threatTime(double rx, double ry, double dz, double vx, double vy, double vz, const AlertZone &z)
{
    const double inf = qInf();
    // the time window inside the circle: |r + v t| <= R
    double v2 = vx * vx + vy * vy;
    double b = rx * vx + ry * vy;
    double c = rx * rx + ry * ry - z.radiusM * z.radiusM;
    double tIn = 0, tOut = inf;
    if (v2 < 1e-6) {
        if (c > 0)
            return inf; // not moving relative to the unit, outside
    } else {
        double disc = b * b - v2 * c;
        if (disc < 0)
            return inf; // passes by
        double s = std::sqrt(disc);
        tOut = (-b + s) / v2;
        if (tOut < 0)
            return inf; // was inside in the past, diverging
        tIn = qMax(0.0, (-b - s) / v2);
    }
    // the time window inside the altitude band: |dz + vz t| <= H
    double zIn = 0, zOut = inf;
    if (!qIsNaN(dz)) {
        if (qAbs(vz) < 1e-3) {
            if (qAbs(dz) > z.dAltM)
                return inf; // level, outside the band
        } else {
            double t1 = (-z.dAltM - dz) / vz, t2 = (z.dAltM - dz) / vz;
            zOut = qMax(t1, t2);
            if (zOut < 0)
                return inf; // left the band in the past
            zIn = qMax(0.0, qMin(t1, t2));
        }
    }
    double t = qMax(tIn, zIn);
    if (t > tOut || t > zOut)
        return inf; // the windows do not overlap: out of the band while inside the circle
    return t;
}

// the threat time over the cone of the possible maneuvers of the aircraft (speed va m/s,
// track and vertical speed as reported) relative to the unit velocity: the worst case
static double threatTimeCone(double rx, double ry, double dz, double va, double trackDeg,
                             double vsA, double uvx, double uvy, double uvz, const AlertZone &z)
{
    double v = qMax(va, 1.0);
    double halfAngle = qBound(cone_angle_min, cone_angle_k / v, cone_angle_max);
    double dvs = qBound(cone_vs_min, cone_vs_k / v, cone_vs_max);
    double tau = qInf();
    for (int i = -cone_steps; i <= cone_steps; ++i) {
        double t = qDegreesToRadians(trackDeg + halfAngle * i / cone_steps);
        double vx = va * std::sin(t) - uvx;
        double vy = va * std::cos(t) - uvy;
        for (int k = -1; k <= 1; ++k)
            tau = qMin(tau, threatTime(rx, ry, dz, vx, vy, vsA + dvs * k - uvz, z));
    }
    return tau;
}

void Traffic::onTimer()
{
    if (!f_enabled->value().toBool() || !origin().isValid())
        return;
    auto now = QDateTime::currentMSecsSinceEpoch();

    // no replies for a while (errors, rate limit backoff): the positions are stale
    _aircraft->dropStale(now - adsb_stale_ms);
    dropGone();

    if (now >= _adsbHoldMs)
        requestAdsb();

    // airspaces: the country file once per period, the visible part on every move
    auto cc = countryCode(f_country);
    if (cc.isEmpty() || _atcBlocked)
        return;
    if (cc != _atcCountry || now - _atcTimeMs > atc_period_ms)
        requestAtc();
    filterAirspaces();
}

void Traffic::requestAdsb()
{
    if (_adsbReply)
        return; // previous request in progress
    auto c = origin();
    double nm = qBound(1.0, f_radius->value().toDouble() / 1.852, 250.0);
    QNetworkRequest req{QUrl(QString(adsb_url)
                                 .arg(c.latitude(), 0, 'f', 5)
                                 .arg(c.longitude(), 0, 'f', 5)
                                 .arg(qRound(nm)))};
    req.setHeader(QNetworkRequest::UserAgentHeader, "apx-gcs");
    req.setTransferTimeout(adsb_timeout_ms);
    _adsbReply = _net.get(req);
    connect(_adsbReply, &QNetworkReply::finished, this, &Traffic::onAdsbReply);
}

void Traffic::onAdsbReply()
{
    auto reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        if (reply->property("aborted").toBool())
            return;
        setRequestDown(isNetworkDown(reply));
        if (_reqDown) // offline: keep probing, but not every second
            _adsbHoldMs = QDateTime::currentMSecsSinceEpoch() + adsb_offline_poll_ms;
        _adsbStatus = replyError("ADS-B", reply);
        updateStatus();
        // rate limit: back off, Retry-After when given
        int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (code == 429 || code == 503) {
            qint64 retry = reply->rawHeader("Retry-After").toLongLong() * 1000; // seconds or a date (0)
            _adsbBackoffMs = retry > 0 ? int(qMin<qint64>(retry, adsb_backoff_max_ms))
                                       : qMin(adsb_backoff_max_ms,
                                              _adsbBackoffMs ? _adsbBackoffMs * 2
                                                             : adsb_period_ms * 2);
            _adsbHoldMs = QDateTime::currentMSecsSinceEpoch() + _adsbBackoffMs;
        }
        return;
    }
    _adsbBackoffMs = 0;
    setRequestDown(false);

    auto root = QJsonDocument::fromJson(reply->readAll()).object();
    auto acv = root.value("aircraft"); // adsb.fi
    if (!acv.isArray())
        acv = root.value("ac"); // adsb.lol, airplanes.live
    if (!acv.isArray()) {
        // not the API reply (captive portal, proxy page): keep the aircraft, not an empty list
        _adsbStatus = QString("ADS-B: %1").arg(tr("bad reply"));
        updateStatus();
        return;
    }
    auto ac = acv.toArray(); // empty is no traffic around

    auto unit = unitState();
    auto o = unit.valid ? unit.pos : _mapCenter;
    auto now = QDateTime::currentMSecsSinceEpoch();
    // the unit velocity (east, north), m/s
    double uvx = unit.spdMs * std::sin(qDegreesToRadians(unit.crsDeg));
    double uvy = unit.spdMs * std::cos(qDegreesToRadians(unit.crsDeg));

    QList<Aircraft> list;
    for (const auto &v : ac) {
        auto j = v.toObject();
        Aircraft a;
        a.hex = j.value("hex").toString();
        auto lat = j.value("lat"), lon = j.value("lon");
        if (!lat.isDouble() || !lon.isDouble())
            continue; // no position (would be placed at 0,0)
        a.coordinate = QGeoCoordinate(lat.toDouble(), lon.toDouble());
        if (a.hex.isEmpty() || !a.coordinate.isValid())
            continue;
        if (j.value("seen_pos").toDouble() > adsb_seen_pos_max)
            continue; // the position is too old
        a.callsign = j.value("flight").toString().trimmed();
        a.type = j.value("t").toString();
        a.category = j.value("category").toString();
        // altitudes come in ft: alt_baro is the pressure altitude ("ground" on the surface),
        // alt_geom is the GNSS altitude above the WGS84 ellipsoid
        auto baro = j.value("alt_baro");
        a.onGround = baro.isString();
        if (baro.isDouble())
            a.altBaroM = baro.toDouble() * 0.3048;
        else if (a.onGround)
            a.altBaroM = 0;
        auto geom = j.value("alt_geom");
        if (geom.isDouble())
            a.altGeomM = geom.toDouble() * 0.3048;
        a.gsKt = j.value("gs").toDouble();
        // track is often missing (on the ground, slow), heading is the next best for the icon
        for (const char *key : {"track", "true_heading", "mag_heading", "nav_heading"}) {
            auto t = j.value(QLatin1String(key));
            if (t.isDouble()) {
                a.track = t.toDouble();
                a.trackValid = true;
                break;
            }
        }
        auto rate = j.value("geom_rate");
        if (!rate.isDouble())
            rate = j.value("baro_rate");
        a.vsMs = rate.toDouble() * 0.3048 / 60.0;
        a.distKm = o.distanceTo(a.coordinate) / 1000.0;
        a.brg = o.azimuthTo(a.coordinate);
        a.seenMs = now;
        // the altitude of the same kind as the unit altitude: baro with the pressure
        // altitude, GNSS with HMSL (the other one when the aircraft has no such)
        double alt = unit.baro ? (qIsNaN(a.altBaroM) ? a.altGeomM : a.altBaroM)
                               : (qIsNaN(a.altGeomM) ? a.altBaroM : a.altGeomM);
        a.dAltM = alt - unit.altM; // NaN when either is unknown
        // the alert is by the time to the zone around the unit (or the map center, then
        // the unit is still): the relative position and velocity (east, north, up)
        double rx = a.distKm * 1000 * std::sin(qDegreesToRadians(a.brg));
        double ry = a.distKm * 1000 * std::cos(qDegreesToRadians(a.brg));
        double va = a.gsKt * kt_to_ms;
        double vx = va * std::sin(qDegreesToRadians(a.track)) - uvx;
        double vy = va * std::cos(qDegreesToRadians(a.track)) - uvy;
        // our own aircraft as seen by ADS-B: at the unit on every axis (an unknown altitude
        // counts as the same), not drawn on the map, grey in the list, no alerts
        a.own = unit.valid && qAbs(rx) < own_dist_m && qAbs(ry) < own_dist_m
                && (qIsNaN(a.dAltM) || qAbs(a.dAltM) < own_dist_m);
        // the alerts are relative to the unit only: none with the map center as the origin
        if (!unit.valid || a.own || a.onGround) {
            a.tauS = qInf();
            a.level = 0;
        } else {
            // the worst case over the cone of the possible maneuvers
            a.tauS = threatTimeCone(rx, ry, a.dAltM, va, a.track, a.vsMs, uvx, uvy, unit.vsMs, prox_zone);
            if (threatTimeCone(rx, ry, a.dAltM, va, a.track, a.vsMs, uvx, uvy, unit.vsMs, alert_zone)
                <= alert_zone.tauS)
                a.level = 2;
            else if (a.tauS <= prox_zone.tauS)
                a.level = 1;
            else
                a.level = 0;
        }
        list.append(a);
    }
    bool unitChanged = unit.valid != _unit.valid || unit.baro != _unit.baro
                       || !(unit.altM == _unit.altM || (qIsNaN(unit.altM) && qIsNaN(_unit.altM)));
    if (unit.valid != _unit.valid)
        _aircraftSorted->sort(unit.valid ? 0 : -1); // no unit: the reply order
    _unit = unit;
    if (unitChanged)
        emit unitStateChanged();
    _aircraft->update(list);
    dropGone();
    _adsbStatus.clear();
    updateStatus();
}

void Traffic::setSelected(const QString &hex)
{
    if (_selected == hex)
        return;
    _selected = hex;
    emit selectedChanged();
}

void Traffic::setHighlighted(const QString &hex)
{
    if (_highlighted == hex)
        return;
    _highlighted = hex;
    emit highlightedChanged();
}

void Traffic::dropGone()
{
    if (!_selected.isEmpty() && !_aircraft->contains(_selected))
        setSelected({});
    if (!_highlighted.isEmpty() && !_aircraft->contains(_highlighted))
        setHighlighted({});
}

void Traffic::requestAtc()
{
    if (_atcReply || _atcWatcher.isRunning())
        return;
    QString cc = countryCode(f_country);
    if (cc.isEmpty())
        return;
    QNetworkRequest req{QUrl(QString(atc_url).arg(cc))};
    req.setHeader(QNetworkRequest::UserAgentHeader, "apx-gcs");
    req.setTransferTimeout(atc_timeout_ms);
    _atcCountry = cc;
    _atcTimeMs = QDateTime::currentMSecsSinceEpoch();
    _atcStatus.clear();
    updateStatus();
    f_country->setProgress(0);
    f_country->setDescr(tr("Downloading airspaces..."));
    _atcReply = _net.get(req);
    connect(_atcReply, &QNetworkReply::downloadProgress, this,
            [this](qint64 received, qint64 total) {
                if (total > 0)
                    f_country->setProgress(qBound(1, int(received * 90 / total), 90));
            });
    connect(_atcReply, &QNetworkReply::finished, this, &Traffic::onAtcReply);
}

static QString limitText(const QJsonObject &j)
{
    int v = qRound(j.value("value").toDouble());
    int unit = j.value("unit").toInt();            // 0 m, 1 ft, 6 FL
    int datum = j.value("referenceDatum").toInt(); // 0 GND, 1 MSL, 2 STD
    if (unit == 6)
        return QString("FL%1").arg(v);
    if (v == 0 && datum == 0)
        return "SFC";
    if (unit == 0)
        v = qRound(v / 0.3048);
    return QString("%1 ft%2").arg(v).arg(datum == 0 ? " AGL" : "");
}

// OpenAIP file -> airspaces, runs in a worker thread (big country files take a while)
static QList<Airspace> parseAirspaces(const QByteArray &data)
{
    // OpenAIP airspace types
    static const QHash<int, QString> types{
        {1, "R"},     {2, "D"},      {3, "P"},      {4, "CTR"},   {5, "TMZ"},   {6, "RMZ"},
        {7, "TMA"},   {8, "TRA"},    {9, "TSA"},    {12, "ADIZ"}, {13, "ATZ"},  {14, "MATZ"},
        {15, "AWY"},  {16, "MTR"},   {17, "ALERT"}, {18, "WARN"}, {19, "PROT"}, {20, "HTZ"},
        {21, "GLD"},  {22, "TRP"},   {23, "TIZ"},   {24, "TIA"},  {25, "MTA"},  {26, "CTA"},
        {27, "ACC"},  {28, "SPORT"}, {29, "LFR"},   {30, "MRT"},  {31, "TFR"},
    };

    auto root = QJsonDocument::fromJson(data).object();
    // country export: {features:[{properties, geometry}]}, API reply: {items:[{...geometry}]}
    auto items = root.value("features").toArray();
    bool geojson = !items.isEmpty();
    if (!geojson)
        items = root.value("items").toArray();

    QList<Airspace> list;
    for (const auto &v : items) {
        auto f = v.toObject();
        auto j = geojson ? f.value("properties").toObject() : f;
        int type = j.value("type").toInt();
        if (type == 10 || type == 11) // FIR, UIR
            continue;
        auto geom = f.value("geometry").toObject();
        if (geom.value("type").toString() != "Polygon")
            continue;
        auto ring = geom.value("coordinates").toArray().first().toArray();
        if (ring.size() < 3)
            continue;
        Airspace a;
        double lat = 0, lon = 0;
        a.minLat = a.minLon = 1e9;
        a.maxLat = a.maxLon = -1e9;
        for (const auto &p : ring) {
            auto pt = p.toArray();
            QGeoCoordinate c(pt.at(1).toDouble(), pt.at(0).toDouble());
            lat += c.latitude();
            lon += c.longitude();
            a.minLat = qMin(a.minLat, c.latitude());
            a.maxLat = qMax(a.maxLat, c.latitude());
            a.minLon = qMin(a.minLon, c.longitude());
            a.maxLon = qMax(a.maxLon, c.longitude());
            a.path.append(QVariant::fromValue(c));
        }
        a.path.append(a.path.first()); // closed ring for the polyline
        a.center = QGeoCoordinate(lat / ring.size(), lon / ring.size());
        a.name = j.value("name").toString();
        a.label = QString("%1 %2 %3-%4")
                      .arg(types.value(type, "ASP"))
                      .arg(a.name)
                      .arg(limitText(j.value("lowerLimit").toObject()))
                      .arg(limitText(j.value("upperLimit").toObject()));
        a.color = (type >= 1 && type <= 3) || type == 31 ? "#FF4C00" : "#3779C5";
        list.append(a);
    }
    return list;
}

void Traffic::onAtcReply()
{
    auto reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        if (!reply->property("aborted").toBool()) {
            bool down = isNetworkDown(reply);
            setRequestDown(down);
            if (down) {
                // one try without a connection: the country is locked until the plugin restarts
                _atcBlocked = true;
                f_country->setEnabled(false);
                _atcStatus = tr("ATC: offline");
                f_country->setDescr(tr("Offline, restart the plugin to retry"));
            } else {
                _atcStatus = replyError("ATC", reply);
                f_country->setDescr(_atcStatus);
                // retry sooner than the regular period
                _atcTimeMs = QDateTime::currentMSecsSinceEpoch() - atc_period_ms + atc_retry_ms;
            }
            updateStatus();
        }
        f_country->setProgress(-1);
        return;
    }
    setRequestDown(false);
    f_country->setProgress(95);
    f_country->setDescr(tr("Parsing airspaces..."));
    _atcWatcher.setFuture(QtConcurrent::run(parseAirspaces, reply->readAll()));
}

void Traffic::onAtcParsed()
{
    if (!_atcReply)
        f_country->setProgress(-1);
    // country changed, switched off or the plugin disabled while parsing
    if (_atcCountry.isEmpty() || _atcCountry != countryCode(f_country))
        return;
    _atcAll = _atcWatcher.result();
    _atcFilterOrigin = QGeoCoordinate(); // _atcShown is of the previous list
    filterAirspaces(true);
    if (_atcAll.isEmpty()) {
        _atcStatus = tr("ATC: no valid airspaces in file");
        f_country->setDescr(_atcStatus);
    } else {
        f_country->setDescr(tr("Loaded %1 airspaces, %2 in range")
                               .arg(_atcAll.size()).arg(_atcShown.size()));
    }
    updateStatus();
}

// the model gets the airspaces within 1.5 radius of the origin,
// recomputed when the origin moves by 1/10 of the radius (no network)
void Traffic::filterAirspaces(bool force)
{
    auto o = origin();
    if (!o.isValid())
        return;
    double maxDist = f_radius->value().toDouble() * 1000 * 1.5;
    if (!force && _atcFilterOrigin.isValid() && _atcFilterDist == maxDist
        && _atcFilterOrigin.distanceTo(o) < maxDist / 15)
        return;
    _atcFilterOrigin = o;
    _atcFilterDist = maxDist;

    QList<int> shown;
    for (int i = 0; i < _atcAll.size(); ++i) {
        const auto &a = _atcAll.at(i);
        // nearest point of the bounding box to the origin
        QGeoCoordinate n(qBound(a.minLat, o.latitude(), a.maxLat),
                         qBound(a.minLon, o.longitude(), a.maxLon));
        if (o.distanceTo(n) < maxDist)
            shown.append(i);
    }
    if (shown == _atcShown && !_atcAll.isEmpty())
        return;
    _atcShown = shown;

    QList<Airspace> list;
    list.reserve(shown.size());
    for (int i : shown)
        list.append(_atcAll.at(i));
    _airspaces->reset(list);
    _atcStatus = (!_atcAll.isEmpty() && list.isEmpty()) ? tr("ATC: no airspaces in range")
                                                          : QString();
    if (!_atcAll.isEmpty())
        f_country->setDescr(tr("Loaded %1 airspaces, %2 in range")
                               .arg(_atcAll.size()).arg(list.size()));
    updateStatus();
}
