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
    explicit DBReqNode(quint64 nodeID)
        : DBReqVehicles()
        , nodeID(nodeID)
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
    explicit DBReqLoadNodeInfo(quint64 nodeID)
        : DBReqNode(nodeID)
    {}

    bool run(QSqlQuery &query) override;

    auto info() const { return _info; }

private:
    QVariantMap _info;

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
    explicit DBReqLoadNodeDict(QString uid, QString hash)
        : DBReqNode(uid)
        , _hash(hash)
    {}

    explicit DBReqLoadNodeDict(quint64 dictID)
        : DBReqNode(QString())
        , _dictID(dictID)
    {}

    bool run(QSqlQuery &query);

    auto dict() const { return _dict; }

private:
    QString _hash;
    quint64 _dictID{};

    QVariantMap _dict;

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

    auto configID() const { return _configID; }

private:
    QString _hash;
    QVariantMap _values;
    quint64 _time;
    quint64 _configID{};

    quint64 getValueID(QSqlQuery &query, const QVariant &v);

signals:
    void configSaved(quint64 configID);
};

class DBReqLoadNodeConfig : public DBReqNode
{
    Q_OBJECT
public:
    explicit DBReqLoadNodeConfig(QString uid, QString hash) // latest if no hash
        : DBReqNode(uid)
        , _hash(hash)
    {}
    explicit DBReqLoadNodeConfig(quint64 configID)
        : DBReqNode(QString())
        , _configID(configID)
    {}

    bool run(QSqlQuery &query);

    auto values() const { return _values; }
    auto time() const { return _time; }

protected:
    QString _hash;
    quint64 _configID{};
    quint64 _time{};

    QVariantMap _values;

signals:
    void configLoaded(QVariantMap values);
};
