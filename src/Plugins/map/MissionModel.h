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
#ifndef MissionModel_H
#define MissionModel_H
//=============================================================================
#include <QtCore>
#include <QtWidgets>
#include <QItemDelegate>
#include "QMandala.h"
#include "MissionItem.h"
#include "MissionItemField.h"
#include "MissionItemWp.h"
#include "MissionItemRw.h"
#include "MissionItemTw.h"
#include "MissionItemPi.h"
#include "MissionItemHome.h"
#include "MissionItemCategory.h"
#include "MissionItemArea.h"
class MapView;
class MissionDelegate;
//=============================================================================
class MissionModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  MissionModel(MapView *mapView);
  ~MissionModel();

  QString missionFileName;
  QString missionName;

  QItemSelectionModel *selectionModel;
  MissionDelegate *delegate;
  MapView *mapView;

  //members
  MissionItem *rootItem;

  MissionItemCategory<MissionItemWp> *waypoints;
  MissionItemCategory<MissionItemRw> *runways;
  MissionItemCategory<MissionItemTw> *taxiways;
  MissionItemCategory<MissionItemPi> *points;
  MissionItemCategory<MissionItemArea> *restricted;
  MissionItemCategory<MissionItemArea> *emergency;

  Q_INVOKABLE bool isEmpty(void) const;
  Q_INVOKABLE bool isModified(void) const;

  Q_INVOKABLE QList<MissionItem*> selectedItems(void) const;

  void beginResetModel(){QAbstractItemModel::beginResetModel();}
  void endResetModel(){QAbstractItemModel::endResetModel();}

  MissionItem *item(const QModelIndex &index) const;

  QModelIndex findIndex(MissionItem *item) const;

private:
  QModelIndexList getPersistentIndexList();

  //override
  QVariant data(const QModelIndex &index, int role) const;
  Qt::ItemFlags flags(const QModelIndex & index) const;
  QVariant headerData(int section, Qt::Orientation orientation,int role = Qt::DisplayRole) const;
  QModelIndex parent(const QModelIndex &index) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
  //moving items
  Qt::DropActions supportedDropActions() const;
  bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent);
public:
  QModelIndex index(int row, int column,const QModelIndex &parent = QModelIndex()) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
private:
  QTimer  updTimer;

  QList<QMetaObject::Connection>mcon;
private slots:
  void mandalaCurrentChanged(QMandalaItem *m);
  void emit_layoutChanged(){if(!updTimer.isActive())updTimer.start(100);}

  void mandala_data(uint8_t id,QByteArray data); //read mission from UAV

private:
  QByteArray telemetry_hash;
  void saveToTelemetry(void);

public slots:
  void clear(void);
  void upload(void);
  void request(void);

  void remove(); //all selected

  //files
  void newFile();
  void saveToFile(QString fname);
  void loadFromFile(const QString &fname);
  void loadFromTelemetry(void);
  void saveToXml(QDomNode dom) const;
  void loadFromXml(QDomNode dom);

  QByteArray pack() const;
  void unpack(const QByteArray &ba);

signals:
  void changed();
  void addedRemoved();
  void missionNameChanged(QString);
  //void fileLoaded();
};
//=============================================================================
class MissionDelegate : public QItemDelegate
{
  Q_OBJECT
public:
  MissionDelegate(QObject *parent = 0) : QItemDelegate(parent){}
  virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,const QModelIndex &index) const;
  void setEditorData(QWidget *editor, const QModelIndex &index) const;
  void setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const;
protected:
  QWidget *createEditorEx(QWidget *parent, const QStyleOptionViewItem &option,const QModelIndex &index,uint ftype,const QStringList opts) const;
};
//=============================================================================
#endif
