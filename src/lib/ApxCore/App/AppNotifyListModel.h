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

#include "AppNotify.h"
#include <QtCore>

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

    void clear();

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

signals:
    void countChanged();

private slots:
    void notification(QString msg, QString subsystem, AppNotify::NotifyFlags flags, Fact *fact);
};
