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
#include "TelemetryFilesModel.h"
#include "TelemetryFilesWorker.h"

#include <App/App.h>

static QString timestampToTitle(quint64 ts)
{
    return QDateTime::fromMSecsSinceEpoch(ts).toString("yyyy MMM dd hh:mm:ss");
}

TelemetryFilesModel::TelemetryFilesModel(TelemetryFilesWorker *worker,
                                         const QString &path,
                                         QObject *parent)
    : QAbstractListModel(parent)
    , _worker(worker)
    , _path(path)
{
    _updateTimer.setInterval(1000);
    _updateTimer.setSingleShot(true);
    connect(&_updateTimer, &QTimer::timeout, this, &TelemetryFilesModel::updateFilesList);

    _watcher.addPath(AppDirs::telemetry().absolutePath());
    connect(&_watcher, &QFileSystemWatcher::directoryChanged, this, [this]() {
        if (!_updateTimer.isActive())
            _updateTimer.start();
    });
}

void TelemetryFilesModel::setActiveId(int id)
{
    if (_activeId == id)
        return;

    auto oldId = _activeId;
    _activeId = id;

    // deselect old
    if (oldId >= 0 && oldId < _filesList.size())
        emit dataChanged(index(oldId, 0), index(oldId, 0), {ValuesRole});

    // select new
    if (id >= 0 && id < _filesList.size())
        emit dataChanged(index(id, 0), index(id, 0), {ValuesRole});
}

void TelemetryFilesModel::updateFilesList()
{
    qDebug() << "updating files list...";
    auto job = new TelemetryFilesJobList(_worker, _path);
    connect(job, &TelemetryFilesJobList::result, this, &TelemetryFilesModel::filesListUpdated);
    job->schedule();
}

void TelemetryFilesModel::filesListUpdated(QStringList files)
{
    if (_filesList == files)
        return;

    // remove old
    bool changed = false;
    for (auto i = 0; i < _filesList.size(); i++) {
        if (files.contains(_filesList.value(i)))
            continue;
        beginRemoveRows(QModelIndex(), i, i);
        _filesList.removeAt(i);
        _cache.clear();
        _cacheQueue.clear();
        endRemoveRows();
        changed = true;
        i--;
    }
    // add new
    for (auto i = 0; i < files.size(); i++) {
        if (_filesList.value(i) == files.value(i))
            continue;
        beginInsertRows(QModelIndex(), i, i);
        _filesList.insert(i, files.value(i));
        _cache.clear();
        _cacheQueue.clear();
        endInsertRows();
        changed = true;
    }
    if (changed)
        emit countChanged();
}

void TelemetryFilesModel::cacheInfo(QJsonObject info, int id)
{
    // qDebug() << info;

    if (id < 0 || id >= _filesList.size())
        return;

    if (_cache.contains(id))
        return;

    QString callsign = info["call"].toString();
    QString comment = info["conf"].toString();
    QString notes = info["notes"].toString();

    QString total;
    quint64 duration = info["duration"].toInteger();
    if (duration > 0)
        total = AppRoot::timeToString(duration / 1000);

    QStringList descr;
    if (!comment.isEmpty())
        descr << comment;
    if (!notes.isEmpty())
        descr << notes;

    QStringList value;
    if (!callsign.isEmpty())
        value << callsign;
    if (!total.isEmpty())
        value << total;

    info["id"] = id;
    info["title"] = timestampToTitle(info["timestamp"].toInteger());
    info["descr"] = descr.join(" - ");
    info["value"] = value.join(' ');

    // save cache
    _cache[id] = info;
    _cacheQueue.enqueue(id);

    if (_cacheQueue.size() > 500)
        _cache.remove(_cacheQueue.dequeue());

    _cacheReq.removeOne(id);

    emit dataChanged(index(id, 0), index(id, 0), {ValuesRole});
}
void TelemetryFilesModel::updateFileInfo(QJsonObject info, int id)
{
    _cache.remove(id);
    cacheInfo(info, id);
}

int TelemetryFilesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return _filesList.size();
}

QHash<int, QByteArray> TelemetryFilesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ValuesRole] = "values";
    return roles;
}

QJsonObject TelemetryFilesModel::get(int i) const
{
    if (i < 0 || i >= _filesList.size())
        return {};

    if (_cache.contains(i)) {
        auto m = _cache[i];
        m["active"] = i == _activeId;
        return m;
    }

    const auto &name = _filesList.at(i);

    // request data from worker
    if (!_cacheReq.contains(i)) {
        const_cast<TelemetryFilesModel *>(this)->_cacheReq.append(i);
        auto job = new TelemetryFilesJobInfo(_worker,
                                             QDir(_path).absoluteFilePath(name).append('.').append(
                                                 telemetry::APXTLM_FTYPE),
                                             i);
        connect(job, &TelemetryFilesJobInfo::result, this, &TelemetryFilesModel::cacheInfo);
        job->schedule();
    }

    // guess temporary data from file name

    QJsonObject m;
    m["id"] = i;
    m["name"] = name;
    auto timestamp = name.section('_', 0, 0).toLongLong();
    m["timestamp"] = timestamp;
    m["title"] = timestampToTitle(timestamp);
    m["value"] = name.section('_', 1, 1);
    m["descr"] = name;

    m["active"] = i == _activeId;

    return m;
}

QVariant TelemetryFilesModel::data(const QModelIndex &index, int role) const
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

int TelemetryFilesModel::count() const
{
    return rowCount();
}
QString TelemetryFilesModel::filter() const
{
    return m_filter;
}
void TelemetryFilesModel::setFilter(QString v)
{
    v = v.trimmed();
    if (m_filter == v)
        return;
    m_filter = v;
    emit filterChanged();
}
void TelemetryFilesModel::resetFilter()
{
    setFilter({});
}
