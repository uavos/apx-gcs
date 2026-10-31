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

#include <QtCore>
#include <QtPositioning>

// ADS-B aircraft around the origin (current unit or map center)
struct Aircraft
{
    QString hex;
    QString callsign;
    QString type;     // ICAO type code, "A320"
    QString category; // ADS-B emitter category, "A3"
    QGeoCoordinate coordinate;
    bool onGround{false};
    double altBaroM{qQNaN()}; // pressure altitude (std 1013), m
    double altGeomM{qQNaN()}; // GNSS altitude (WGS84), m
    double gsKt{0};           // ground speed, kt
    double track{0};          // deg
    double vsMs{0};           // vertical speed, m/s
    double distKm{0};         // from origin
    double brg{0};            // from origin, deg
    int level{0};             // 0 other, 1 proximity, 2 alert
    qint64 seenMs{0};         // last reply that contained the aircraft

    // altitude used for display and alerts: GNSS when available, baro otherwise
    double altM() const { return qIsNaN(altGeomM) ? altBaroM : altGeomM; }
    QString altSrc() const { return qIsNaN(altGeomM) ? "BARO" : "GNSS"; }
};

class AircraftModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        HexRole = Qt::UserRole + 1,
        CallsignRole,
        TypeRole,
        CategoryRole,
        CategoryTextRole,
        IconRole,
        IconScaleRole,
        CoordinateRole,
        OnGroundRole,
        AltRole,     // altM, m
        AltSrcRole,  // "BARO" or "GNSS"
        AltBaroRole, // m, NaN when unknown
        AltGeomRole, // m, NaN when unknown
        GsRole,      // kt
        TrackRole,
        VsRole, // m/s
        DistRole,
        BrgRole,
        LevelRole,
    };

    explicit AircraftModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // merge a complete reply: update, insert and drop missing (by hex)
    void update(const QList<Aircraft> &list);
    void clear();
    bool contains(const QString &hex) const;

private:
    QList<Aircraft> _items;
};

// airspace polygon from the ATC data provider
struct Airspace
{
    QString name;
    QString label; // "CTR SFC-3500 ft"
    QString color;
    QGeoCoordinate center;
    QVariantList path; // closed ring of QGeoCoordinate for MapPolyline
    // bounding box for the fast distance filter
    double minLat{0}, maxLat{0}, minLon{0}, maxLon{0};
};

class AirspaceModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        LabelRole,
        ColorRole,
        CenterRole,
        PathRole,
    };

    explicit AirspaceModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void reset(const QList<Airspace> &list);

private:
    QList<Airspace> _items;
};
