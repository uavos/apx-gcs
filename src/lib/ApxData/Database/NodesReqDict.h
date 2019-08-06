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
#ifndef NodesReqDict_H
#define NodesReqDict_H
//=============================================================================
#include "NodesDB.h"
#include <QtCore>
//=============================================================================
class DBReqNodesLoadInfo : public DBReqNodes
{
    Q_OBJECT
public:
    explicit DBReqNodesLoadInfo(QString sn)
        : DBReqNodes(sn)
    {}

protected:
    bool run(QSqlQuery &query);
signals:
    void infoLoaded(QVariantMap info);
};
//=============================================================================
class DBReqNodesSaveInfo : public DBReqNodes
{
    Q_OBJECT
public:
    explicit DBReqNodesSaveInfo(QVariantMap info)
        : DBReqNodes(info.value("sn").toString())
        , info(info)
    {}
    bool run(QSqlQuery &query);

private:
    QVariantMap info;
};
//=============================================================================
class DBReqNodesSaveUser : public DBReqNodes
{
    Q_OBJECT
public:
    explicit DBReqNodesSaveUser(QString sn, QVariantMap info, qint64 t = 0)
        : DBReqNodes(sn)
        , info(info)
        , t(t > 0 ? t : QDateTime::currentDateTime().toMSecsSinceEpoch())
    {}
    bool run(QSqlQuery &query);

private:
    QVariantMap info;
    qint64 t;
};
class DBReqNodesLoadUser : public DBReqNodes
{
    Q_OBJECT
public:
    explicit DBReqNodesLoadUser(QString sn)
        : DBReqNodes(sn)
    {}
    //result
    QVariantMap info;
    bool run(QSqlQuery &query);
};
//=============================================================================
class DBReqNodesLoadDict : public DBReqNodes
{
    Q_OBJECT
public:
    //load cache
    explicit DBReqNodesLoadDict(QString sn, QString chash)
        : DBReqNodes(sn)
        , dictID(0)
        , chash(chash)
    {}
    explicit DBReqNodesLoadDict(quint64 dictID)
        : DBReqNodes()
        , dictID(dictID)
    {}
    bool run(QSqlQuery &query);
    //result
    QVariantMap info;
    DictNode::Dict dict;

private:
    quint64 dictID;
    QString chash;
signals:
    void dictInfoFound(QVariantMap dictInfo);
    void dictLoaded(QVariantMap info, DictNode::Dict dict);
};
//=============================================================================
class DBReqNodesSaveDict : public DBReqNodes
{
    Q_OBJECT
public:
    explicit DBReqNodesSaveDict(QVariantMap info, const DictNode::Dict &dict)
        : DBReqNodes(info.value("sn").toString())
        , info(info)
    {
        makeRecords(dict);
    }
    bool run(QSqlQuery &query);
    //result
    QVariantMap info;

private:
    Records records;
    void makeRecords(const DictNode::Dict &dict);
signals:
    void dictInfoFound(QVariantMap dictInfo);
};
//=============================================================================
#endif
