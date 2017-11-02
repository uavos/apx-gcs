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
#ifndef NodeFact_H
#define NodeFact_H
//=============================================================================
#include <QtCore>
#include "NodeData.h"
#include "NodeField.h"
class Nodes;
//=============================================================================
class NodeFact: public NodeData
{
  Q_OBJECT
  Q_PROPERTY(bool valid READ valid WRITE setValid NOTIFY validChanged)

public:
  explicit NodeFact(Nodes *parent,const QByteArray &sn);

  bool unpackService(uint ncmd, const QByteArray &ba);

  Fact * f_version;
  Fact * f_hardware;

  Fact * f_fields;

  quint64 conf_uid;

  QList<NodeField*> allFields;

  void groupFields(void);

private slots:
  void updateStats();

  //data comm
signals:



  //---------------------------------------
  // PROPERTIES
public:
  bool valid() const;
  void setValid(const bool &v);

protected:
  bool m_valid;

signals:
  void validChanged();
};
//=============================================================================
#endif

