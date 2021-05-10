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
#include "FactListModelActions.h"
#include "Fact.h"

FactListModelActions::FactListModelActions(Fact *parent)
    : QAbstractListModel(parent)
    , fact(parent)
{
    connect(parent, &Fact::actionsUpdated, this, [this]() {
        beginResetModel();
        endResetModel();
    });
}

int FactListModelActions::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return fact->actions().size();
}

QHash<int, QByteArray> FactListModelActions::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Fact::ModelDataRole] = "modelData";
    return roles;
}

QVariant FactListModelActions::data(const QModelIndex &index, int role) const
{
    Fact *item = fact->actions().value(index.row());
    if (!item)
        return QVariant();
    return item->data(index.column(), role);
}
