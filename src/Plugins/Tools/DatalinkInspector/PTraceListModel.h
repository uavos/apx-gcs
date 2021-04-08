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

class PTraceListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit PTraceListModel(QObject *parent = nullptr);

    void append(QStringList item);
    void updateItem(int row, QStringList value);

    QList<QStringList> items() const { return m_items; }

    void clear();

    enum PTraceListModelRoles {
        DataRole = Qt::UserRole + 1,
    };

protected:
    QHash<int, QByteArray> roleNames() const override;

    //ListModel override
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QList<QStringList> m_items;
};

class PTraceFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit PTraceFilterProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {}

    void add_filter(QString s);
    void remove_filter(QString s);
    void clear_filter();

private:
    QStringList _filter;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};
