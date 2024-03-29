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

#include "DatabaseLookupModel.h"
#include "DatabaseRequest.h"
#include <Fact/Fact.h>
#include <QtCore>

class DatabaseLookup : public Fact
{
    Q_OBJECT

public:
    explicit DatabaseLookup(Fact *parent,
                            const QString &name,
                            const QString &title,
                            const QString &descr,
                            DatabaseSession *db,
                            FactBase::Flags flags = FactBase::Flags(Group));

    Q_INVOKABLE void query(const QString &queryString,
                           const QVariantList &bindValues = QVariantList());
    Q_INVOKABLE void query(DatabaseRequest *req);

    DatabaseLookupModel *dbModel() const;

protected:
    DatabaseSession *db;
    DatabaseRequest::Records records;
    DatabaseLookupModel::ItemsList recordsItems;

    QString filter() const;

    virtual bool fixItemDataThr(QVariantMap *item);

    QTimer modelSyncTimer;

    QMutex loadMutex;

protected slots:
    void reloadQueryResults();
    void loadRecordsItems(DatabaseLookupModel::ItemsList list);
    virtual void loadItems();

public slots:
    virtual void defaultLookup();
    void loadQueryResults(DatabaseRequest::Records records);
    void triggerItem(QVariantMap modelData);

signals:
    void itemTriggered(QVariantMap modelData);

    //internal loading
    void _itemsLoaded(DatabaseLookupModel::ItemsList list);
};
