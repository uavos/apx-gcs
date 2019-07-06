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
#include "DatabaseLookupModel.h"
//=============================================================================
DatabaseLookupModel::DatabaseLookupModel(QObject *parent)
    : QAbstractListModel(parent)
    , ordered(true)
    , qmlMapSafe(false)
{}
DatabaseLookupModel::~DatabaseLookupModel() {}
//=============================================================================
int DatabaseLookupModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return _items.size();
}
//=============================================================================
QHash<int, QByteArray> DatabaseLookupModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ValuesRole] = "values";
    return roles;
}
//=============================================================================
QVariantMap DatabaseLookupModel::get(int i) const
{
    return _items.value(i, QVariantMap());
}
bool DatabaseLookupModel::set(int i, QVariantMap v)
{
    if (i >= _items.size())
        return false;
    if (_items.at(i) == v)
        return false;
    _items[i] = v;
    emit dataChanged(index(i, 0), index(i, 0));
    emit itemEdited(i, v);
    return true;
}
//=============================================================================
QVariant DatabaseLookupModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount())
        return QVariant();
    switch (role) {
    default:
        break;
    case ValuesRole:
        return get(index.row());
    }
    return QVariant();
}
//=============================================================================
//=============================================================================
int DatabaseLookupModel::count() const
{
    return rowCount();
}
//=============================================================================
void DatabaseLookupModel::syncItems(ItemsList list)
{
    //QTime t0;
    //t0.start();
    //qDebug()<<"size"<<_items.size()<<list.size();
    bool bReset = false;
    //find deleted
    QList<quint64> newKeys;
    for (int i = 0; i < list.size(); ++i) {
        newKeys.append(list.at(i).value("key").toULongLong());
    }
    for (int i = 0; i < _items.size(); ++i) {
        QVariantMap item = _items.at(i);
        quint64 key = item.value("key").toULongLong();
        if (newKeys.contains(key))
            continue;
        //if(list.contains(_items.at(i)))continue;
        //qDebug()<<"rm row"<<i<<item.value("title").toString();
        if (qmlMapSafe && (!bReset)) {
            bReset |= i < (_items.size() - 1);
            if (bReset)
                beginResetModel();
        }
        beginRemoveRows(QModelIndex(), i, i);
        _items.removeAt(i);
        keys.removeAt(i);
        endRemoveRows();
        emit itemRemoved(i, item);
        emit countChanged();
        i--;
    }
    //_items.reserve(list.size());
    //find inserted and moved
    for (int i = 0; i < list.size(); ++i) {
        QVariantMap item = list.at(i);
        quint64 key = item.value("key").toULongLong();
        //arrange
        int j = keys.indexOf(key);
        if (j == i) {
            //no change in position
            //check data change
            if (_items.at(i) != item) {
                _items[i] = item;
                emit dataChanged(index(i, 0), index(i, 0));
            }
            /*QVariantMap myItem=_items.at(i);
      foreach(QString key, item.keys()){
        if(item.value(key).toString()==myItem.value(key).toString())continue;
        //change in data detected
        _items[i]=item;
        emit dataChanged(index(i,0),index(i,0));
        break;
      }*/
            continue;
        }
        if (j < 0) {
            //inserted
            int idx = ordered ? i : _items.size();
            //qDebug()<<"ins row"<<idx<<item.value("title").toString();
            beginInsertRows(QModelIndex(), idx, idx);
            _items.insert(idx, item);
            keys.insert(idx, key);
            endInsertRows();
            emit countChanged();
            if (qmlMapSafe && ordered && (!bReset)) {
                beginResetModel();
                bReset = true;
            }
        } else if (ordered) {
            //moved
            //qDebug()<<"mov row"<<i<<j<<item.value("title").toString();
            beginMoveRows(QModelIndex(), j, j, QModelIndex(), i);
            _items.removeAt(j);
            keys.removeAt(j);
            _items.insert(i, item);
            keys.insert(i, key);
            endMoveRows();
            if (qmlMapSafe && (!bReset)) {
                beginResetModel();
                bReset = true;
            }
        }
    }
    if (bReset) {
        endResetModel();
    } else {
        //check for changed data
    }
    emit synced();
    //qDebug()<<t0.elapsed()<<"ms";
}
//=============================================================================
int DatabaseLookupModel::indexOf(const QString &name, const QVariant &value) const
{
    for (int i = 0; i < _items.size(); ++i) {
        if (_items.at(i).value(name) == value)
            return i;
    }
    return -1;
}
//=============================================================================
