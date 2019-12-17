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
#ifndef ProtocolService_H
#define ProtocolService_H
#include "ProtocolBase.h"
#include "ProtocolServiceNode.h"
#include "ProtocolServiceRequest.h"
#include <QtCore>
class ProtocolVehicle;
//=============================================================================
class ProtocolService : public ProtocolBase
{
    Q_OBJECT
public:
    explicit ProtocolService(ProtocolVehicle *vehicle);

    ProtocolServiceRequest *request(
        QString sn, quint16 cmd, QByteArray data, int timeout_ms, bool highprio, int retry = -1);
    ProtocolServiceNode *getNode(QString sn, bool createNew = true);

    ProtocolServiceRequest *acknowledgeRequest(QString sn,
                                               quint16 cmd,
                                               QByteArray data = QByteArray());

    void nodeUpgrading(const QString &sn);
    void nodeUpgradingFinished(const QString &sn);
    bool checkUpgrading();

private:
    ProtocolVehicle *vehicle;
    QList<ProtocolServiceRequest *> pool;
    QTimer timer;
    quint32 activeCount;
    QElapsedTimer reqTime;
    QTimer finishedTimer;

    static QStringList upgradingNodes;

    QHash<QString, ProtocolServiceNode *> nodes;

    void doNextRequestImmediately();

    void reset();

private slots:
    void doNextRequest();
    void requestFinished(ProtocolServiceRequest *request);
    void serviceData(QString sn, quint16 cmd, QByteArray data);
    void checkFinished();

public slots:
    void stop();

signals:
    void next();
    void sendServiceRequest(QString sn, quint16 cmd, QByteArray data = QByteArray());
    void stopRequested();
    void finished();

    //export signals and slots
public slots:
    //nodes dict sync
    void requestNodes();
    void clearNodes();
    void removeNode(QString sn);

    void rebootAll();

signals:
    void nodeFound(QString sn, ProtocolServiceNode *protocol);
    void requestTimeout(ProtocolServiceRequest *request, ProtocolServiceNode *node);
    void loaderServiceData(QString sn, quint16 cmd, QByteArray data);

    //properties
public:
    bool active() const;
    void setActive(bool v);

private:
    int m_active;
signals:
    void activeChanged();
};
//=============================================================================
#endif
