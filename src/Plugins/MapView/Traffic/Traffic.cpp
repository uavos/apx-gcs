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
#include <App/AppDirs.h>
#include <Fleet/Fleet.h>
#include <Fleet/Unit.h>
#include <Mandala/Mandala.h>
#include <Mandala/MandalaFact.h>

#include <QPolygonF>

#include <cmath>

static constexpr int adsb_period_ms = 1000;
static constexpr int adsb_timeout_ms = 5000;      // inactivity, also the offline detection time
static constexpr int adsb_offline_poll_ms = 5000; // request period while offline
static constexpr int adsb_backoff_max_ms = 120000;
static constexpr int adsb_stale_ms = 30000;     // drop aircraft not updated for this long
static constexpr double adsb_seen_pos_max = 60; // skip aircraft with an older position, s
static constexpr int atc_retry_ms = 30 * 1000; // after a failed download (HTTP errors)
static constexpr int atc_timeout_ms = 5000;    // inactivity, a live download keeps resetting it
static constexpr double kt_to_ms = 0.514444;
// our own aircraft as seen by ADS-B: within this distance of the unit on every axis
static constexpr double own_dist_m = 25;

// conflict prediction: the possible positions of every aircraft (a slice of its reach
// volume) are computed for the instants of the horizon; an aircraft is a threat when
// its possible positions come closer than the separation to ours
static constexpr double sep_radius_m = 500; // the separation, horizontal
static constexpr double sep_alt_m = 150;    // and vertical
static constexpr double horizon_s = 60;     // yellow within the horizon
static constexpr double horizon_red_s = 30; // red this close
static constexpr double reach_step_s = 2;   // the conflict scan time step
static constexpr double alt_unc_m = 50;     // altitude uncertainty (baro vs the reference)
static constexpr double omega_cap = qDegreesToRadians(6.0); // turn rate cap, rad/s
// radar display rings, m (red and yellow)
static constexpr double alert_radius_m = 2000;
static constexpr double prox_radius_m = 4000;

// how far an aircraft can maneuver: the turn is limited by the bank angle
// (omega = g tan(bank) / v, R = v^2 / (g tan(bank))) and the rate cap, the vertical
// speed can change by dVs. A hover capable craft (disc) can go anywhere within its
// speed (at least vMin), and so can one with an unknown track. The banks model the
// plausible maneuvering of the class, not the airframe limits: wider makes the
// prediction more conservative (more alerts)
struct ManeuverClass
{
    double bankDeg;
    double dVs;  // m/s
    bool disc;   // hover capable
    double vMin; // m/s, the floor of the speed assumed for the reach
};

// by the ADS-B emitter category (see categoryInfo in TrafficModels.cpp)
static ManeuverClass maneuverClass(const QString &c)
{
    static const QHash<QString, ManeuverClass> map{
        {"A1", {30, 4, false, 0}},   // light
        {"A2", {25, 6, false, 0}},   // small
        {"A3", {15, 8, false, 0}},   // large
        {"A4", {15, 8, false, 0}},   // high vortex
        {"A5", {12, 8, false, 0}},   // heavy
        {"A6", {45, 30, false, 0}},  // high performance
        {"A7", {0, 8, true, 60}},    // rotorcraft
        {"B1", {30, 3, false, 0}},   // glider
        {"B2", {0, 3, true, 15}},    // lighter-than-air
        {"B3", {0, 8, true, 15}},    // parachutist
        {"B4", {25, 3, false, 0}},   // ultralight
        {"B6", {0, 8, true, 40}},    // UAV
        {"B7", {30, 100, false, 0}}, // space vehicle
    };
    return map.value(c, {20, 8, false, 0});
}
static constexpr ManeuverClass unit_class{30, 5, false, 0}; // our own aircraft

// the possible positions at time t: the horizontal ring (ENU m relative to the position
// now) and the altitude band (m relative to the altitude now)
struct ReachSlice
{
    QPolygonF ring;
    double lo{0}, hi{0};
};

