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
#include "MissionMapItemsModel.h"
#include <Fact/Fact.h>
//=============================================================================
MissionMapItemsModel::MissionMapItemsModel(Fact *fact)
    : FactListModel(fact)
{}
//=============================================================================
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
//=============================================================================
