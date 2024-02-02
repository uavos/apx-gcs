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

class Fact;
class FactBase;

class FactListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
    explicit FactListModel(Fact *fact);

    typedef QList<QPointer<Fact>> ItemsList;

    QHash<int, QByteArray> roleNames() const;

public:
    Q_INVOKABLE Fact *get(int i) const;

public:
    int count() const;
    QString filter() const;
    void setFilter(QString v);

protected:
    Fact *fact;
    ItemsList _items;

    virtual void populate(ItemsList *list, Fact *f);
    void connectFact(Fact *fact);

    virtual void syncModel(const ItemsList &list);

    //ListModel override
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

private:
    QString m_filter;

private:
    QTimer *syncTimer;

public slots:
    void sync();
    void scheduleSync();
    void resetFilter();

signals:
    void countChanged();
    void filterChanged();
    void layoutChanged();
};
