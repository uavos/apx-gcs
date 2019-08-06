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
#ifndef ProtocolServiceRequest_H
#define ProtocolServiceRequest_H
#include "ProtocolServiceNode.h"
#include <QtCore>
class ProtocolService;
//=============================================================================
class ProtocolServiceRequest : public QObject
{
    Q_OBJECT
public:
    explicit ProtocolServiceRequest(ProtocolService *service,
                                    const QString &sn,
                                    quint16 cmd,
                                    const QByteArray &data,
                                    int timeout_ms,
                                    bool highprio,
                                    int retry);

    QString sn;
    quint16 cmd;
    QByteArray data;
    int timeout_ms;
    int retryCnt;
    QTimer timer;
    bool active;
    bool highprio;

    bool acknowledged;

    bool equals(QString sn, uint cmd, QByteArray data);
    bool confirms(QString sn, uint cmd, QByteArray data);
    void finish(bool acknowledged = false);

private:
    ProtocolService *service;
    int retryNum;
    //QTime t;

private slots:
    void triggerTimeout();

public slots:
    void trigger();

signals:
    void sendServiceRequest(QString sn, quint16 cmd, QByteArray data = QByteArray());
    void finished(ProtocolServiceRequest *request);
    void retrying(int retry, int cnt);
    void timeout();
};
//=============================================================================
#endif
