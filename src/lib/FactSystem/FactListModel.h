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
#ifndef FactListModel_H
#define FactListModel_H
//=============================================================================
#include <QtCore>
class Fact;
class FactTree;
//=============================================================================
class FactListModel: public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(bool flat READ flat WRITE setFlat NOTIFY flatChanged)

public:

  explicit FactListModel(Fact *parent);

  QList<FactTree*> items() const;

  QHash<int, QByteArray> roleNames() const;

private:
  Fact *fact;

  Fact * sectionParent(Fact *item) const;
  int sectionRow(Fact *parent, Fact *sect) const;

private slots:
  void itemToBeInserted(int row, FactTree *item);
  void itemInserted(FactTree *item);
  void itemToBeRemoved(int row,FactTree *item);
  void itemRemoved(FactTree *item);
  void itemToBeMoved(int row,int dest,FactTree *item);
  void itemMoved(FactTree *item);

protected:
  //ListModel override
  int rowCount(const QModelIndex & parent = QModelIndex()) const;
  QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);


  //-----------------------------------------
  //PROPERTIES
public:
  bool flat() const;
  void setFlat(const bool &v);
protected:
  bool m_flat;
signals:
  void flatChanged();
};
//=============================================================================
#endif