// v m/s, track deg, vertical speed vs m/s. Fixed wing at constant speed: the turn at the
// full rate for tau then straight traces the outer boundary on each side (Dubins), the
// rear is closed by a chord (slightly wider than the true set). A hover capable craft,
// or one that can turn all the way around within t, gets the full disc
static ReachSlice reachSlice(double v, double trackDeg, double vs, const ManeuverClass &mc, double t)
{
    ReachSlice s;
    s.lo = vs * t - (mc.dVs * t + alt_unc_m);
    s.hi = vs * t + (mc.dVs * t + alt_unc_m);
    double vv = qMax(v, mc.vMin);
    double omega = mc.disc ? 0
                           : qMin(omega_cap,
                                  9.81 * std::tan(qDegreesToRadians(mc.bankDeg)) / qMax(vv, 1.0));
    if (mc.disc || omega * t >= M_PI) {
        double r = vv * t;
        for (int i = 0; i < 12; ++i) {
            double a = 2 * M_PI * i / 12;
            s.ring << QPointF(std::sin(a) * r, std::cos(a) * r);
        }
        return s;
    }
    // local frame (x to the right of the track, y along it), rotated to ENU by the track
    double tr = qDegreesToRadians(trackDeg);
    double sinT = std::sin(tr), cosT = std::cos(tr);
    auto pt = [&](double side, double tau) {
        double th = omega * tau; // the heading change over the turn
        double R = vv / qMax(omega, 1e-6);
        double x = side * R * (1 - std::cos(th));
        double y = R * std::sin(th);
        x += side * vv * (t - tau) * std::sin(th);
        y += vv * (t - tau) * std::cos(th);
        return QPointF(y * sinT + x * cosT, y * cosT - x * sinT); // east, north
    };
    const int N = 5;
    for (int k = 0; k <= N; ++k) // the tip (straight) to the full right turn
        s.ring << pt(+1, t * k / N);
    for (int k = N; k >= 1; --k) // the full left turn back towards the tip
        s.ring << pt(-1, t * k / N);
    return s;
}

// the smallest distance from the point to the segment
static double segDist(const QPointF &p, const QPointF &a, const QPointF &b)
{
    QPointF d = b - a;
    double l2 = d.x() * d.x() + d.y() * d.y();
    double u = l2 < 1e-9 ? 0.0
                         : qBound(0.0,
                                  ((p.x() - a.x()) * d.x() + (p.y() - a.y()) * d.y()) / l2,
                                  1.0);
    return std::hypot(p.x() - (a.x() + d.x() * u), p.y() - (a.y() + d.y() * u));
}

// the smallest distance between two rings, m (0 when one is inside the other), B offset by ob
static double ringGap(const QPolygonF &A, const QPolygonF &B, const QPointF &ob)
{
    QPolygonF Bo(B);
    Bo.translate(ob);
    if (A.containsPoint(Bo.first(), Qt::OddEvenFill) || Bo.containsPoint(A.first(), Qt::OddEvenFill))
        return 0;
    double best = qInf();
    for (const auto &p : A)
        for (int i = 0; i < Bo.size(); ++i)
            best = qMin(best, segDist(p, Bo.at(i), Bo.at((i + 1) % Bo.size())));
    for (const auto &p : Bo)
        for (int i = 0; i < A.size(); ++i)
            best = qMin(best, segDist(p, A.at(i), A.at((i + 1) % A.size())));
    return best;
}

// the first instant of the horizon when the possible positions of the aircraft (at rx, ry
// east/north m and dAlt m above the unit, NaN counts as the same altitude) come closer
// than the separation to the possible positions of the unit (a slice per scan step)
static double conflictTime(const QList<ReachSlice> &own, double rx, double ry, double dAlt,
                           double v, double trackDeg, double vs, const ManeuverClass &mc)
{
    for (int k = 0; k < own.size(); ++k) {
        double t = k * reach_step_s;
        auto s = reachSlice(v, trackDeg, vs, mc, t);
        if (ringGap(own.at(k).ring, s.ring, QPointF(rx, ry)) >= sep_radius_m)
            continue;
        if (!qIsNaN(dAlt)) {
            double gap = qMax((own.at(k).lo - (dAlt + s.hi)), ((dAlt + s.lo) - own.at(k).hi));
            if (gap >= sep_alt_m)
                continue;
        }
        return t;
    }
    return qInf();
}

// readsb style traffic around a point, no key
static const char *adsb_url = "https://opendata.adsb.fi/api/v2/lat/%1/lon/%2/dist/%3";
// OpenAIP airspaces export of a country
static const char *atc_url = "https://storage.openaip.net/openaip-system-exports/%1_asp.geojson";

