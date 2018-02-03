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
class FactTree: public QObject
{
  Q_OBJECT
  Q_ENUMS(ItemType)

  Q_PROPERTY(ItemType treeItemType READ treeItemType WRITE setTreeItemType NOTIFY treeItemTypeChanged)
  Q_PROPERTY(int size READ size NOTIFY sizeChanged)
  Q_PROPERTY(int num READ num NOTIFY numChanged)

  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

public:

  enum ItemType {
    RootItem =0,
    GroupItem,
    SectionItem,
    FactItem,
  };

  explicit FactTree(FactTree *parent, const QString &name, ItemType treeItemType);
  ~FactTree();

  //tree structure manipulation
  virtual void insertItem(int i, FactTree *item);
  virtual void removeItem(FactTree *item, bool deleteLater=true);

  //internal tree
  Q_INVOKABLE void addItem(FactTree *item);
  Q_INVOKABLE void moveItem(FactTree *item,int n,bool safeMode=false);
  Q_INVOKABLE FactTree * child(int n) const;
  Q_INVOKABLE FactTree * parentItem() const;
  Q_INVOKABLE int indexInParent() const;
  Q_INVOKABLE QList<FactTree*> childItems() const;

  Q_INVOKABLE FactTree * child(const QString &name) const;
  Q_INVOKABLE QString path(const QChar pathDelimiter=QChar('.')) const;

  QList<FactTree*> pathList() const;

  template<class T>
  T parent_cast() const
  {
    for(FactTree *i=parentItem();i;i=i->parentItem()){
      T p=qobject_cast<T>(i);
      if(p)return p;
    }
    return NULL;
  }

public slots:
  void remove();
  void removeAll();

signals:
  //tree structure change signals for models
  void itemToBeInserted(int row, FactTree *item);
  void itemInserted(FactTree *item);
  void itemToBeRemoved(int row,FactTree *item);
  void itemRemoved(FactTree *item);
  void itemToBeMoved(int row,int dest,FactTree *item);
  void itemMoved(FactTree *item);

  void removed();

private:
  QList<FactTree*> m_items;
  QPointer<FactTree> m_parentItem;
  QString makeNameUnique(const QString &s);
  QString nameSuffix;

  void updateNum();
  void setParentItem(FactTree *v);

  //-----------------------------------------
  //PROPERTIES
public:
  virtual ItemType treeItemType() const;
  virtual void setTreeItemType(const ItemType &v);
  virtual int size() const;
  virtual int num() const;

  QString name(void) const;
  void setName(const QString &v);

protected:
  ItemType m_treeItemType;
  QString  m_name;
  int m_num;

signals:
  void treeItemTypeChanged();
  void nameChanged();
  void sizeChanged();
  void numChanged();
  void parentItemChanged();
};
//=============================================================================
#endif
