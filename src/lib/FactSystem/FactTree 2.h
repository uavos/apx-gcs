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

  Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
  Q_CLASSINFO("DefaultProperty", "value")

public:
  enum ItemType {
    RootItem =0,
    GroupItem,
    FactItem,
  };


  //root
  explicit FactTree(QObject *parent, QString name, QString title, QString descr);
  //any
  explicit FactTree(FactTree *parent, QString name, QString title=QString(), QString descr=QString(), ItemType type=FactItem, QString alias=QString());

  ~FactTree();

  static const QChar pathDelimiter;

  //internal tree
  Q_INVOKABLE FactTree * child(int n) const;
  Q_INVOKABLE FactTree * parentItem() const;
  Q_INVOKABLE int num() const;
  Q_INVOKABLE QList<FactTree*> childItems() const;

  //deep search
  Q_INVOKABLE FactTree * childByName(const QString &itemName) const;
  QList<FactTree*> allFacts() const;
  Q_INVOKABLE FactTree * findByPath(const QString &itemNamePath) const;
  Q_INVOKABLE FactTree * findByAlias(const QString &itemAlias) const;
  Q_INVOKABLE FactTree * fact(const QString &factName) const;
  Q_INVOKABLE FactTree * valueEnumItem() const;

  Q_INVOKABLE QString path(int fromLevel=0) const;


  //item type and status
  Q_INVOKABLE bool isFact(void) const;
  Q_INVOKABLE bool isFactsList(void) const;
  Q_INVOKABLE bool isRoot(void) const;

  //data types test
  Q_INVOKABLE bool isBool(void) const;
  Q_INVOKABLE bool isText(void) const;

  //value
  //Q_INVOKABLE QVariant & operator=(const QVariant &v){setValue(v);return m_value;}
  //Q_INVOKABLE operator QVariant() const {return value();}

  //tree structure manipulation
  Q_INVOKABLE void addItem(FactTree *child);
  Q_INVOKABLE void removeItem(FactTree *child);

  enum FactModelRoles {
    ModelDataRole = Qt::UserRole + 1,
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

  ItemType itemType;


public slots:
  virtual void clear(void);
  virtual void reset(void);
  virtual void activate(void); //execute fact event (onClick)
signals:
  void structChanged(FactTree *item);
  void triggered();
  void closePage(); //to close dialog or page (add new item fact)


  //-----------------------------------------
  //PROPERTIES
public:

  Q_PROPERTY(QString name READ name NOTIFY nameChanged)
  Q_PROPERTY(QString title READ title NOTIFY titleChanged)
  Q_PROPERTY(QString descr READ descr NOTIFY descrChanged)
  Q_PROPERTY(QString alias READ alias CONSTANT)

  Q_PROPERTY(int level READ level CONSTANT)

  Q_PROPERTY(int size READ size NOTIFY sizeChanged)

  Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
  Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
  Q_PROPERTY(QString section READ section WRITE setSection NOTIFY sectionChanged)

  Q_PROPERTY(QString text READ text WRITE setText NOTIFY valueChanged)
public:
  virtual QVariant value(void) const;
  virtual bool setValue(const QVariant &v);

  virtual QString name(void) const;
  virtual QString title(void) const;
  virtual QString descr(void) const;
  virtual QString alias(void) const;

  virtual int level(void) const;

  virtual int size() const;

  virtual bool enabled() const;
  virtual void setEnabled(const bool &v);
  virtual bool visible() const;
  virtual void setVisible(const bool &v);

  virtual QString section() const;
  virtual void setSection(const QString &v);

  virtual QString text() const;
  virtual void setText(const QString &v);

protected:
  QVariant m_value;

  int m_level;

  QString  m_name;
  QString  m_title;
  QString  m_descr;
  QString  m_alias;

  bool m_enabled;
  bool m_visible;
  QString  m_section;

signals:
  void nameChanged(FactTree *item);
  void titleChanged(FactTree *item);
  void descrChanged(FactTree *item);

  void valueChanged(FactTree *item);
  void sizeChanged(FactTree *item);

  void enabledChanged(FactTree *item);
  void visibleChanged(FactTree *item);
  void sectionChanged(FactTree *item);
};
//=============================================================================
#endif
