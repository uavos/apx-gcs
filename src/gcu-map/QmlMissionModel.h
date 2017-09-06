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
#ifndef QmlMissionModel_H
#define QmlMissionModel_H
//=============================================================================
#include <QtCore>
#include <QtQuick>
#include "MissionModel.h"
#include "QmlView.h"
#include "QmlMap.h"
//=============================================================================
class QmlMissionModel : public MissionModel
{
  Q_OBJECT

public:
  QmlMissionModel(QQuickItem *parent=0);


  //EXPOSED TO QML PROPERTIES
public:
  Q_PROPERTY(QQmlListProperty<MissionItemWp> waypoints READ waypoints NOTIFY waypointsChanged)
  QQmlListProperty<MissionItemWp> waypoints();
  Q_PROPERTY(QQmlListProperty<MissionItemRw> runways READ runways NOTIFY runwaysChanged)
  QQmlListProperty<MissionItemRw> runways();
  Q_PROPERTY(QQmlListProperty<MissionItemTw> taxiways READ taxiways NOTIFY taxiwaysChanged)
  QQmlListProperty<MissionItemTw> taxiways();
  Q_PROPERTY(QQmlListProperty<MissionItemPi> points READ points NOTIFY pointsChanged)
  QQmlListProperty<MissionItemPi> points();
signals:
  void waypointsChanged();
  void runwaysChanged();
  void taxiwaysChanged();
  void pointsChanged();
};
//=============================================================================
#endif
