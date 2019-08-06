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
#ifndef NodesReqVehicle_H
#define NodesReqVehicle_H
//=============================================================================
#include "NodesDB.h"
#include <QtCore>
//=============================================================================
class DBReqSaveVehicleInfo : public DBReqNodes
{
    Q_OBJECT
public:
    explicit DBReqSaveVehicleInfo(QVariantMap info)
        : DBReqNodes()
        , vehicleID(0)
        , info(info)
        , t(QDateTime::currentDateTime().toMSecsSinceEpoch())
    {}
    bool run(QSqlQuery &query);
    //result
    quint64 vehicleID;
    QVariantMap info;

private:
    quint64 t;
signals:
    void foundID(quint64 key);
};
//=============================================================================
class DBReqNodesSaveConfig : public DBReqNodes
{
    Q_OBJECT
public:
    explicit DBReqNodesSaveConfig(QList<quint64> nconfList,
                                  quint64 vehicleID,
                                  QString notes,
                                  quint64 t = 0)
        : DBReqNodes()
        , nconfList(nconfList)
        , vehicleID(vehicleID)
        , notes(notes)
        , t(t ? t : static_cast<quint64>(QDateTime::currentDateTime().toMSecsSinceEpoch()))
    {}
    bool run(QSqlQuery &query);
    //result
    QVariantMap configInfo;

private:
    QList<quint64> nconfList;
    quint64 vehicleID;
    QString notes;
    quint64 t;
signals:
    void configInfoFound(QVariantMap info);
    void configUpdated();
    void configCreated();
};
//=============================================================================
class DBReqNodesLoadConfig : public DBReqNodes
{
    Q_OBJECT
public:
    explicit DBReqNodesLoadConfig(QString hash)
        : DBReqNodes()
        , hash(hash)
    {}
    bool run(QSqlQuery &query);
    //result
    QVariantMap configInfo;
    QList<QVariantMap> data;

private:
    QString hash;
signals:
    void loaded(QVariantMap configInfo, QList<QVariantMap> data);
};
//=============================================================================
#endif
