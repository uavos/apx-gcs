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
#pragma once

#include "TrafficModels.h"

#include <Fact/Fact.h>
#include <QtConcurrent>
#include <QtCore>
#include <QtNetwork>
#include <QtPositioning>

// ADS-B traffic (readsb style API) and airspaces (OpenAIP) around the current unit
class Traffic : public Fact
{
    Q_OBJECT

    Q_PROPERTY(QAbstractListModel *aircraft READ aircraft CONSTANT)
    Q_PROPERTY(QAbstractItemModel *aircraftSorted READ aircraftSorted CONSTANT) // nearest first
    Q_PROPERTY(QAbstractListModel *airspaces READ airspaces CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    // no internet: the network is unreachable or the requests fail at the network layer
    Q_PROPERTY(bool offline READ offline NOTIFY offlineChanged)
    Q_PROPERTY(QStringList countries READ countries CONSTANT)
    // hex of the aircraft with the info panel pinned on the map (click), empty is none
    Q_PROPERTY(QString selected READ selected WRITE setSelected NOTIFY selectedChanged)

public:
    explicit Traffic(Fact *parent = nullptr);

    Fact *f_enabled;
    Fact *f_radius;
    Fact *f_country; // airspaces country code, empty is off
    Fact *f_own;     // ICAO hex or callsign of our aircraft (ADS-B), empty is off

    QAbstractListModel *aircraft() const { return _aircraft; }
    QAbstractItemModel *aircraftSorted() const { return _aircraftSorted; }
    QAbstractListModel *airspaces() const { return _airspaces; }
    QString status() const { return _status; }
    bool offline() const { return _offline; }
    QStringList countries() const; // airspaces exports available on OpenAIP, "-" is off
    QString selected() const { return _selected; }
    void setSelected(const QString &hex);

    // map view center, used when the current unit has no position
    Q_INVOKABLE void setMapCenter(const QGeoCoordinate &c) { _mapCenter = c; }

private:
    AircraftModel *_aircraft;
    QSortFilterProxyModel *_aircraftSorted;
    AirspaceModel *_airspaces;

    QNetworkAccessManager _net;
    QTimer _timer;
    QPointer<QNetworkReply> _adsbReply;
    QPointer<QNetworkReply> _atcReply;

    QGeoCoordinate _mapCenter;
    qint64 _adsbHoldMs{0}; // no ADS-B requests until this time (rate limit backoff)
    int _adsbBackoffMs{0};

    // airspaces: the country file is parsed in a worker thread and kept,
    // the model gets the part around the origin and is updated when the origin moves
    QFutureWatcher<QList<Airspace>> _atcWatcher;
    QList<Airspace> _atcAll;
    QString _atcCountry;   // country of _atcAll (and of the request in progress)
    qint64 _atcTimeMs{0};  // time of the last download
    QGeoCoordinate _atcFilterOrigin;
    double _atcFilterDist{0};
    QList<int> _atcShown;  // indexes of _atcAll in the model
    // the download failed without a connection: the country is locked, no retries
    // until the plugin is switched off and on
    bool _atcBlocked{false};
    void filterAirspaces(bool force = false);
    void clearAirspaces();
    void updateCountryDescr();

    // ICAO hex or callsign of our aircraft (trimmed), empty when not set
    QString ownId() const;
    void updateOwnDescr();

    QString _selected;

    QString _status;
    QString _adsbStatus;
    QString _atcStatus;
    void updateStatus();

    // offline is reported while the plugin is enabled and either the OS says the network
    // is unreachable or the last request failed below HTTP (no route, DNS, timeout).
    // ADS-B goes on at a slower rate while offline, a reply clears both flags.
    bool _offline{false};
    bool _netDown{false}; // QNetworkInformation reachability
    bool _reqDown{false}; // the last reply failed at the network layer
    static bool isNetworkDown(QNetworkReply *reply);
    void setRequestDown(bool v);
    void updateOffline();

    // the unit position, or the map center when the unit has no position
    QGeoCoordinate origin() const;
    static void abortReply(QNetworkReply *reply); // abort by the plugin, not a timeout

    void requestAdsb();
    void requestAtc();

private slots:
    void onEnabledChanged();
    void onTimer();
    void onAdsbReply();
    void onAtcReply();
    void onAtcParsed();
    void onReachabilityChanged();

signals:
    void statusChanged();
    void offlineChanged();
    void selectedChanged();
};
