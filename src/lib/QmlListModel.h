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
#ifndef QmlListModel_H
#define QmlListModel_H
#include <QtCore>
#include <QQmlEngine>
//=============================================================================
/*template<class T>
class xQmlListModel : public QAbstractListModel
{
public:
  enum xQmlListModelRoles {
      ItemRole = Qt::UserRole + 1
  };
  xQmlListModel(QObject *parent = 0)
    : QAbstractListModel(parent)
  {
  }

  Q_INVOKABLE virtual void addItem(T *item)
  {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_items << item;
    endInsertRows();
  }
  Q_INVOKABLE void removeItem(T *item)
  {
    int i=m_items.indexOf(item);
    if(i<0)return;
    beginRemoveRows(QModelIndex(), i, i);
    m_items.removeOne(item);
    endRemoveRows();
    item->deleteLater();
  }

  int rowCount(const QModelIndex & parent = QModelIndex()) const
  {
    Q_UNUSED(parent);
    return m_items.count();
  }

  QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const
  {
    if (index.row() < 0 || index.row() >= m_items.count())
      return QVariant();
    if (role == ItemRole) return QVariant::fromValue(m_items[index.row()]);
    return QVariant();
  }

  T *item(int row) const
  {
    if(row>m_items.count())return NULL;
    return m_items[row];
  }

protected:
  QHash<int, QByteArray> roleNames() const
  {
    QHash<int, QByteArray> roles;
    roles[ItemRole] = "item";
    return roles;
  }

private:
  QList<T*> m_items;
};*/
//=============================================================================
class QmlListModel : public QAbstractListModel
{
  //Q_OBJECT
public:
  explicit QmlListModel(QObject *parent = 0)
    : QAbstractListModel(parent)
  {
  }

  QObject *item(int row) const
  {
    if(row>m_items.count())return NULL;
    return m_items[row];
  }

  enum QmlListModelRoles {
      ItemRole = Qt::UserRole + 1
  };

protected:
  virtual void addItem(QObject *item)
  {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_items << item;
    endInsertRows();
  }
  virtual void removeItem(QObject *item)
  {
    int i=m_items.indexOf(item);
    if(i<0)return;
    beginRemoveRows(QModelIndex(), i, i);
    m_items.removeOne(item);
    endRemoveRows();
    item->deleteLater();
  }

  int rowCount(const QModelIndex & parent = QModelIndex()) const
  {
    Q_UNUSED(parent);
    return m_items.count();
  }

  QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const
  {
    if (index.row() < 0 || index.row() >= m_items.count())
      return QVariant();
    if (role == ItemRole) return QVariant::fromValue(m_items[index.row()]);
    return QVariant();
  }

protected:
  QHash<int, QByteArray> roleNames() const
  {
    QHash<int, QByteArray> roles;
    roles[ItemRole] = "item";
    return roles;
  }

private:
  QList<QObject*> m_items;
};
//=============================================================================
#endif
