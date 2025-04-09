/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
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

#include "MissionGroup.h"
#include <Fact/Fact.h>
#include <Fleet/Fleet.h>

#include <QGeoCoordinate>
#include <QGeoRectangle>
#include <QtCore>

class MissionListModel;
class Waypoint;
class Runway;
class Taxiway;
class Poi;
class Area;
class LookupMissions;
class MissionShare;
class MissionStorage;
class MissionTools;

class UnitMission : public Fact
{
    Q_OBJECT

    Q_PROPERTY(QGeoCoordinate startPoint READ startPoint WRITE setStartPoint NOTIFY startPointChanged)
    Q_PROPERTY(double startHeading READ startHeading WRITE setStartHeading NOTIFY startHeadingChanged)
    Q_PROPERTY(double startLength READ startLength WRITE setStartLength NOTIFY startLengthChanged)

    Q_PROPERTY(MissionListModel *listModel READ listModel CONSTANT)

    Q_PROPERTY(int missionSize READ missionSize NOTIFY missionSizeChanged)
    Q_PROPERTY(bool empty READ empty NOTIFY emptyChanged)

    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)

    Q_PROPERTY(QString site READ site WRITE setSite NOTIFY siteChanged)

    Q_PROPERTY(bool synced READ synced NOTIFY syncedChanged)

    Q_PROPERTY(Fact *selectedItem READ selectedItem WRITE setSelectedItem NOTIFY selectedItemChanged)
public:
    explicit UnitMission(Unit *parent);

    typedef MissionGroupT<Runway> Runways;
    typedef MissionGroupT<Waypoint> Waypoints;
    typedef MissionGroupT<Taxiway> Taxiways;
    typedef MissionGroupT<Poi> Pois;
    typedef MissionGroupT<Area> Areas;

    Runways *f_runways;
    Waypoints *f_waypoints;
    Taxiways *f_taxiways;
    Pois *f_pois;
    Areas *f_areas;
    //MissionItems *f_restricted;
    //MissionItems *f_emergency;

    Fact *f_title;

    MissionStorage *storage;

    Fact *f_request;
    Fact *f_clear;
    Fact *f_upload;

    LookupMissions *f_lookup;
    MissionShare *f_share;

    MissionTools *f_tools;

    Unit *unit;

    QList<MissionGroup *> groups;

    bool blockSizeUpdate;

    Q_INVOKABLE QGeoRectangle boundingGeoRectangle() const;

    //Fact override
    QJsonValue toJson() override;
    void fromJson(const QJsonValue &jsv) override;

    void hashData(QCryptographicHash *h) const override;

private slots:
    void updateStatus();
    void updateActions();
    void updateStartPath();

public slots:
    void updateSize();
    void clearMission();

    void uploadMission();
    void downloadMission();

    void test(int n = 50);

signals:
    void actionsUpdated();

    void missionDownloaded();
    void missionUploaded();

    //protocols
private slots:
    void missionReceived(QVariant var);

    //---------------------------------------
    // PROPERTIES
public:
    QGeoCoordinate startPoint() const;
    void setStartPoint(const QGeoCoordinate &v);

    double startHeading() const;
    void setStartHeading(const double &v);

    double startLength() const;
    void setStartLength(const double &v);

    MissionListModel *listModel() const;

    int missionSize() const;
    void setMissionSize(const int v);

    bool empty() const;
    void setEmpty(const bool v);

    QGeoCoordinate coordinate() const;
    void setCoordinate(const QGeoCoordinate &v);

    QString site() const;
    void setSite(const QString &v);

    bool synced() const;
    void setSynced(const bool v);

    Fact *selectedItem() const;
    void setSelectedItem(Fact *v);

protected:
    QGeoCoordinate m_startPoint;
    double m_startHeading;
    double m_startLength;

    MissionListModel *m_listModel;

    int m_missionSize;
    bool m_empty;

    QGeoCoordinate m_coordinate;
    QString m_site;

    bool m_synced;
    bool m_saved;

    QPointer<Fact> m_selectedItem;

signals:
    void startPointChanged();
    void startHeadingChanged();
    void startLengthChanged();
    void missionSizeChanged();
    void emptyChanged();
    void coordinateChanged();
    void siteChanged();
    void syncedChanged();
    void selectedItemChanged();
};
