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
#pragma once

#include <QtCore>
class ProtocolConverter;

class ProtocolBase : public QObject
{
    Q_OBJECT
public:
    ProtocolBase(QObject *parent = nullptr);

    virtual void setConverter(ProtocolConverter *c);

private:
    QTimer reqTimer;
    QList<QByteArray> reqList;

protected:
    ProtocolConverter *m_converter;
    void scheduleRequest(QByteArray packet);

protected slots:
    virtual void unpack(const QByteArray packet);

public slots:
    void downlinkData(QByteArray packet); //connect rx

    void send(QByteArray packet); //call to send data to tx

signals:
    void uplinkData(QByteArray packet); //connect tx

    //properties
public:
    int progress() const;
    void setProgress(int v);
    QString status() const;
    void setStatus(const QString &v);

private:
    int m_progress;
    QString m_status;
signals:
    void progressChanged();
    void statusChanged();
};
