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

static constexpr int adsb_period_ms = 1000;
static constexpr int adsb_backoff_max_ms = 120000;
static constexpr int atc_period_ms = 10 * 60 * 1000;
static constexpr double alert_dist_km = 10;
static constexpr double alert_dalt_m = 600;
static constexpr double prox_dist_km = 25;

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

    auto updateCountryDescription = [this]() {
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
    };
    connect(f_country, &Fact::valueChanged, this, updateCountryDescription);
    updateCountryDescription();

    _aircraft = new AircraftModel(this);
    _aircraftSorted = new QSortFilterProxyModel(this);
    _aircraftSorted->setSourceModel(_aircraft);
    _aircraftSorted->setSortRole(AircraftModel::DistRole);
    _aircraftSorted->setDynamicSortFilter(true);
    _aircraftSorted->sort(0);
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

    connect(App::instance(), &App::loadingFinished, this, &Traffic::onEnabledChanged);

    loadQml("qrc:/" PLUGIN_NAME "/TrafficPlugin.qml");   // traffic list widget
    loadQml("qrc:/" PLUGIN_NAME "/TrafficMapLayer.qml"); // aircraft and airspaces on the map
}

// short status for the panel: "ADS-B: HTTP 429" or the error text
static QString replyError(const QString &src, QNetworkReply *reply)
{
    int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (code)
        return QString("%1: HTTP %2").arg(src).arg(code);
    return QString("%1: %2").arg(src).arg(reply->errorString().section(" - ", -1));
}

void Traffic::updateStatus()
{
    QString s = _adsbStatus;
    if (!_atcStatus.isEmpty())
        s += (s.isEmpty() ? "" : "  ") + _atcStatus;
    if (_status == s)
        return;
    _status = s;
    emit statusChanged();
}

void Traffic::onEnabledChanged()
{
    bool v = f_enabled->value().toBool();
    setActive(v);
    if (v) {
        _atcTimeMs = 0;
        _adsbHoldMs = 0;
        _adsbBackoffMs = 0;
        onTimer();
        _timer.start();
        return;
    }
    _timer.stop();
    if (_adsbReply)
        _adsbReply->abort();
    _aircraft->clear();
    clearAirspaces();
    _adsbStatus.clear();
    updateStatus();
}

void Traffic::clearAirspaces()
{
    if (_atcReply)
        _atcReply->abort();
    _atcAll.clear();
    _atcCountry.clear();
    _atcTimeMs = 0;
    _atcShown.clear();
    _atcFilterOrigin = QGeoCoordinate();
    _airspaces->reset({});
    f_country->setProgress(-1);
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

double Traffic::originAltM() const
{
    auto unit = Fleet::instance()->current();
    if (!unit || !unit->f_mandala)
        return 0;
    auto f = unit->f_mandala->fact(mandala::est::nav::pos::hmsl::uid);
    return f ? f->value().toDouble() : 0;
}

void Traffic::onTimer()
{
    if (!f_enabled->value().toBool() || !origin().isValid())
        return;
    auto now = QDateTime::currentMSecsSinceEpoch();

    if (now >= _adsbHoldMs)
        requestAdsb();

    // airspaces: the country file once per period, the visible part on every move
    auto cc = countryCode(f_country);
    if (cc.isEmpty())
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
    req.setTransferTimeout(10000);
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
        if (reply->error() == QNetworkReply::OperationCanceledError)
            return;
        _adsbStatus = replyError("ADS-B", reply);
        updateStatus();
        // rate limit: back off, Retry-After when given
        int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (code == 429 || code == 503) {
            int retry = reply->rawHeader("Retry-After").toInt() * 1000;
            _adsbBackoffMs = retry > 0 ? retry
                                       : qMin(adsb_backoff_max_ms,
                                              _adsbBackoffMs ? _adsbBackoffMs * 2
                                                             : adsb_period_ms * 2);
            _adsbHoldMs = QDateTime::currentMSecsSinceEpoch() + _adsbBackoffMs;
        }
        return;
    }
    _adsbBackoffMs = 0;

    auto root = QJsonDocument::fromJson(reply->readAll()).object();
    auto ac = root.value("aircraft").toArray(); // adsb.fi
    if (ac.isEmpty())
        ac = root.value("ac").toArray(); // adsb.lol, airplanes.live

    auto o = origin();
    double oAlt = originAltM();
    auto now = QDateTime::currentMSecsSinceEpoch();

    QList<Aircraft> list;
    for (const auto &v : ac) {
        auto j = v.toObject();
        Aircraft a;
        a.hex = j.value("hex").toString();
        a.coordinate = QGeoCoordinate(j.value("lat").toDouble(), j.value("lon").toDouble());
        if (a.hex.isEmpty() || !a.coordinate.isValid())
            continue;
        a.callsign = j.value("flight").toString().trimmed();
        a.type = j.value("t").toString();
        a.altFt = j.value("alt_baro").toDouble(); // "ground" -> 0
        a.gsKt = j.value("gs").toDouble();
        a.track = j.value("track").toDouble();
        a.vsFpm = j.value("baro_rate").toDouble();
        a.distKm = o.distanceTo(a.coordinate) / 1000.0;
        a.brg = o.azimuthTo(a.coordinate);
        a.seenMs = now;
        double dAlt = qAbs(a.altFt * 0.3048 - oAlt);
        if (a.distKm < alert_dist_km && dAlt < alert_dalt_m)
            a.level = 2;
        else if (a.distKm < prox_dist_km && dAlt < alert_dalt_m * 2)
            a.level = 1;
        list.append(a);
    }
    _aircraft->update(list);
    _adsbStatus.clear();
    updateStatus();
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
    req.setTransferTimeout(60000);
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
    int v = j.value("value").toInt();
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
        if (reply->error() != QNetworkReply::OperationCanceledError) {
            _atcStatus = replyError("ATC", reply);
            f_country->setDescr(_atcStatus);
            updateStatus();
        }
        f_country->setProgress(-1);
        return;
    }
    f_country->setProgress(95);
    f_country->setDescr(tr("Parsing airspaces..."));
    _atcWatcher.setFuture(QtConcurrent::run(parseAirspaces, reply->readAll()));
}

void Traffic::onAtcParsed()
{
    f_country->setProgress(-1);
    if (_atcCountry != countryCode(f_country))
        return; // country changed while parsing
    _atcAll = _atcWatcher.result();
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
