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
#ifndef FactTreeModel_H
#define FactTreeModel_H
//=============================================================================
#include <Fact/Fact.h>
#include <QtCore>
//=============================================================================
class FactTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit FactTreeModel(Fact *root);

    Fact *fact(const QModelIndex &index) const;
    QModelIndex factIndex(FactBase *item, int column = 0) const;

    void recursiveDisconnect(Fact *fact);
    void checkConnections(Fact *fact) const;

    QList<Fact *> expandedFacts;

    QHash<int, QByteArray> roleNames() const;

protected:
    Fact *root;

    //override
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

private:
    QTimer updateTimer;
    QList<QPointer<Fact>> updateList;
    QMultiHash<Fact *, int> updateHash;

    QList<Fact *> conFactLayout;

private slots:
    void itemToBeInserted(int row, FactBase *item);
    void itemInserted(FactBase *);
    void itemToBeRemoved(int row, FactBase *item);
    void itemRemoved(FactBase *);
    void itemToBeMoved(int row, int dest, FactBase *item);
    void itemMoved(FactBase *item);

    void textChanged();
    void titleChanged();
    void descrChanged();
    void enabledChanged();
    void activeChanged();
    void modifiedChanged();
    void progressChanged();
    void visibleChanged();

    void updateTimerTimeout();
    void itemDestroyed();
};
//=============================================================================
#endif
