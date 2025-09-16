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
#include "PTraceListModel.h"

#define PTRACE_MAX_ROWS 1000

// Qt 6.7 compatibility
#if QT_VERSION < QT_VERSION_CHECK(6, 10, 0)
#define beginFilterChange()
#define endFilterChange() invalidateFilter()
#endif

PTraceListModel::PTraceListModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int PTraceListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_items.size();
}

QHash<int, QByteArray> PTraceListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[PTraceListModel::DataRole] = "blocks";
    return roles;
}

QVariant PTraceListModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount())
        return QVariant();
    const auto &item = m_items.at(index.row());
    switch (role) {
    case DataRole:
        return item;
    }
    return QVariant();
}

void PTraceListModel::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

void PTraceListModel::append(QStringList item)
{
    if (item.isEmpty())
        return;
    int row = rowCount();
    if (row > PTRACE_MAX_ROWS) {
        beginRemoveRows(QModelIndex(), 0, 0);
        m_items.removeAt(0);
        endRemoveRows();
    }
    row = rowCount();
    beginInsertRows(QModelIndex(), row, row);
    m_items.append(item);
    endInsertRows();
}

void PTraceListModel::updateItem(int row, QStringList value)
{
    if (row < 0 || row >= rowCount())
        return;
    // m_items.removeAt(row);
    // m_items.insert(row, value);
    m_items[row] = value;
    QModelIndex i = index(row, 0);
    emit dataChanged(i, i, QVector<int>() << DataRole);
}

void PTraceFilterProxyModel::add_filter(QString s)
{
    if (_filter.contains(s))
        return;

    beginFilterChange();
    _filter.append(s);
    endFilterChange();
}

void PTraceFilterProxyModel::remove_filter(QString s)
{
    beginFilterChange();
    _filter.removeAll(s);
    endFilterChange();
}

void PTraceFilterProxyModel::clear_filter()
{
    beginFilterChange();
    _filter.clear();
    endFilterChange();
}

bool PTraceFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
    QStringList blocks = sourceModel()->data(index0, PTraceListModel::DataRole).toStringList();
    for (const auto &i : blocks) {
        if (!i.startsWith('$'))
            continue;
        if (_filter.contains(i))
            return false;
    }
    return true;
}
