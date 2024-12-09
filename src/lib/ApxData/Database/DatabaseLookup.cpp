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
#include "DatabaseLookup.h"
#include "DatabaseSession.h"

DatabaseLookup::DatabaseLookup(Fact *parent,
                               const QString &name,
                               const QString &title,
                               const QString &descr,
                               DatabaseSession *db,
                               Flags flags)
    : Fact(parent, name, title, descr, flags | FilterModel, "database-search")
    , db(db)
{
    setOpt("page", "Menu/FactMenuPageLookupDB.qml");
    setOpt("dbtool", true); // will use old signals on qml

    setModel(new DatabaseLookupModel(this));

    connect(dbModel(), &DatabaseLookupModel::filterChanged, this, &DatabaseLookup::defaultLookup);
    connect(db,
            &DatabaseSession::modified,
            this,
            &DatabaseLookup::defaultLookup,
            Qt::QueuedConnection);

    connect(this, &Fact::triggered, this, &DatabaseLookup::defaultLookup);

    Fact *f_tools = new Fact(this,
                             "tools",
                             tr("Tools"),
                             tr("Database maintenance"),
                             Action | IconOnly,
                             "wrench");
    f_tools->setBinding(db);

    // delayed update loaded records
    connect(
        this,
        &DatabaseLookup::_itemsLoaded,
        this,
        [this](DatabaseLookupModel::ItemsList items) {
            _loadedItems = items;
            _modelSyncTimer.start();
        },
        Qt::QueuedConnection);

    _modelSyncTimer.setSingleShot(true);
    _modelSyncTimer.setInterval(100);
    connect(&_modelSyncTimer, &QTimer::timeout, this, [this]() {
        static_cast<DatabaseLookupModel *>(model())->syncItems(_loadedItems);
    });
}

QString DatabaseLookup::filter() const
{
    return static_cast<DatabaseLookupModel *>(m_model)->filter();
}
DatabaseLookupModel *DatabaseLookup::dbModel() const
{
    return static_cast<DatabaseLookupModel *>(m_model);
}

void DatabaseLookup::query(const QString &queryString, const QVariantList &bindValues)
{
    //qDebug()<<"updateQuery"<<queryString<<bindValues;
    DatabaseRequest *req = new DatabaseRequest(db, queryString, bindValues);
    query(req);
}
void DatabaseLookup::query(DatabaseRequest *req)
{
    connect(req,
            &DatabaseRequest::queryResults,
            this,
            &DatabaseLookup::thr_loadQueryResults,
            Qt::DirectConnection);
    req->exec();
}
void DatabaseLookup::thr_loadQueryResults(QJsonArray records)
{
    DatabaseLookupModel::ItemsList items;

    // fix items data in thread
    for (auto i : records) {
        auto item = thr_prepareRecordData(i.toObject());
        if (item.isEmpty())
            continue;
        items.append(item);
    }
    json::save("DatabaseLookup", records);
    qDebug() << "DatabaseLookup: " << title() << records.size() << items.size();

    emit _itemsLoaded(items); // call from main thread
}
