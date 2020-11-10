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

#include "FactDelegate.h"
#include "FactDelegateDialog.h"
#include <QAbstractItemModel>

class FactDelegateArrayModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    FactDelegateArrayModel(Fact *group, QObject *parent = nullptr);
    QPointer<Fact> group;
    void emitReset() { emit layoutChanged(); }

private:
    Fact *field(const QModelIndex &index) const;

    //override
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
};

class FactDelegateArray : public FactDelegateDialog
{
    Q_OBJECT
public:
    explicit FactDelegateArray(Fact *fact, QWidget *parent = 0);

private:
    FactDelegateArrayModel *model;
    QTreeView *treeView;
};
