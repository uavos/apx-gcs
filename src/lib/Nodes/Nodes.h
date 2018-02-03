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
#ifndef Nodes_H
#define Nodes_H
//=============================================================================
#include <QtCore>
#include <QDomDocument>
#include "FactSystem.h"
#include "NodeItem.h"
#include "NodesXml.h"
class Vehicle;
typedef QList<NodeItem*> NodesList;
//=============================================================================
class Nodes: public Fact
{
  Q_OBJECT

  Q_PROPERTY(int nodesCount READ nodesCount NOTIFY nodesCountChanged)

public:
  explicit Nodes(Vehicle *parent);

  FactAction *f_upload;
  FactAction *f_request;
  FactAction *f_stop;
  FactAction *f_reload;

  Vehicle *vehicle;

  NodeItem * node(const QByteArray &sn){return snMap.value(sn);}

  //sn lookup
  QHash<QByteArray,NodeItem*> snMap;
  QList<NodeItemBase*> nGroups;

  NodeItem * nodeCheck(const QByteArray &sn);

  NodesXml *xml;

  //firmware upgrades
  typedef enum {UpgradeLoader,UpgradeFirmware,UpgradeMHX}UpgradeType;
  QMap<UpgradeType,NodesList> upgradeNodes;

private:

  bool isBroadcast(const QByteArray &sn) const;

private slots:
  void updateActions();

public slots:
  bool unpackService(const QByteArray &packet); //data comm

  void clear();
  void request();
  void reload();
  void upload();
  void stop();

  void updateProgress();

  void nstat();

  void clearCache();
signals:
  void actionsUpdated();


  //---------------------------------------
  // PROPERTIES
public:
  int nodesCount() const;

protected:
  int m_nodesCount;

signals:
  void nodesCountChanged();
};
//=============================================================================
#endif

