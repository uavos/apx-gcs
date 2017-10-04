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
#ifndef NodesModel_H
#define NodesModel_H
//=============================================================================
#include <QtCore>
#include <QtWidgets>
#include <QItemDelegate>
#include <QSortFilterProxyModel>
#include <QDomDocument>
#include "QMandala.h"
#include "node.h"
#include "NodesItem.h"
#include "NodesItemNode.h"
#include "NodesItemField.h"
#include "NodesItemGroup.h"
#include "NodesItemNgrp.h"
#include "NodesView.h"
#include "NodesRequestManager.h"
typedef QList<NodesItemNode*> NodesList;
class QMandala;
//=============================================================================
class NodesModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  NodesModel(QMandalaItem *m,QObject *parent = 0);
  ~NodesModel();

  QMandalaItem *mvar;

  NodesRequestManager requestManager;

  static QAction *aUpload;

  QDir backup_dir;

  //members
  QHash<QByteArray,NodesItemNode *> nodes;  //by sn

  Q_INVOKABLE bool isValid(void) const;
  Q_INVOKABLE bool isEmpty(void) const;
  Q_INVOKABLE bool isModified(void) const;

  Q_INVOKABLE bool isUpgrading(void) const;
  Q_INVOKABLE bool isUpgradable(void) const;

  Q_INVOKABLE void saveToFile(QString fname) const;
  Q_INVOKABLE void loadFromFile(const QString &fname);
  Q_INVOKABLE void loadFromTelemetry(void);

  Q_INVOKABLE void restoreVehicleBackup(QString fname);

  void saveIdentToXml(QDomNode dom) const;

  Q_INVOKABLE QString title() const;


  NodesItem *item(const QModelIndex &index) const;

  QModelIndexList getGroupsIndexList(void);

  enum Roles {
    NodesItemRole=Qt::UserRole
  };

  void beginResetModel(){QAbstractItemModel::beginResetModel();}
  void endResetModel(){QAbstractItemModel::endResetModel();}

  //firmware upgrades
  typedef enum {UpgradeLoader,UpgradeFirmware,UpgradeMHX}UpgradeType;
  QMap<UpgradeType,NodesList> upgradeNodes;

public slots:
  void itemDataChanged(NodesItem *item);

private:
  QMandala *mandala;
  NodesItem *root;

  QModelIndexList getPersistentIndexList();

  QByteArray md5() const;

  //backup vehicles
  bool backup_dir_ok;
  QSettings *backup_sidx;

  //override
  QVariant data(const QModelIndex &index, int role) const;
  Qt::ItemFlags flags(const QModelIndex & index) const;
  QVariant headerData(int section, Qt::Orientation orientation,int role = Qt::DisplayRole) const;
  QModelIndex index(int row, int column,const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &index) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);


  QByteArray telemetry_hash;
  void saveToTelemetry(void);
  void saveToXml(QDomNode dom) const;
  int loadFromXml(QDomNode dom);
  uint importFromXml(QDomNode dom);

  int nodes_found_cnt;
  bool requestActive;

  //data requests server
private slots:
  void mandala_updated(uint var_idx); //for initial sync
  void service(const QByteArray &packet_data);

  void saveVehicleBackup();

  void firmwareUpgraded();

  void requestManagerFinished();

  //from telemetry player
  void replay_progress(uint time_ms);
private:
  QTimer syncTimer;

  void create_node(QByteArray sn, _node_info *ninfo);

public slots:
  void clear(void);
  void sync(void);
  void upload(void);
  void cacheClear(void);
  void stop(void);
  void stats(void);
  void upgradeFirmware(NodesList nodesList,UpgradeType utype);

signals:
  void syncProgress(uint);
  void changed(void);
  void done();
};
//=============================================================================
#endif
