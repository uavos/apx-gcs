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
#ifndef NodesItemNode_H
#define NodesItemNode_H
#include <QScriptValue>
#include "NodesItem.h"
#include "node.h"
#include "FirmwareLoader.h"
class NodesModel;
class NodesItemField;
class BlackboxDownload;
//=============================================================================
class NodesItemNode: public NodesItem
{
  Q_OBJECT
public:
  NodesItemNode(NodesItem *parent,NodesModel *model, QByteArray sn, _node_info *ninfo);
  ~NodesItemNode();
  //override NodesItem
  QVariant data(int column,int role = Qt::DisplayRole) const;
  QVariant getValue(void) const;
  Qt::ItemFlags flags(int column) const;


  bool isValid(void) const;
  bool isReconf(void) const;
  bool isModified(void) const;

  bool isUpgrading(void) const;
  bool isUpgradable(void) const;
  bool isUpgradePending(void) const;

  int getConfSize(void) const;
  uint progress() const;

  void saveToXml(QDomNode dom) const;
  void loadFromXml(QDomNode dom);
  bool differentToXmlFile(QString fname) const;

  int loadFromOldFormat(QDomNode e);


  //methods
  QVariant valueByName(QString vname) const;

  QByteArray ba_conf;
  QByteArray ba_conf_bkp;

  //backup nodes
  QDir backup_dir;

  QByteArray md5() const;

  //node parameters
  const QByteArray  sn;
  bool              inf_valid;        //true if conf_inf received
  bool              cmd_valid;        //true if cmd received
  struct{
    QList<uint> cmd;
    QStringList name;
    QStringList descr;
  }commands;
  _node_info        node_info;
  _conf_inf         conf_inf;

  QList<NodesItemField*> fields;
  NodesModel  *model;


  FirmwareLoader firmwareLoader;

  //data exchange
  void response_received(unsigned char cmd, const QByteArray data);
  void sync(void);
  void command(int cmd_idx);
signals:
  void request(uint cmd,const QByteArray sn,const QByteArray data,uint timeout_ms);

private:
  //internal use
  bool backup_dir_ok,valid_state_s;
  QSettings *backup_sidx;

  void create_fields(void);

  void saveToCache(void);
  void loadFromCache(void);

  QTimer sync_apcfg_timer;
  void sync_apcfg_scr(NodesItem *item, QScriptValue mobj);

  QTimer chk_inf_timer;

  QTimer save_bkp_timer;

  //node stats
  _node_status node_status;
  bool statsWarn;
  bool statsInit;
  QTimer statsShowTimer;

  //bb download
  BlackboxDownload *blackboxDownload;

private slots:
  void sync_apcfg();
  void saveBackupFile(void);
  void chk_inf();
  void updateModelData();
  void blackboxDownloadFinished();


public slots:
  void field_changed();
  void clear(void);
  void stop(void);
  void upload(void);
  void stats(void);
  void restoreBackupFile(QString fname);
  bool restoreBackupXml(QDomNode dom);
  QString restoreRecentBackup(void);
};
//=============================================================================
#endif
