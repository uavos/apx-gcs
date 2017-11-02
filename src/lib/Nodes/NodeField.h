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
#include "FactSystem.h"
class NodeFact;
//=============================================================================
class NodeField: public Fact
{
  Q_OBJECT
  Q_PROPERTY(bool valid READ valid WRITE setValid NOTIFY validChanged)
  Q_PROPERTY(int array READ array WRITE setArray NOTIFY arrayChanged)
  Q_PROPERTY(bool dataValid READ dataValid WRITE setDataValid NOTIFY dataValidChanged)

public:
  explicit NodeField(NodeFact *node,quint16 id);
  explicit NodeField(NodeFact *node,NodeField *parent, const QString &name, const QString &title, const QString &descr,int ftype);

  bool unpackService(uint ncmd, const QByteArray &data);

private:
  NodeFact *node;
  quint16 id;
  int ftype;

  bool unpackValue(const QByteArray &data);
  int ftypeSize() const;
  void updateDataType();

  void createSubFields(void);


  //---------------------------------------
  // PROPERTIES
public:
  bool valid() const;
  void setValid(const bool &v);
  int array() const;
  void setArray(const int &v);
  bool dataValid() const;
  void setDataValid(const bool &v);

protected:
  bool m_valid;
  int m_array;
  bool m_dataValid;

signals:
  void validChanged();
  void arrayChanged();
  void dataValidChanged();
};
//=============================================================================
#endif

