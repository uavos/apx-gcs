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
#ifndef VehicleNmtManager_H
#define VehicleNmtManager_H
//=============================================================================
#include <QtCore>
class VehicleNmtManager;
//=============================================================================
class VehicleNmt : public QObject
{
  Q_OBJECT
public:
  VehicleNmt(uint cmd,const QByteArray sn,const QByteArray data,uint timeout_ms);

  bool equals(uint cmd,const QByteArray &sn,const QByteArray &data);

  uint cmd;
  QByteArray sn;
  QByteArray data;
  uint timeout_ms;
  uint retry;
  QTimer timer;
  bool active;

private slots:
  void timeout();
signals:
  void finished(VehicleNmt *request);
  void sendUplink(const QByteArray &packet);
public slots:
  void trigger();
  void nmtResponse(const QByteArray &packet);
};
//=============================================================================
class VehicleNmtManager : public QObject
{
  Q_OBJECT
public:
  VehicleNmtManager(QObject *parent =0);

private:
  static QList<VehicleNmt*> pool;
  static QTimer timer;
  uint activeCount;
private slots:
  void next();
  void requestFinished(VehicleNmt *request);

public slots:
  void request(uint cmd, const QByteArray &sn, const QByteArray &data, uint timeout_ms,bool highprio);
  void stop();

signals:
  void sendUplink(const QByteArray &packet);
  void nmtReceived(const QByteArray &packet); //from vehicle

  //PROPERTIES
public:
  Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
  bool enabled();
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  bool busy();
private:
  bool m_enabled;
  bool m_busy;
signals:
  void enabledChanged(bool);
  void busyChanged(bool);
public slots:
  Q_INVOKABLE void setEnabled(bool v);
};
//=============================================================================
#endif
