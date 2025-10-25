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
#include "AppNotifyListModel.h"
#include <Fact/Fact.h>

AppNotifyListModel::AppNotifyListModel(AppNotify *appNotify)
    : QAbstractListModel(appNotify)
{
    qRegisterMetaType<QtMsgType>("QtMsgType");

    connect(appNotify, &AppNotify::notification, this, &AppNotifyListModel::notification);
}
AppNotifyListModel::~AppNotifyListModel()
{
    qDeleteAll(m_items);
}

int AppNotifyListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_items.size();
}

QHash<int, QByteArray> AppNotifyListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[AppNotifyListModel::TextRole] = "text";
    roles[AppNotifyListModel::SubsystemRole] = "subsystem";
    roles[AppNotifyListModel::SourceRole] = "source";
    roles[AppNotifyListModel::TypeRole] = "type";
    roles[AppNotifyListModel::OptionsRole] = "options";
    roles[AppNotifyListModel::FactRole] = "fact";
    roles[AppNotifyListModel::TimestampRole] = "timestamp";
    roles[AppNotifyListModel::SelectedRole] = "selected";
    return roles;
}

QVariant AppNotifyListModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount())
        return QVariant();
    NotifyListItem *item = m_items.at(index.row());
    switch (role) {
    case TextRole:
        return item->text;
    case SubsystemRole:
        return item->subsystem;
    case SourceRole:
        return static_cast<int>(item->flags) & AppNotify::NotifySourceMask;
    case TypeRole:
        return static_cast<int>(item->flags) & AppNotify::NotifyTypeMask;
    case OptionsRole:
        return static_cast<int>(item->flags) & AppNotify::NotifyOptionsMask;
    case FactRole:
        return QVariant::fromValue(item->fact);
    case TimestampRole:
        return item->timestamp;
    case SelectedRole:
        return item->selected;
    }
    return QVariant();
}

void AppNotifyListModel::notification(QString msg,
                                      QString subsystem,
                                      AppNotify::NotifyFlags flags,
                                      Fact *fact)
{
    if (msg.isEmpty())
        return;
    int row = rowCount();
    if (row > 1000) {
        beginRemoveRows(QModelIndex(), 0, 1);
        m_items.removeAt(0);
        m_items.removeAt(0);
        endRemoveRows();
    }
    row = rowCount();
    beginInsertRows(QModelIndex(), row, row);
    NotifyListItem *item = new NotifyListItem;
    item->timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    item->text = msg;
    item->subsystem = subsystem;
    item->flags = flags;
    item->fact = fact;
    m_items.append(item);
    endInsertRows();
    emit countChanged();
}

void AppNotifyListModel::updateItem(int row, const QVariant &value, int role)
{
    if (row < 0 || row >= rowCount())
        return;
    NotifyListItem *item = m_items.at(row);
    switch (role) {
    default:
        return;
    case TextRole:
        item->text = value.toString();
        break;
    case SubsystemRole:
        item->subsystem = value.toString();
        break;
    case SourceRole:
        item->flags = (item->flags & ~AppNotify::NotifySourceMask)
                      | static_cast<AppNotify::NotifyFlag>(value.toInt()
                                                           & AppNotify::NotifySourceMask);
        break;
    case TypeRole:
        item->flags = (item->flags & ~AppNotify::NotifyTypeMask)
                      | static_cast<AppNotify::NotifyFlag>(value.toInt()
                                                           & AppNotify::NotifyTypeMask);
        break;
    case OptionsRole:
        item->flags = (item->flags & ~AppNotify::NotifyOptionsMask)
                      | static_cast<AppNotify::NotifyFlag>(value.toInt()
                                                           & AppNotify::NotifyOptionsMask);
        break;
    case FactRole:
        item->fact = value.value<Fact *>();
        break;
    case TimestampRole:
        item->timestamp = value.toLongLong();
        break;
    case SelectedRole:
        item->selected = value.toBool();
        break;
    }
    QModelIndex i = index(row, 0);
    emit dataChanged(i, i, QVector<int>() << role);
}

void AppNotifyListModel::clear()
{
    beginResetModel();
    qDeleteAll(m_items);
    m_items.clear();
    endResetModel();
    emit countChanged();
}
