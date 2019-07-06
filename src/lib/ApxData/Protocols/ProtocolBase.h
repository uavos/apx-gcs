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
#ifndef ProtocolBase_H
#define ProtocolBase_H
#include <QtCore>
//=============================================================================
class ProtocolBase : public QObject
{
    Q_OBJECT
public:
    ProtocolBase(QObject *parent = nullptr);

private:
    QTimer reqTimer;
    QList<QByteArray> reqList;

protected:
    void scheduleRequest(QByteArray packet);

public slots:
    virtual bool unpack(QByteArray packet);

signals:
    void sendUplink(QByteArray packet);

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
//=============================================================================
#endif
