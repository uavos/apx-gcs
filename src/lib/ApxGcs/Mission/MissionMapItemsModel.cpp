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
#include "MissionMapItemsModel.h"
#include <Fact/Fact.h>

MissionMapItemsModel::MissionMapItemsModel(Fact *fact)
    : FactListModel(fact)
{}

void MissionMapItemsModel::syncModel(const ItemsList &list)
{
    //find deleted
    for (int i = 0; i < _items.size(); ++i) {
        if (list.contains(_items.at(i)))
            continue;
        beginRemoveRows(QModelIndex(), i, i);
        _items.removeAt(i);
        endRemoveRows();
        //qDebug()<<"del"<<i<<this;
        emit countChanged();
        i--;
    }
    //find inserted
    for (int i = 0; i < list.size(); ++i) {
        Fact *src = list.at(i);
        if (_items.contains(src))
            continue;
        beginInsertRows(QModelIndex(), _items.size(), _items.size());
        _items.append(src);
        endInsertRows();
        //qDebug()<<"ins"<<i<<this;
        emit countChanged();
    }
}
