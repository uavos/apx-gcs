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
#ifndef FactListModel_H
#define FactListModel_H
//=============================================================================
#include <QtCore>
class Fact;
class FactBase;
//=============================================================================
class FactListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit FactListModel(Fact *fact);

    typedef QList<QPointer<Fact>> ItemsList;

    QHash<int, QByteArray> roleNames() const;

public slots:
    void sync();
    void scheduleSync();

private:
    QTimer *syncTimer;

protected:
    Fact *fact;
    ItemsList _items;

    virtual void populate(ItemsList *list, Fact *fact);
    void connectFact(Fact *fact);

    virtual void syncModel(const ItemsList &list);

    //ListModel override
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

public:
    Q_INVOKABLE Fact *get(int i) const;

    //-----------------------------------------
    //PROPERTIES
public:
    int count() const;

signals:
    void countChanged();
    void layoutChanged();
};
//=============================================================================
#endif
