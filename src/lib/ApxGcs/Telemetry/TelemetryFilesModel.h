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
#pragma once

#include <QtCore>

class TelemetryFilesWorker;

class TelemetryFilesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
    explicit TelemetryFilesModel(TelemetryFilesWorker *worker,
                                 const QString &path,
                                 QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const;
    enum ItemRoles {
        ValuesRole = Qt::UserRole + 1,
    };

    Q_INVOKABLE QJsonObject get(int i) const;
    int count() const;

    QString filter() const;
    void setFilter(QString v);

    void setActiveId(int id);

protected:
    //ListModel override
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

private:
    TelemetryFilesWorker *_worker;
    QString _path;

    QStringList _filesList;
    void updateFilesList();

    QMap<int, QJsonObject> _cache;
    QQueue<int> _cacheQueue;
    QList<int> _cacheReq;
    void cacheInfo(QJsonObject info, int id);

    QString m_filter;
    int _activeId{-1};

public slots:
    void resetFilter();
    void updateFileInfo(QJsonObject info, int id);

signals:
    // void itemEdited(int i, QJsonObject v);
    // void itemRemoved(int i, QJsonObject v);
    // void synced();

signals:
    void countChanged();
    void filterChanged();
};
