/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef VehicleMission_H
#define VehicleMission_H
//=============================================================================
#include "MissionGroup.h"
#include <Fact/Fact.h>
#include <Vehicles/Vehicles.h>
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
//=============================================================================
class VehicleMission : public Fact
{
    Q_OBJECT
    Q_ENUMS(MissionItemType)

    Q_PROPERTY(QGeoCoordinate startPoint READ startPoint WRITE setStartPoint NOTIFY startPointChanged)
    Q_PROPERTY(double startHeading READ startHeading WRITE setStartHeading NOTIFY startHeadingChanged)
    Q_PROPERTY(double startLength READ startLength WRITE setStartLength NOTIFY startLengthChanged)

    Q_PROPERTY(MissionListModel *listModel READ listModel CONSTANT)

    Q_PROPERTY(int missionSize READ missionSize NOTIFY missionSizeChanged)
    Q_PROPERTY(bool empty READ empty NOTIFY emptyChanged)

    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)

    Q_PROPERTY(QString site READ site WRITE setSite NOTIFY siteChanged)

    Q_PROPERTY(bool synced READ synced NOTIFY syncedChanged)
    Q_PROPERTY(bool saved READ saved NOTIFY savedChanged)

    Q_PROPERTY(Fact *selectedItem READ selectedItem WRITE setSelectedItem NOTIFY selectedItemChanged)
public:
    explicit VehicleMission(Vehicle *parent);

    enum MissionItemType {
        WaypointType = 0,
        RunwayType,
        TaxiwayType,
        PoiType,
        AreaType,
    };

    Q_ENUM(MissionItemType)

    typedef MissionGroupT<Runway, RunwayType> Runways;
    typedef MissionGroupT<Waypoint, WaypointType> Waypoints;
    typedef MissionGroupT<Taxiway, TaxiwayType> Taxiways;
    typedef MissionGroupT<Poi, PoiType> Pois;
    typedef MissionGroupT<Area, AreaType> Areas;

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
    Fact *f_save;
    MissionShare *f_share;

    MissionTools *f_tools;

    Vehicle *vehicle;

    QList<MissionGroup *> groups;

    bool blockSizeUpdate;

    Q_INVOKABLE QGeoRectangle boundingGeoRectangle() const;

    //Fact override
    void backup();
    void restore();

    void hashData(QCryptographicHash *h) const;

private slots:
    void updateStatus();
    void updateActions();
    void updateStartPath();

public slots:
    void updateSize();
    void clearMission();
    void uploadMission();
    void test(int n = 50);

signals:
    void missionAvailable();
    void actionsUpdated();

    void missionDownloaded();
    void missionUploaded();

    //protocols
private slots:
    void missionDataReceived(DictMission::Mission d);
    void missionDataError();
signals:
    void missionDataUpload(DictMission::Mission d);

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

    bool saved() const;
    void setSaved(const bool v);

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
    void savedChanged();
    void selectedItemChanged();
};
//=============================================================================
#endif
