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
#ifndef NodesItemField_H
#define NodesItemField_H
#include "NodesItem.h"
#include "node.h"
class NodesItemNode;
class PawnScript;
//=============================================================================
class NodesItemField: public NodesItem
{
  Q_OBJECT
public:
  NodesItemField(NodesItem *parent, NodesItemNode *node, uint fnum);
  NodesItemField(NodesItemField *field, NodesItemNode *node,QString name, QString descr, uint ftype, QStringList opts);
  ~NodesItemField();
  //override NodesItem
  QVariant data(int column,int role = Qt::DisplayRole) const;
  QVariant getValue(void) const;
  bool setValue(const QVariant &value);
  Qt::ItemFlags flags(int column) const;

  int getConfSize(void) const;

  QString typeToString(int t) const;
  QString units(void) const;

  void saveToXml(QDomNode dom) const;
  void saveValueToXml(QDomNode dom) const;
  bool saveOldValueToXml(QDomNode dom) const;
  void loadFromXml(QDomNode dom);

  //status tests
  Q_INVOKABLE bool isZero(void) const;
  Q_INVOKABLE bool isValid(void) const;
  Q_INVOKABLE bool isModified(void) const;
  Q_INVOKABLE bool isUpgrading(void) const;
  //methods
  Q_INVOKABLE void backup(void);
  Q_INVOKABLE void restore(void);
  Q_INVOKABLE void invalidate(void);
  Q_INVOKABLE void upload(void);

  //field parameters
  uint              fnum;  //field number in node for data exchanges
  uint              ftype; //float, uint, serial etc
  QStringList       opts;
  uint              array; //1=not array
  bool              dsc_valid;  //true if conf_inf received
  bool              data_valid;  //true if conf_read received
  QString           conf_name;  //parameter name as-is in conf struct
  QString           conf_descr;  //parameter name as-is in conf struct

  NodesItemNode     *node;
  NodesItemField    *field;  //part of another field item
  PawnScript        *script; //compiler

  const QByteArray & data() const; //binary data

  //data exchange
  void sync(void);
  void response_received(unsigned char cmd, const QByteArray data);

private:
  QByteArray ba_conf;
  QByteArray ba_conf_bkp;

signals:
  void request(uint cmd,const QByteArray sn,const QByteArray data,uint timeout_ms);

private slots:
  void arrange();
  void updateModelData();

public: //properties
  Q_PROPERTY(QVariant value READ getValue WRITE setValue)
};
//=============================================================================
#endif
