/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "DatabaseLookupModel.h"

DatabaseLookupModel::DatabaseLookupModel(QObject *parent)
    : QAbstractListModel(parent)
    , ordered(true)
    , qmlMapSafe(false)
{}

int DatabaseLookupModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return _items.size();
}

QHash<int, QByteArray> DatabaseLookupModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ValuesRole] = "values";
    return roles;
}

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

int DatabaseLookupModel::count() const
{
    return rowCount();
}
QString DatabaseLookupModel::filter() const
{
    return m_filter;
}
void DatabaseLookupModel::setFilter(QString v)
{
    v = v.trimmed();
    if (m_filter == v)
        return;
    m_filter = v;
    emit filterChanged();
}
void DatabaseLookupModel::resetFilter()
{
    setFilter(QString());
}

void DatabaseLookupModel::syncItems(ItemsList items)
{
    //QTime t0;
    //t0.start();
    qDebug() << "size" << _items.size() << items.size();
    bool bReset = false;
    //find deleted
    QList<quint64> newKeys;
    for (int i = 0; i < items.size(); ++i) {
        newKeys.append(items.at(i).value("key").toULongLong());
    }
    for (int i = 0; i < _items.size(); ++i) {
        QVariantMap item = _items.at(i);
        quint64 key = item.value("key").toULongLong();
        if (newKeys.contains(key))
            continue;
        //if(items.contains(_items.at(i)))continue;
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
    //_items.reserve(items.size());
    //find inserted and moved
    for (int i = 0; i < items.size(); ++i) {
        QVariantMap item = items.at(i);
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
            if (idx > _items.size())
                idx = _items.size();
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
            int idx = (i >= _items.size() && i > 0) ? _items.size() - 1 : i;
            // beginMoveRows(QModelIndex(), j, j, QModelIndex(), idx);
            if (!bReset) {
                bReset = true;
                beginResetModel();
            }
            _items.removeAt(j);
            keys.removeAt(j);
            _items.insert(idx, item);
            keys.insert(idx, key);
            // endMoveRows();
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

int DatabaseLookupModel::indexOf(const QString &name, const QVariant &value) const
{
    for (int i = 0; i < _items.size(); ++i) {
        if (_items.at(i).value(name) == value)
            return i;
    }
    return -1;
}
