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
#ifndef FactData_H
#define FactData_H
//=============================================================================
#include "FactTree.h"
//=============================================================================
class FactData: public FactTree
{
  Q_OBJECT
  Q_ENUMS(DataType)
  Q_ENUMS(ActionType)

  Q_PROPERTY(DataType dataType READ dataType WRITE setDataType NOTIFY dataTypeChanged)

  Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)

  Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
  Q_PROPERTY(QString descr READ descr WRITE setDescr NOTIFY descrChanged)

  Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

  Q_PROPERTY(QStringList enumStrings READ enumStrings WRITE setEnumStrings NOTIFY enumStringsChanged)

public:

  enum DataType {
    NoData =0,      // no editor
    ConstData,
    TextData,
    FloatData,
    IntData,
    BoolData,
    EnumData,       // value=value of enumStrings (set by text or index or enumValues)
    ActionData,     // button, value=action type
    KeySequenceData,
  };

  enum ActionType {
    NormalAction =0,
    RemoveAction,
    UplinkAction,
  };

  explicit FactData(FactTree *parent, const QString &name, const QString &title, const QString &descr, ItemType treeItemType, DataType dataType);


  void copyValuesFrom(const FactData *item);

  int enumValue(const QVariant &v) const;
  QString enumText(int v) const;

  virtual void bind(FactData *item);



  //LIST MODEL
  enum FactModelRoles {
    ModelDataRole = Qt::UserRole + 1,
    NameRole,
    ValueRole,
    TextRole,
  };
  enum { //model columns
    FACT_ITEM_COLUMN_NAME=0,
    FACT_ITEM_COLUMN_VALUE,
    FACT_ITEM_COLUMN_DESCR,
  };

  //FactTree override
  virtual void insertItem(int i, FactTree *item);
  virtual void removeItem(FactTree *item, bool deleteLater=true);

signals:
  void childValueChanged(void);


protected:
  //ListModel override
  virtual QHash<int, QByteArray> roleNames() const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

  FactData *_bindedFact;

public:
  //---------------------------------------
  DataType dataType() const;
  void setDataType(const DataType &v);

  virtual QVariant value(void) const;
  Q_INVOKABLE virtual bool setValue(const QVariant &v);

  QString title(void) const;
  void setTitle(const QString &v);
  QString descr(void) const;
  void setDescr(const QString &v);

  QString text() const;
  void setText(const QString &v);

  QStringList enumStrings() const;
  void setEnumStrings(const QStringList &v, const QList<int> &enumValues=QList<int>());
  void setEnumStrings(const QMetaEnum &v);

protected:
  DataType m_dataType;

  QVariant m_value;

  QString  m_title;
  QString  m_descr;

  QStringList  m_enumStrings;
  QList<int>   m_enumValues;

signals:
  void dataTypeChanged();

  void valueChanged();

  void titleChanged();
  void descrChanged();

  void textChanged();
  void enumStringsChanged();
};
//=============================================================================
#endif
