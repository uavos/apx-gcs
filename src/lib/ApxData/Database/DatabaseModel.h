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

#include <Database/DatabaseRequest.h>

#include <QtCore>

class DatabaseModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
    explicit DatabaseModel(QObject *parent = nullptr);

    using RecordsList = QList<quint64>;

    virtual QHash<int, QByteArray> roleNames() const;
    enum ItemRoles {
        ValuesRole = Qt::UserRole + 1,
    };

    Q_INVOKABLE QJsonObject get(int i) const;

    int count() const { return rowCount(); }

    QString filter() const { return _filter; }
    void setFilter(QString v);
    QString getFilterExpression(QStringList fields, QString extra_filter = {}) const;

    const RecordsList &recordsList() const { return _recordsList; }

    void setActiveRecordId(quint64 id);
    auto activeRecordId() const { return _activeRecordId; }

    //ListModel override
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

private:
    RecordsList _recordsList;
    quint64 _activeRecordId{};

    static constexpr int CACHE_SIZE = 300;
    QMap<quint64, QJsonObject> _cache;
    QQueue<quint64> _cacheQueue;

    QString _filter;

public slots:
    void setRecordsList(RecordsList recordsList);
    void setRecordInfo(quint64 id, QJsonObject info);

    void resetFilter() { setFilter({}); }

    void triggerItem(quint64 id);

signals:
    void countChanged(quint64 count);
    void itemTriggered(quint64 id);
    void filterChanged();
    void recordsListChanged();

    void requestRecordsList();
    void requestRecordInfo(quint64 id);
};
