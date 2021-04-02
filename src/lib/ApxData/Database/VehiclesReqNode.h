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

#include "VehiclesDB.h"

class DBReqNode : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqNode(QString uid)
        : DBReqVehicles()
        , _uid(uid)
    {}

    virtual bool run(QSqlQuery &query) override;

    quint64 nodeID{};

protected:
    QString _uid;

signals:
    void foundID(quint64 key);
};

class DBReqSaveNodeInfo : public DBReqNode
{
    Q_OBJECT
public:
    explicit DBReqSaveNodeInfo(QVariantMap info)
        : DBReqNode(info.value("uid").toString())
        , _info(info)
    {}

    bool run(QSqlQuery &query) override;

private:
    QVariantMap _info;
};

class DBReqLoadNodeInfo : public DBReqNode
{
    Q_OBJECT
public:
    explicit DBReqLoadNodeInfo(QString uid)
        : DBReqNode(uid)
    {}

    bool run(QSqlQuery &query) override;

signals:
    void infoLoaded(QVariantMap info);
};

class DBReqSaveNodeDict : public DBReqNode
{
    Q_OBJECT
public:
    explicit DBReqSaveNodeDict(QString uid, QVariantMap dict)
        : DBReqNode(uid)
        , _dict(dict)
    {}

    bool run(QSqlQuery &query);

private:
    QVariantMap _dict;
};

class DBReqLoadNodeDict : public DBReqNode
{
    Q_OBJECT
public:
    //load cache
    explicit DBReqLoadNodeDict(QString uid, QString hash)
        : DBReqNode(uid)
        , _hash(hash)
    {}

    bool run(QSqlQuery &query);

private:
    QString _hash;
    QVariantList _dict;

signals:
    void dictLoaded(QVariantMap dict);
    void dictMissing(QString hash);
};

class DBReqSaveNodeConfig : public DBReqNode
{
    Q_OBJECT
public:
    explicit DBReqSaveNodeConfig(QString uid, QString hash, QVariantMap values, quint64 time = 0)
        : DBReqNode(uid)
        , _hash(hash)
        , _values(values)
        , _time(time ? time : QDateTime::currentDateTime().toMSecsSinceEpoch())
    {}

    bool run(QSqlQuery &query);

private:
    QString _hash;
    QVariantMap _values;
    quint64 _time;

    quint64 getValueID(QSqlQuery &query, const QVariant &v);
};

class DBReqLoadNodeConfig : public DBReqNode
{
    Q_OBJECT
public:
    explicit DBReqLoadNodeConfig(QString uid, QString hash) // latest if no hash
        : DBReqNode(uid)
        , _hash(hash)
    {}

    bool run(QSqlQuery &query);

protected:
    QString _hash;

signals:
    void configLoaded(QVariantMap values);
};
