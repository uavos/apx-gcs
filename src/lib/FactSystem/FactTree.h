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

public:
  //root
  explicit FactTree(QObject *parent, QString name, QString descr);
  //any
  explicit FactTree(FactTree *parent, QString name, QString descr, QString alias=QString());

  ~FactTree();

  //internal tree
  Q_INVOKABLE FactTree * child(int n);
  Q_INVOKABLE FactTree * parentItem() const;
  Q_INVOKABLE int num() const;

  //deep search
  Q_INVOKABLE FactTree * childByName(const QString &itemName) const;
  QList<FactTree*> allFields() const;
  Q_INVOKABLE FactTree * find(const QString &itemNamePath) const;
  Q_INVOKABLE FactTree * findByAlias(const QString &itemAlias) const;

  Q_INVOKABLE QString path(int fromLevel=0) const;

  //item type and status
  Q_INVOKABLE bool isField(void) const;
  Q_INVOKABLE bool isFieldsGroup(void) const;
  Q_INVOKABLE bool isRoot(void) const;

  //value
  Q_INVOKABLE QVariant & operator=(const QVariant &v){setValue(v);return m_value;}
  Q_INVOKABLE operator QVariant() const {return m_value;}


  //tree structure
  Q_INVOKABLE void addItem(FactTree *child);
  Q_INVOKABLE void removeItem(FactTree *child);

  enum MandalaListModelRoles {
    ItemRole = Qt::UserRole + 1,
    NameRole,
    ValueRole,
    DescrRole,
    ValueTextRole,
  };
  enum { //model columns
    FACT_ITEM_COLUMN_NAME=0,
    FACT_ITEM_COLUMN_VALUE,
    FACT_ITEM_COLUMN_DESCR,
  };

protected:
  //ListModel override
  virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
  virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
  virtual QHash<int, QByteArray> roleNames() const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

private:
  QList<FactTree*> m_items;

  FactTree *m_parentItem;

public slots:
  virtual void clear(void);
  virtual void reset(void);
signals:
  void structChanged(FactTree *item);


  //-----------------------------------------
  //PROPERTIES
public:
  Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)

  Q_PROPERTY(QString name READ name CONSTANT)
  Q_PROPERTY(QString descr READ descr CONSTANT)
  Q_PROPERTY(QString alias READ alias CONSTANT)

  Q_PROPERTY(int level READ level CONSTANT)

  Q_PROPERTY(QString valueText READ valueText NOTIFY valueChanged)

  Q_PROPERTY(int size READ size NOTIFY sizeChanged)

public:
  virtual QVariant value(void) const;
  virtual bool setValue(const QVariant &v);

  virtual QString name(void) const;
  virtual QString descr(void) const;
  virtual QString alias(void) const;

  int level(void) const;

  virtual QString valueText(void) const;

  int size() const;

protected:
  QVariant m_value;

  int m_level;

  QString  m_name;
  QString  m_descr;
  QString  m_alias;

signals:
  void valueChanged(FactTree *item);
  void usedChanged(FactTree *item);
  void sizeChanged(FactTree *item);
};
//=============================================================================
#endif
