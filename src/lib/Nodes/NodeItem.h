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
#ifndef NodeItem_H
#define NodeItem_H
//=============================================================================
#include <QtCore>
#include <QDomDocument>
#include "NodeItemData.h"
#include "NodeField.h"
class Nodes;
//=============================================================================
class NodeItem: public NodeItemData
{
  Q_OBJECT

  Q_PROPERTY(QString version READ version WRITE setVersion NOTIFY versionChanged)
  Q_PROPERTY(QString hardware READ hardware WRITE setHardware NOTIFY hardwareChanged)

  Q_PROPERTY(bool infoValid READ infoValid WRITE setInfoValid NOTIFY infoValidChanged)

public:
  explicit NodeItem(Nodes *parent,const QByteArray &sn);

  QString conf_hash;

  QList<NodeField*> allFields;

  QHash<QString,NodeField*> allFieldsDataMap();

  int timeout_ms;
  void request(uint cmd, const QByteArray &data, uint timeout_ms, bool highprio=false);

  void message(QString msg);

  //override
  QVariant data(int col, int role) const;
  void hashData(QCryptographicHash *h) const;

  Nodes *nodes;
  NodeItemBase *group;

  struct{
    bool valid;
    QList<uint> cmd;
    QStringList name;
    QStringList descr;
  }commands;
  void clearCommands();
  void cmdexec(int cmd_idx);

  uint lastSeenTime;

  bool skipCache;

private:
  QStringList sortNames;
  QTimer syncTimer;

  void groupFields(void);
  void groupNodes(void);
  void requestConf();

  void saveTelemetryUploadEvent();

private slots:
  void updateStats();
  void updateProgress();

  void validateData();
  void validateInfo();

  void updateReconf();

  void sync();
  void syncLater(int timeout=2000);

public slots:
  void nstat();
  void upload();

  void validateDict();
  void confWritten();

  //data comm
public slots:
  bool unpackService(uint ncmd, const QByteArray &ba);
signals:
  void nmtRequest(uint cmd,const QByteArray &sn,const QByteArray &data,uint timeout_ms, bool highprio);



  //---------------------------------------
  // PROPERTIES
public:
  QString version() const;
  void setVersion(const QString &v);
  QString hardware() const;
  void setHardware(const QString &v);
  bool infoValid() const;
  void setInfoValid(const bool &v);

protected:
  QString m_version;
  QString m_hardware;
  bool m_infoValid;

signals:
  void versionChanged();
  void hardwareChanged();
  void infoValidChanged();
};
//=============================================================================
#endif

