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
#ifndef NodeField_H
#define NodeField_H
//=============================================================================
#include <QtCore>
#include "NodeFieldBase.h"
class NodeItem;
class PawnScript;
//=============================================================================
class NodeField: public NodeFieldBase
{
  Q_OBJECT
  Q_PROPERTY(int array READ array WRITE setArray NOTIFY arrayChanged)

public:
  explicit NodeField(NodeItem *node,quint16 id);
  explicit NodeField(NodeItem *node,NodeField *parent, const QString &name, const QString &title, const QString &descr,int ftype);

  bool unpackService(uint ncmd, const QByteArray &data);

  QByteArray packValue() const;

  quint16 id;
  int ftype;

  QString ftypeString(int i=-1) const;

  //field path in node with groups
  QStringList groups;
  QString fpath(const QChar pathDelimiter=QChar('/')) const;

  //Fact override
  void setModified(const bool &v);

  //Mandala support
  QString mandalaToString(quint16 mid) const;
  quint16 stringToMandala(const QString &s) const;
  const QStringList * mandalaNames() const;

  void hashData(QCryptographicHash *h) const;

  NodeItem *node;
  PawnScript *script;

  void createSubFields(void);
private:
  NodeField *parentField;

  bool unpackValue(const QByteArray &data);
  int ftypeSize() const;
  void updateDataType();


private slots:
  void updateStatus();
  void validateDict();
  void validateData();

  //---------------------------------------
  // PROPERTIES
public:
  int array() const;
  void setArray(const int &v);

protected:
  int m_array;

signals:
  void arrayChanged();
};
//=============================================================================
#endif

