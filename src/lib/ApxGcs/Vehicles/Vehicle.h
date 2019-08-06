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
#ifndef Vehicle_H
#define Vehicle_H
//=============================================================================
#include <QGeoCoordinate>
#include <QtCore>

#include <Fact/Fact.h>
#include <Protocols/ProtocolVehicle.h>

#include <Xbus/XbusVehicle.h>

#include "VehicleMandala.h"

class Vehicles;
class Nodes;
class VehicleMission;
class VehicleWarnings;
class Telemetry;
//=============================================================================
class Vehicle : public Fact
{
    Q_OBJECT
    Q_ENUMS(VehicleClass)
    Q_ENUMS(StreamType)

    Q_PROPERTY(QString callsign READ callsign NOTIFY callsignChanged)
    Q_PROPERTY(quint16 squawk READ squawk NOTIFY squawkChanged)

    Q_PROPERTY(VehicleClass vehicleClass READ vehicleClass NOTIFY vehicleClassChanged)
    Q_PROPERTY(QString vehicleClassText READ vehicleClassText NOTIFY vehicleClassChanged)

    Q_PROPERTY(StreamType streamType READ streamType NOTIFY streamTypeChanged)
    Q_PROPERTY(QString streamTypeText READ streamTypeText NOTIFY streamTypeChanged)

    Q_PROPERTY(QString info READ info NOTIFY infoChanged)

    Q_PROPERTY(bool follow READ follow WRITE setFollow NOTIFY followChanged)

    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate NOTIFY coordinateChanged)

    Q_PROPERTY(bool flying READ flying NOTIFY flyingChanged)

public:
    enum VehicleClass {
        //must match the IDENT::_vclass type
        VEHICLE_CLASS_LIST,

        //internal use
        LOCAL = 100,
        REPLAY
    };
    Q_ENUM(VehicleClass)

    enum StreamType { OFFLINE = 0, SERVICE, DATA, XPDR, TELEMETRY };
    Q_ENUM(StreamType)

    explicit Vehicle(Vehicles *vehicles,
                     QString callsign,
                     quint16 squawk,
                     QString uid,
                     VehicleClass vclass,
                     ProtocolVehicle *protocol);

    VehicleMandala *f_mandala;
    Nodes *f_nodes;
    VehicleMission *f_mission;
    Telemetry *f_telemetry;
    VehicleWarnings *f_warnings;

    FactAction *f_select;

    QString uid;

    quint64 dbKey; //from db

    ProtocolVehicle *protocol;

    QString vehicleClassText() const;
    QString streamTypeText() const;
    QString squawkText() const;
    QString squawkText(quint16 v) const;

    QString fileTitle() const; //name based on Vehicle title and nodes shiva comment
    QString confTitle() const;

    Q_INVOKABLE bool isLocal() const;
    Q_INVOKABLE bool isReplay() const;

private:
    QTimer dlinkReqTimer;
    QTimer onlineTimer;
    QTime telemetryTime;
    QTime xpdrTime;
    QTime replayTime;

    QTimer updateInfoTimer;

    VehicleMandalaFact *f_gps_lat;
    VehicleMandalaFact *f_gps_lon;
    VehicleMandalaFact *f_gps_hmsl;
    VehicleMandalaFact *f_home_lat;
    VehicleMandalaFact *f_home_lon;
    VehicleMandalaFact *f_home_hmsl;

    VehicleMandalaFact *f_gps_Vdown;
    VehicleMandalaFact *f_mode;
    VehicleMandalaFact *f_stage;

    void setReplay(bool v);

private slots:
    void updateTitle();
    void updateStatus();
    void updateInfo();
    void updateInfoReq();
    void updateCoordinate();
    void updateFlying();

    void dbSetVehicleKey(quint64 key);

private slots:
    //protocols connection
    void setStreamXpdr();
    void setStreamTelemetry();
    void setStreamData();
    void setStreamService();

signals:
    //forward from protocols
    void downstreamDataReceived(); //used by widgets like signals
    void valueDataReceived();
    void serialDataReceived(uint portNo, QByteArray data); //for serial port

signals:
    //forward for recorder
    void recordDownlink();
    void recordUplink(Fact *f);
    //events
    void recordMessage(QString nodeName, QString text, QString sn);
    void recordConfigUpdate(QString nodeName, QString fieldName, QString value, QString sn);
    void recordSerialData(quint16 portNo, QByteArray data, bool uplink);

    //provided methods
public slots:
    void vmexec(QString func);
    void sendSerial(quint8 portID, QByteArray data);

    void flyHere(const QGeoCoordinate &c);
    void lookHere(const QGeoCoordinate &c);
    void setHomePoint(const QGeoCoordinate &c);
    void sendPositionFix(const QGeoCoordinate &c);

    //Database
public slots:
    void dbSaveVehicleInfo();

    //---------------------------------------
    // PROPERTIES
public:
    StreamType streamType(void) const;
    void setStreamType(const StreamType v);

    quint16 squawk(void) const;
    void setSquawk(const quint16 v);

    QString callsign(void) const;
    void setCallsign(const QString &v);

    VehicleClass vehicleClass(void) const;
    void setVehicleClass(const VehicleClass v);
    void setVehicleClass(const QString &v);

    QString info(void) const;

    bool follow(void) const;
    void setFollow(const bool &v);

    QGeoCoordinate coordinate(void) const;
    void setCoordinate(const QGeoCoordinate &v);

    bool flying(void) const;
    void setFlying(const bool &v);

protected:
    StreamType m_streamType;
    quint16 m_squawk;
    QString m_callsign;
    QString m_info;
    VehicleClass m_vehicleClass;
    bool m_follow;
    QGeoCoordinate m_coordinate;
    bool m_flying;

signals:
    void streamTypeChanged();
    void squawkChanged();
    void callsignChanged();
    void vehicleClassChanged();
    void infoChanged();
    void followChanged();
    void coordinateChanged();
    void flyingChanged();
};
//=============================================================================
#endif
