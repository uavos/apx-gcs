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
#include "TerminalListModel.h"
#include <Fact/Fact.h>
//=============================================================================
TerminalListModel::TerminalListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    _enterIndex = 0;
    qRegisterMetaType<QtMsgType>("QtMsgType");

    connect(ApxApp::instance(), &ApxApp::notification, this, &TerminalListModel::notification);
}
TerminalListModel::~TerminalListModel()
{
    qDeleteAll(_items);
}
//=============================================================================
//=============================================================================
int TerminalListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return _items.size();
}
//=============================================================================
QHash<int, QByteArray> TerminalListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TerminalListModel::TextRole] = "text";
    roles[TerminalListModel::SubsystemRole] = "subsystem";
    roles[TerminalListModel::SourceRole] = "source";
    roles[TerminalListModel::TypeRole] = "type";
    roles[TerminalListModel::OptionsRole] = "options";
    roles[TerminalListModel::FactRole] = "fact";
    roles[TerminalListModel::TimestampRole] = "timestamp";
    return roles;
}
//=============================================================================
QVariant TerminalListModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount())
        return QVariant();
    TerminalListItem *item = _items.at(index.row());
    switch (role) {
    case TextRole:
        return item->text;
    case SubsystemRole:
        return item->subsystem;
    case SourceRole:
        return static_cast<int>(item->flags) & ApxApp::NotifySourceMask;
    case TypeRole:
        return static_cast<int>(item->flags) & ApxApp::NotifyTypeMask;
    case OptionsRole:
        return static_cast<int>(item->flags) & ApxApp::NotifyOptionsMask;
    case FactRole:
        return QVariant::fromValue(item->fact);
    case TimestampRole:
        return item->timestamp;
    }
    return QVariant();
}
//=============================================================================
//=============================================================================
void TerminalListModel::notification(QString msg,
                                     QString subsystem,
                                     ApxApp::NotifyFlags flags,
                                     Fact *fact)
{
    if (msg.isEmpty())
        return;
    int row = rowCount();
    if (row > 1000) {
        beginRemoveRows(QModelIndex(), 0, 1);
        _items.removeAt(0);
        _items.removeAt(0);
        endRemoveRows();
    }
    row = rowCount();
    beginInsertRows(QModelIndex(), row, row);
    TerminalListItem *item = new TerminalListItem;
    item->timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    item->text = msg;
    item->subsystem = subsystem;
    item->flags = flags;
    item->fact = fact;
    _items.append(item);
    endInsertRows();
    emit countChanged();
    //qDebug()<<"rows"<<rowCount();
}
/*void TerminalListModel::append(QtMsgType type, QString category, QString text)
{
    int row = rowCount();
    if (row > 1000) {
        beginRemoveRows(QModelIndex(), 0, 1);
        _items.removeAt(0);
        _items.removeAt(0);
        endRemoveRows();
    }
    row = rowCount();
    beginInsertRows(QModelIndex(), row, row);
    TerminalListItem *item = new TerminalListItem;
    item->type = type;
    item->category = category;
    item->text = text;
    item->timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    _items.append(item);
    endInsertRows();
    emit countChanged();
    //qDebug()<<"rows"<<rowCount();
}*/
//=============================================================================
void TerminalListModel::enter(const QString &line)
{
    //append(QtInfoMsg, "input", line);
    notification(line, "", ApxApp::FromInput, nullptr);
    _enterIndex = _items.size() - 1;
}
void TerminalListModel::enterResult(bool ok)
{
    if (_enterIndex >= _items.size())
        return;
    _items[_enterIndex]->text.insert(0, QString("[%1]").arg(ok ? tr("ok") : tr("error")));
    QModelIndex i = index(_enterIndex, 0);
    emit dataChanged(i, i, QVector<int>() << TextRole);
}
//=============================================================================
