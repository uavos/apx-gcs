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
#include "FactListModelActions.h"
#include "Fact.h"
//=============================================================================
FactListModelActions::FactListModelActions(Fact *parent)
    : QAbstractListModel(parent)
    , fact(parent)
{
    connect(parent, &Fact::actionsUpdated, this, [this]() {
        beginResetModel();
        endResetModel();
    });
}
//=============================================================================
//=============================================================================
int FactListModelActions::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return fact->actions().size();
}
//=============================================================================
QHash<int, QByteArray> FactListModelActions::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Fact::ModelDataRole] = "modelData";
    return roles;
}
//=============================================================================
QVariant FactListModelActions::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount())
        return QVariant();
    Fact *item = qobject_cast<Fact *>(fact->actions().at(index.row()));
    return item->data(index.column(), role);
}
//=============================================================================
