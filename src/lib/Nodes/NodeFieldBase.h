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
#ifndef NodeFieldBase_H
#define NodeFieldBase_H
//=============================================================================
#include <QtCore>
#include <FactSystem.h>
//=============================================================================
class NodeFieldBase: public Fact
{
  Q_OBJECT
  Q_PROPERTY(bool dictValid READ dictValid WRITE setDictValid NOTIFY dictValidChanged)
  Q_PROPERTY(bool dataValid READ dataValid WRITE setDataValid NOTIFY dataValidChanged)

public:
  explicit NodeFieldBase(Fact *parent, const QString &name, const QString &title, const QString &descr, ItemType treeItemType, DataType dataType);

protected:
  //override
  QVariant data(int col, int role) const;

  //---------------------------------------
  // PROPERTIES
public:
  bool dictValid() const;
  void setDictValid(const bool &v,bool recursive=true);
  bool dataValid() const;
  void setDataValid(const bool &v,bool recursive=true);

protected:
  bool m_dictValid;
  bool m_dataValid;

signals:
  void dictValidChanged();
  void dataValidChanged();
};
//=============================================================================
#endif

