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
    Q_PROPERTY(QAbstractListModel *airspaces READ airspaces CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QStringList countries READ countries CONSTANT)
    // hex of the aircraft with the info panel pinned on the map (click), empty is none
    Q_PROPERTY(QString selected READ selected WRITE setSelected NOTIFY selectedChanged)

public:
    explicit Traffic(Fact *parent = nullptr);

    Fact *f_enabled;
    Fact *f_radius;
    Fact *f_country; // airspaces country code, empty is off

    QAbstractListModel *aircraft() const { return _aircraft; }
    QAbstractListModel *airspaces() const { return _airspaces; }
    QString status() const { return _status; }
    QStringList countries() const; // airspaces exports available on OpenAIP, "-" is off
    QString selected() const { return _selected; }
    void setSelected(const QString &hex);

    // map view center, used when the current unit has no position
    Q_INVOKABLE void setMapCenter(const QGeoCoordinate &c) { _mapCenter = c; }

private:
    AircraftModel *_aircraft;
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
    void filterAirspaces(bool force = false);
    void clearAirspaces();

    QString _selected;

    QString _status;
    QString _adsbStatus;
    QString _atcStatus;
    void updateStatus();

    QGeoCoordinate origin() const;
    double originAltM() const;

    void requestAdsb();
    void requestAtc();

private slots:
    void onEnabledChanged();
    void onTimer();
    void onAdsbReply();
    void onAtcReply();
    void onAtcParsed();

signals:
    void statusChanged();
    void selectedChanged();
};
