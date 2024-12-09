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

class DatabaseLookupModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
    explicit DatabaseLookupModel(QObject *parent = nullptr);

    bool ordered;
    bool qmlMapSafe;

    typedef QList<QVariantMap> ItemsList;

    virtual QHash<int, QByteArray> roleNames() const;
    enum ItemRoles {
        ValuesRole = Qt::UserRole + 1,
    };

    int indexOf(const QString &name, const QVariant &value) const;

public:
    Q_INVOKABLE QVariantMap get(int i) const;
    Q_INVOKABLE bool set(int i, QVariantMap v);

public:
    int count() const;
    QString filter() const;
    void setFilter(QString v);

protected:
    ItemsList _items;
    QList<quint64> keys;

    //ListModel override
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

private:
    QString m_filter;

public slots:
    void syncItems(ItemsList items);
    void resetFilter();

signals:
    void itemEdited(int i, QVariantMap v);
    void itemRemoved(int i, QVariantMap v);
    void synced();

signals:
    void countChanged();
    void filterChanged();
};
