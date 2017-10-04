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
#ifndef NODESREQUESTMANAGER_H
#define NODESREQUESTMANAGER_H
//=============================================================================
#include <QtCore>
class NodesModel;
class NodesRequestManager;
//=============================================================================
class NodesRequest : public QObject
{
  Q_OBJECT
public:
  NodesRequest(uint cmd,const QByteArray sn,const QByteArray data,uint timeout_ms);

  uint cmd;
  QByteArray sn;
  QByteArray data;
  uint timeout_ms;
  uint retry;
  QTime time;
  QTimer timer;
  bool active;
private:
  bool forceNext;
private slots:
  void timerTimeout();
signals:
  void done(const QByteArray &packet_data,bool forceNext);
  void send(uint cmd,const QByteArray &sn,const QByteArray &data);
  void serviceDataReceived(const QByteArray &packet_data,bool accepted);
public slots:
  void trigger();
  void serviceData(const QByteArray &packet_data);
};
//=============================================================================
class NodesRequestManager : public QObject
{
  Q_OBJECT
public:
  NodesRequestManager(NodesModel *model);
private:
  NodesModel *model;
  QList<NodesRequest*>pool;
  QTimer timer;
  bool emitDone;
  QTimer doneTimer;
  bool disableOnDone;
  QTimer disableTimer;
  uint activeCount;
private slots:
  void next();
  void requestDone(const QByteArray &packet_data,bool forceNext);
  void allDone();
  void disableTimerTimeout();
signals:
  void finished();
  void serviceDataReceived(const QByteArray &packet_data,bool accepted);
public slots:
  void makeRequest(uint cmd,const QByteArray &sn,const QByteArray &data,uint timeout_ms);
  void stop();
  void enableTemporary();

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