// country codes with an airspaces export on OpenAIP (checked 2026-10), "Off" is off
QStringList Traffic::countries() const
{
    static const QStringList list{
        "Off",
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

QStringList Traffic::cachedCountries() const
{
    QStringList list;
    for (const auto &entry : countries()) {
        int pos = entry.lastIndexOf('(');
        if (pos < 0 || !entry.endsWith(')')) {
            list.append(entry); // "Off"
            continue;
        }
        auto cc = entry.mid(pos + 1, entry.size() - pos - 2).toLower();
        if (QFileInfo(cacheFile(cc)).size() > 0)
            list.append(entry);
    }
    return list;
}

// two letter country code of the airspaces export, empty when off
static QString countryCode(Fact *f)
{
    QString value = f->value().toString().trimmed();
    if (value.isEmpty() || value == "-" || value.compare("off", Qt::CaseInsensitive) == 0)
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
    f_country->setDefaultValue("Off");

    connect(f_country, &Fact::valueChanged, this, &Traffic::updateCountryDescr);
    updateCountryDescr();

    f_view3d = new Fact(this,
                        "view3d",
                        tr("3D view"),
                        tr("Traffic and reach volumes in 3D"),
                        Bool | PersistentValue,
                        "video-3d");
    f_view3d->setDefaultValue(false);

    f_cache = new Fact(this,
                       "cache",
                       tr("Cache size"),
                       tr("Airspaces files on disk"),
                       NoFlags,
                       "database");
    connect(this, &Fact::triggered, this, &Traffic::updateCacheInfo);
    connect(&_cacheInfoWatcher, &QFutureWatcherBase::finished, this, &Traffic::cacheInfoReady);

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

    // 3D traffic window, visibility follows f_view3d (as the Ats RSSI window)
    QVariantMap opts;
    opts.insert("fact", QVariant::fromValue(f_view3d));
    opts.insert("traffic", QVariant::fromValue(this));
    _window3d = App::loadQml("qrc:/" PLUGIN_NAME "/Traffic3DWindow.qml", opts);
}

// the country item description when nothing is going on
void Traffic::updateCountryDescr()
{
    QString value = f_country->value().toString().trimmed();
    QString cc = countryCode(f_country);
    if (value.isEmpty() || value == "-" || value.compare("off", Qt::CaseInsensitive) == 0) {
        // an empty (or legacy "-") input is "Off": show it as such, called again by valueChanged
        if (value != "Off") {
            f_country->setValue("Off");
            return;
        }
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
            _atcRetryAtMs = 1; // on the next tick
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
    // a blocked airspaces download is retried on restart only: the full list is back
    if (_atcBlocked)
        f_country->setEnumStrings(countries());
    _atcBlocked = false;
    if (v) {
        _atcRetryAtMs = 0;
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
    f_view3d->setValue(false); // close the 3D window
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
    _atcRetryAtMs = 0;
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
    return alert_radius_m;
}
double Traffic::proxRadiusM()
{
    return prox_radius_m;
}
double Traffic::sepRadiusM()
{
    return sep_radius_m;
}
double Traffic::sepAltM()
{
    return sep_alt_m;
}
double Traffic::horizonS()
{
    return horizon_s;
}
double Traffic::horizonRedS()
{
    return horizon_red_s;
}

QVariantList Traffic::reachVolume(const QString &hex) const
{
    double v, trk, vs;
    ManeuverClass mc;
    if (hex.isEmpty()) {
        if (!_unit.valid)
            return {};
        v = _unit.spdMs;
        trk = _unit.crsDeg;
        vs = _unit.vsMs;
        mc = unit_class;
    } else {
        auto a = _aircraft->get(hex);
        if (!a || a->onGround)
            return {};
        v = a->gsKt * kt_to_ms;
        trk = a->track;
        vs = a->vsMs;
        mc = maneuverClass(a->category);
        if (!a->trackValid)
            mc.disc = true; // the direction is unknown
    }
    QVariantList out;
    for (double t = 0; t <= horizon_s + 0.1; t += 6) {
        auto s = reachSlice(v, trk, vs, mc, t);
        QVariantList ring;
        for (const auto &p : s.ring)
            ring << p.x() << p.y();
        out.append(QVariantMap{{"t", t}, {"ring", ring}, {"lo", s.lo}, {"hi", s.hi}});
    }
    return out;
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

    // airspaces: the country file once (cache or download), the visible part on every move
    auto cc = countryCode(f_country);
    if (cc.isEmpty())
        return;
    if (cc != _atcCountry || (_atcRetryAtMs && now >= _atcRetryAtMs))
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
    // the possible positions of the unit over the horizon, one slice per scan step
    QList<ReachSlice> ownSlices;
    if (unit.valid) {
        for (double t = 0; t <= horizon_s + 0.1; t += reach_step_s)
            ownSlices.append(reachSlice(unit.spdMs, unit.crsDeg, unit.vsMs, unit_class, t));
    }

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
        // the vertical speed as reported (ft/min): baro first, the same source as the altitude
        auto rate = j.value("baro_rate");
        if (!rate.isDouble())
            rate = j.value("geom_rate");
        a.vsMs = rate.toDouble() * 0.3048 / 60.0;
        a.distKm = o.distanceTo(a.coordinate) / 1000.0;
        a.brg = o.azimuthTo(a.coordinate);
        a.seenMs = now;
        // the aircraft pressure altitude (every transponder reports it, the GNSS altitude
        // only when there is no baro) against the unit altitude: the pressure altitude
        // from sns.baro.mbar when the unit has it, est.pos.hmsl otherwise
        double alt = qIsNaN(a.altBaroM) ? a.altGeomM : a.altBaroM;
        a.dAltM = alt - unit.altM; // NaN when either is unknown
        // the relative position (east, north), m
        double rx = a.distKm * 1000 * std::sin(qDegreesToRadians(a.brg));
        double ry = a.distKm * 1000 * std::cos(qDegreesToRadians(a.brg));
        // our own aircraft as seen by ADS-B: at the unit on every axis (an unknown altitude
        // counts as the same), not drawn on the map, grey in the list, no alerts
        a.own = unit.valid && qAbs(rx) < own_dist_m && qAbs(ry) < own_dist_m
                && (qIsNaN(a.dAltM) || qAbs(a.dAltM) < own_dist_m);
        // the alerts are relative to the unit only: none with the map center as the origin.
        // The conflict time: the first instant when the possible positions of the aircraft
        // (its reach slice by the maneuver class) come closer than the separation to ours
        if (!unit.valid || a.own || a.onGround) {
            a.tauS = qInf();
            a.level = 0;
        } else {
            auto mc = maneuverClass(a.category);
            if (!a.trackValid)
                mc.disc = true; // the direction is unknown
            a.tauS = conflictTime(ownSlices, rx, ry, a.dAltM, a.gsKt * kt_to_ms, a.track, a.vsMs, mc);
            a.level = a.tauS <= horizon_red_s ? 2 : a.tauS <= horizon_s ? 1 : 0;
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
    emit trafficUpdated();
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

static QList<Airspace> loadAirspaces(const QString &path, const QByteArray &data);

void Traffic::requestAtc()
{
    if (_atcReply || _atcWatcher.isRunning())
        return;
    QString cc = countryCode(f_country);
    if (cc.isEmpty())
        return;
    _atcCountry = cc;
    _atcRetryAtMs = 0;
    _atcStatus.clear();
    updateStatus();

    // the country file is kept in the storage, no download when it is there
    QString path = cacheFile(cc);
    if (QFileInfo(path).size() > 0) {
        f_country->setProgress(95);
        f_country->setDescr(tr("Loading airspaces..."));
        _atcWatcher.setFuture(QtConcurrent::run(loadAirspaces, path, QByteArray()));
        return;
    }
    if (_atcBlocked) {
        // offline: the cached countries only, no download until the plugin restarts
        f_country->setDescr(tr("Offline, not in the cache"));
        return;
    }

    QNetworkRequest req{QUrl(QString(atc_url).arg(cc))};
    req.setHeader(QNetworkRequest::UserAgentHeader, "apx-gcs");
    req.setTransferTimeout(atc_timeout_ms);
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

// worker thread: parse the downloaded data and keep it in the storage when it is valid,
// or (no data) parse the stored file
static QList<Airspace> loadAirspaces(const QString &path, const QByteArray &data)
{
    if (data.isEmpty()) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return {};
        return parseAirspaces(f.readAll());
    }
    auto list = parseAirspaces(data);
    if (list.isEmpty())
        return list; // not a valid file, nothing to keep
    QFileInfo fi(path);
    if (!QDir().mkpath(fi.absolutePath()))
        return list;
    QSaveFile f(path);
    if (f.open(QIODevice::WriteOnly) && f.write(data) == data.size())
        f.commit();
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
                // one try without a connection: the cached countries only until the plugin restarts
                _atcBlocked = true;
                f_country->setEnumStrings(cachedCountries());
                _atcStatus = tr("ATC: offline");
                f_country->setDescr(tr("Offline, cached countries only"));
            } else {
                _atcStatus = replyError("ATC", reply);
                f_country->setDescr(_atcStatus);
                _atcRetryAtMs = QDateTime::currentMSecsSinceEpoch() + atc_retry_ms;
            }
            updateStatus();
        }
        f_country->setProgress(-1);
        return;
    }
    setRequestDown(false);
    f_country->setProgress(95);
    f_country->setDescr(tr("Parsing airspaces..."));
    _atcWatcher.setFuture(QtConcurrent::run(loadAirspaces, cacheFile(_atcCountry), reply->readAll()));
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
    updateCacheInfo(); // a new file may have been kept
}

// cache on disk (as TileLoader::updateCacheInfo)

void Traffic::updateCacheInfo()
{
    if (_cacheInfoWatcher.isRunning()) {
        _cacheInfoDirty = true; // restart when finished
        return;
    }
    _cacheInfoDirty = false;
    const auto path = cachePath();
    _cacheInfoWatcher.setFuture(QtConcurrent::run([path]() {
        quint64 size = 0;
        uint cnt = 0;
        QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            size += it.fileInfo().size();
            cnt++;
        }
        return QPair<quint64, uint>(size, cnt);
    }));
}

void Traffic::cacheInfoReady()
{
    const auto r = _cacheInfoWatcher.result();
    f_cache->setValue(QString("%1 (%2 MB)").arg(r.second).arg(r.first / (1024 * 1024)));
    if (_cacheInfoDirty)
        updateCacheInfo();
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
