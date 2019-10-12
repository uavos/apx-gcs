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
#include "FactListModel.h"
#include "Fact.h"

#include <algorithm>
//=============================================================================
FactListModel::FactListModel(Fact *fact)
    : QAbstractListModel(fact)
    , fact(fact)
{
    syncTimer = new QTimer(this);
    syncTimer->setSingleShot(true);
    syncTimer->setInterval(200);
    connect(syncTimer, &QTimer::timeout, this, &FactListModel::sync);

    if (fact) {
        connectFact(fact);
        //populate(&_items, fact);
        scheduleSync();
    }
}
//=============================================================================
void FactListModel::connectFact(Fact *fact)
{
    connect(fact, &Fact::itemInserted, this, &FactListModel::scheduleSync, Qt::QueuedConnection);
    connect(fact, &Fact::itemRemoved, this, &FactListModel::scheduleSync);
    connect(fact, &Fact::itemMoved, this, &FactListModel::scheduleSync);
    connect(fact, &Fact::optionsChanged, this, &FactListModel::scheduleSync);
}
//=============================================================================int FactListModel::rowCount(const QModelIndex & parent) const
int FactListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return _items.size();
}
//=============================================================================
QHash<int, QByteArray> FactListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Fact::ModelDataRole] = "modelData";
    roles[Fact::FactRole] = "fact";
    roles[Fact::NameRole] = "name";
    roles[Fact::ValueRole] = "value";
    roles[Fact::TextRole] = "text";
    return roles;
}
//=============================================================================
Fact *FactListModel::get(int i) const
{
    return _items.value(i, nullptr);
}
//=============================================================================
QVariant FactListModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount())
        return QVariant();
    QPointer<Fact> item = _items.at(index.row());
    //if(!item)qDebug()<<"INVALID"<<fact;
    return item ? item->data(index.column(), role)
                : QVariant(); //FactSystem::instance()->data(index.column(),role);
}
//=============================================================================
bool FactListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.row() < 0 || index.row() >= rowCount() || role != Qt::EditRole)
        return false;
    Fact *item = _items.at(index.row());
    return item ? item->setValue(value) : false;
}
//=============================================================================
//=============================================================================
int FactListModel::count() const
{
    return rowCount();
}
//=============================================================================
void FactListModel::populate(ItemsList *list, Fact *fact)
{
    bool sect = false;
    for (int i = 0; i < fact->size(); ++i) {
        Fact *item = fact->child(i);
        if (!item)
            continue;
        if (!sect && !item->section().isEmpty())
            sect = true;
        if (fact->options() & Fact::FlatModel && (item->options() & Fact::Section)) {
            connect(item,
                    &Fact::itemInserted,
                    this,
                    &FactListModel::scheduleSync,
                    Qt::UniqueConnection);
            connect(item,
                    &Fact::itemRemoved,
                    this,
                    &FactListModel::scheduleSync,
                    Qt::UniqueConnection);
            connect(item, &Fact::itemMoved, this, &FactListModel::scheduleSync, Qt::UniqueConnection);
            connect(item,
                    &Fact::optionsChanged,
                    this,
                    &FactListModel::scheduleSync,
                    Qt::UniqueConnection);
            populate(list, item);
        } else
            list->append(item);
        connect(item, &FactBase::destroyed, this, &FactListModel::scheduleSync, Qt::UniqueConnection);
    }
    if (sect && fact == this->fact) {
        std::stable_sort(list->begin(), list->end(), [](Fact *a, Fact *b) {
            return a->section().localeAwareCompare(b->section()) < 0;
        });
    }
}
//=============================================================================
void FactListModel::scheduleSync()
{
    syncTimer->start();
}
void FactListModel::sync()
{
    ItemsList list;
    populate(&list, fact);
    syncModel(list);
}
void FactListModel::syncModel(const ItemsList &list)
{
    bool bLayoutChanged = false;
    //find deleted
    for (int i = 0; i < _items.size(); ++i) {
        if (list.contains(_items.at(i)))
            continue;
        beginRemoveRows(QModelIndex(), i, i);
        _items.removeAt(i);
        endRemoveRows();
        bLayoutChanged = true;
        //qDebug()<<"del"<<i<<this;
        emit countChanged();
        i--;
    }
    //find inserted and moved
    for (int i = 0; i < list.size(); ++i) {
        Fact *src = list.at(i);
        Fact *dest = _items.value(i, nullptr);
        if (src == dest)
            continue;
        int j = _items.indexOf(src);
        if (dest && j > 0) {
            //moved
            beginMoveRows(QModelIndex(), j, j, QModelIndex(), i);
            _items.removeAt(j);
            _items.insert(i, src);
            endMoveRows();
            bLayoutChanged = true;
            //qDebug()<<"mov"<<j<<i<<this;
        } else {
            //inserted
            beginInsertRows(QModelIndex(), i, i);
            _items.insert(i, src);
            endInsertRows();
            bLayoutChanged = true;
            //qDebug()<<"ins"<<i<<this;
            emit countChanged();
        }
    }
    if (bLayoutChanged)
        emit layoutChanged();
}
//=============================================================================
