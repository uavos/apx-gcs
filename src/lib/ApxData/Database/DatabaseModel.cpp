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
#include "DatabaseModel.h"

#include <App/App.h>

DatabaseModel::DatabaseModel(QObject *parent)
    : QAbstractListModel(parent)
{}

void DatabaseModel::setActiveRecordId(quint64 id)
{
    if (_activeRecordId == id)
        return;

    auto oldId = _activeRecordId;
    _activeRecordId = id;

    // deselect old
    auto old_idx = _recordsList.indexOf(oldId);
    if (old_idx >= 0)
        emit dataChanged(index(old_idx, 0), index(old_idx, 0), {ValuesRole});

    // select new
    auto new_idx = _recordsList.indexOf(id);
    if (new_idx >= 0)
        emit dataChanged(index(new_idx, 0), index(new_idx, 0), {ValuesRole});
}

void DatabaseModel::setRecordsList(RecordsList recordsList)
{
    qDebug() << recordsList.size();
    beginResetModel();
    _recordsList = recordsList;
    endResetModel();
    emit countChanged(count());
}

void DatabaseModel::setRecordInfo(quint64 id, QJsonObject info)
{
    // qDebug() << info;

    if (!id)
        return;

    _cache[id] = info;
    _cacheQueue.enqueue(id);

    if (_cacheQueue.size() > CACHE_SIZE)
        _cache.remove(_cacheQueue.dequeue());

    auto idx = _recordsList.indexOf(id);
    if (idx < 0)
        return;

    emit dataChanged(index(idx, 0), index(idx, 0), {ValuesRole});
}

int DatabaseModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return _recordsList.size();
}

QHash<int, QByteArray> DatabaseModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ValuesRole] = "values";
    return roles;
}

QJsonObject DatabaseModel::get(int i) const
{
    if (i < 0 || i >= _recordsList.size())
        return {};

    auto id = _recordsList.at(i);
    if (_cache.contains(id)) {
        auto m = _cache.value(id);
        m["active"] = id == _activeRecordId;
        return m;
    }

    // request data from database
    emit const_cast<DatabaseModel *>(this)->requestRecordInfo(id);

    // guess temporary data from id
    QJsonObject m;
    m["id"] = (qint64) id;
    m["active"] = id == _activeRecordId;
    return m;
}

QVariant DatabaseModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount())
        return {};

    switch (role) {
    default:
        break;
    case ValuesRole:
        return get(index.row());
    }

    return {};
}

void DatabaseModel::setFilter(QString v)
{
    v = v.trimmed();
    if (_filter == v)
        return;
    _filter = v;
    emit filterChanged();

    emit requestRecordsList();
}

QString DatabaseModel::getFilterExpression(QStringList fields) const
{
    if (_filter.isEmpty())
        return {};

    QStringList parts;
    for (const auto &f : fields)
        parts.append(f + " LIKE '%" + _filter + "%'");

    return "(" + parts.join(" OR ") + ")";
}
