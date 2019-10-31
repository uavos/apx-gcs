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
#ifndef AppNotifyListModel_H
#define AppNotifyListModel_H
//=============================================================================
#include "AppNotify.h"
#include <QtCore>
//=============================================================================
class AppNotifyListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    explicit AppNotifyListModel(AppNotify *appNotify);
    ~AppNotifyListModel() override;

    enum AppNotifyListModelRoles {
        ModelDataRoleDis = Qt::UserRole + 1,
        TextRole,
        SubsystemRole,

        SourceRole,
        TypeRole,
        OptionsRole,

        FactRole,

        TimestampRole,
    };
    QHash<int, QByteArray> roleNames() const override;

    void updateItem(int row, const QVariant &value, int role = TextRole);

    //ListModel override
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    struct NotifyListItem
    {
        qint64 timestamp;
        QString text;
        QString subsystem;
        AppNotify::NotifyFlags flags;
        QPointer<Fact> fact;
    };
    QList<NotifyListItem *> m_items;

private slots:
    void notification(QString msg, QString subsystem, AppNotify::NotifyFlags flags, Fact *fact);

    //-----------------------------------------
    //PROPERTIES
public:
protected:
signals:
    void countChanged();
};
//=============================================================================
#endif
