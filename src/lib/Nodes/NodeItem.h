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
#include "NodeData.h"
#include "NodeField.h"
class Nodes;
//=============================================================================
class NodeItem: public NodeData
{
  Q_OBJECT
  Q_PROPERTY(bool valid READ valid WRITE setValid NOTIFY validChanged)
  Q_PROPERTY(bool dataValid READ dataValid WRITE setDataValid NOTIFY dataValidChanged)
  Q_PROPERTY(int progress READ progress WRITE setProgress NOTIFY progressChanged)

public:
  explicit NodeItem(Nodes *parent,const QByteArray &sn);


  Fact * f_version;
  Fact * f_hardware;

  Fact * f_fields;

  quint64 conf_uid;

  QList<NodeField*> allFields;

  void groupFields(void);

  int timeout_ms;
  void request(uint cmd, const QByteArray &data, uint timeout_ms, bool highprio=false);

private:
  Nodes *nodes;

private slots:
  void updateStats();

  //data comm
public slots:
  bool unpackService(uint ncmd, const QByteArray &ba);
signals:
  void nmtRequest(uint cmd,const QByteArray &sn,const QByteArray &data,uint timeout_ms, bool highprio);



  //---------------------------------------
  // PROPERTIES
public:
  bool valid() const;
  void setValid(const bool &v);
  bool dataValid() const;
  void setDataValid(const bool &v);
  int progress() const;
  void setProgress(const int &v);

protected:
  bool m_valid;
  bool m_dataValid;
  int m_progress;

signals:
  void validChanged();
  void dataValidChanged();
  void progressChanged();
};
//=============================================================================
#endif

