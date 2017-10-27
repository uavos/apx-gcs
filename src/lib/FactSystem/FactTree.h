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
#ifndef FactTree_H
#define FactTree_H
//=============================================================================
#include <QtCore>
//=============================================================================
class FactTree: public QAbstractListModel
{
  Q_OBJECT
  Q_ENUMS(ItemType)

  Q_PROPERTY(ItemType treeItemType READ treeItemType CONSTANT)
  Q_PROPERTY(int level READ level CONSTANT)
  Q_PROPERTY(int size READ size NOTIFY sizeChanged)
  Q_PROPERTY(bool flatModel READ flatModel WRITE setFlatModel NOTIFY flatModelChanged)

  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

public:

  enum ItemType {
    RootItem =0,
    GroupItem,
    SectionItem,
    FactItem,
    ConstItem,
  };

  explicit FactTree(FactTree *parent, const QString &name, ItemType treeItemType);

  //tree structure manipulation
  virtual void insertItem(int i, FactTree *item);
  virtual void removeItem(FactTree *item);

  //internal tree
  Q_INVOKABLE void addItem(FactTree *item);
  Q_INVOKABLE void remove();
  Q_INVOKABLE void moveItem(FactTree *item, int dest);
  Q_INVOKABLE int num() const;
  Q_INVOKABLE FactTree * child(int n) const;
  Q_INVOKABLE FactTree * parentItem() const;
  Q_INVOKABLE QList<FactTree*> childItems() const;
  Q_INVOKABLE QList<FactTree*> childItemsTree() const; //no flat model

  Q_INVOKABLE FactTree * child(const QString &name) const;
  Q_INVOKABLE QString path(int fromLevel=0,const QChar pathDelimiter=QChar('.')) const;

  Q_INVOKABLE FactTree * flatModelParent() const;

  QList<FactTree*> pathList() const;

public slots:
  virtual void clear(void);
signals:
  void structChanged();

  //signals forwarded to parents globally
  void itemRemoved(FactTree *item);
  void itemAdded(FactTree *item);

protected:
  //ListModel override
  virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
  virtual bool moveRows(const QModelIndex &sourceParent, int src, int cnt, const QModelIndex &destinationParent, int dst);

private:
  QList<FactTree*> m_items;
  FactTree *m_parentItem;

  //-----------------------------------------
  //PROPERTIES
public:
  virtual ItemType treeItemType() const;
  virtual int level(void) const;
  virtual int size() const;
  virtual bool flatModel() const;
  virtual void setFlatModel(const bool &v);

  QString name(void) const;
  void setName(const QString &v);

protected:
  ItemType m_treeItemType;
  int m_level;
  bool m_flatModel;
  QString  m_name;

signals:
  void nameChanged();
  void sizeChanged();
  void flatModelChanged();
};
//=============================================================================
#endif
