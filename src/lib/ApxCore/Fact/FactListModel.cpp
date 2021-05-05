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
#include "FactListModel.h"
#include "Fact.h"

#include <App/App.h>
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
    connect(fact, &Fact::triggered, this, &FactListModel::resetFilter);
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
    Fact *item = qobject_cast<Fact *>(_items.at(index.row()));
    //if(!item)qDebug()<<"INVALID"<<fact;
    QVariant ret = item ? item->data(index.column(), role) : QVariant();
    if (role == Qt::FontRole) {
        auto font = ret.value<QFont>();
        font.setPixelSize(App::font().pixelSize());
    }
    return ret;
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
QString FactListModel::filter() const
{
    return m_filter;
}
void FactListModel::setFilter(QString v)
{
    v = v.trimmed();
    if (m_filter == v)
        return;
    m_filter = v;
    emit filterChanged();
    syncTimer->start();
}
void FactListModel::resetFilter()
{
    setFilter(QString());
}
//=============================================================================
void FactListModel::populate(ItemsList *list, Fact *f)
{
    bool flt = (this->fact->options() & Fact::FilterModel) && !filter().isEmpty();
    bool sect = false;
    for (int i = 0; i < f->size(); ++i) {
        Fact *item = f->child(i);
        if (!item)
            continue;
        if (flt && item->treeType() != Fact::Group
            && !item->showThis(QRegExp(filter(), Qt::CaseInsensitive)))
            continue;
        if (!sect && !item->section().isEmpty())
            sect = true;
        connect(item, &FactBase::destroyed, this, &FactListModel::scheduleSync, Qt::UniqueConnection);

        if ((f->options() & Fact::FlatModel) && (item->options() & Fact::Section)) {
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
            continue;
        }
        if (flt && item->treeType() == Fact::Group) {
            populate(list, item);
        } else {
            list->append(item);
        }
    }
    if (sect && f == this->fact) {
        //check for gaps in sections to sort
        QString s = "@/./";
        QStringList slist;
        for (auto i : *list) {
            const QString &si = i->section();
            if (s == si)
                continue;
            s = si;
            if (!slist.contains(s)) {
                slist.append(s);
                continue;
            }
            //gap found
            std::stable_sort(list->begin(), list->end(), [](Fact *a, Fact *b) {
                return a->section().localeAwareCompare(b->section()) < 0;
            });
            break;
        }
    }
}
//=============================================================================
void FactListModel::scheduleSync()
{
    resetFilter();
    syncTimer->start();
}
void FactListModel::sync()
{
    syncTimer->stop();
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
