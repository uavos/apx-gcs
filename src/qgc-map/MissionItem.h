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
#ifndef MissionItem_H
#define MissionItem_H
//=============================================================================
#include <QtCore>
#include <QtWidgets>
#include <QDomDocument>
#include <QtQuick>
//=============================================================================
class MissionItem: public QObject
{
  Q_OBJECT
public:
  MissionItem(MissionItem *parent=NULL,QString name=QString(),QString caption=QString(),QString descr=QString());
  //virtual ~MissionItem(){}
  //virtual methods
  virtual QVariant data(int column,int role = Qt::DisplayRole) const;
  virtual QVariant value(void) const;
  virtual bool setValue(QVariant value);

  virtual bool isModified(void) const;

  //status
  virtual bool isZero(void) const;
  virtual void restore(void);

  virtual QStringList getToolTip(void) const;

  //methods
  void appendChild(MissionItem *child);
  void removeChild(MissionItem *child);
  MissionItem * find(QString iname, MissionItem *scope);
  void emit_changed(){emit changed();}

  Q_INVOKABLE MissionItem *child(const QString &vname) const;
  QList<MissionItem*> childItemsFlat(void) const;

  virtual QByteArray md5() const;

  //internal tree
  QList<MissionItem*> childItems;
  MissionItem *parentItem;
  MissionItem *child(int row);
  int childCount() const;
  virtual int columnCount() const;
  bool setData(int column, const QVariant & value);
  MissionItem *parent();
  virtual int row() const;
  virtual Qt::ItemFlags flags(int column) const;

  //columns
  enum{tc_field,tc_value};

  QString namePrefix;

  //item parameters
  virtual QString name() const;
  virtual QString caption() const;
  const QString & descr() const;
  void setDescr(const QString &v);
  virtual QString text() const;
private:
  QString m_caption;
  QString m_descr;
signals:
  void changed(void);
  void edited(void);

public slots:
  virtual void clear(void);
  virtual void remove(void);
  virtual void backup(void);
  virtual void invalidate(void);

  virtual void saveToXml(QDomNode dom) const;
  virtual void loadFromXml(QDomNode dom);

  virtual QByteArray pack() const;
  virtual int unpack(const QByteArray &ba);

private:
  static int itemsCount(QQmlListProperty<MissionItem>*list)
  {
    MissionItem *item=static_cast<MissionItem*>(list->object);
    if(item)return item->childCount();
    return 0;
  }
  static MissionItem* itemAt(QQmlListProperty<MissionItem> *list, int i)
  {
    MissionItem *item=static_cast<MissionItem*>(list->object);
    if(item)return qobject_cast<MissionItem*>(item->child(i));
    return 0;
  }

  //PROPERTIES
public:
  Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY changed)
  Q_PROPERTY(QString caption READ caption NOTIFY captionChanged)
  Q_PROPERTY(QString name READ name NOTIFY nameChanged)
  Q_PROPERTY(QString text READ text NOTIFY changed)
  Q_PROPERTY(QQmlListProperty<MissionItem> items READ items NOTIFY itemsChanged)
  QQmlListProperty<MissionItem> items();
signals:
  void nameChanged();
  void captionChanged();
  void itemsChanged();
};
//=============================================================================
#endif
