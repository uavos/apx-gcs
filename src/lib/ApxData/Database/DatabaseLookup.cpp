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

    setModel(new DatabaseLookupModel(this));

    connect(this,
            &DatabaseLookup::_itemsLoaded,
            this,
            &DatabaseLookup::loadRecordsItems,
            Qt::QueuedConnection);

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

    modelSyncTimer.setSingleShot(true);
    modelSyncTimer.setInterval(500);
    connect(&modelSyncTimer, &QTimer::timeout, this, &DatabaseLookup::loadItems);
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
            &DatabaseLookup::loadQueryResults,
            Qt::DirectConnection);
    req->exec();
}
void DatabaseLookup::loadQueryResults(DatabaseRequest::Records records)
{
    loadMutex.lock();
    this->records = records;
    loadMutex.unlock();
    reloadQueryResults();
}
void DatabaseLookup::reloadQueryResults()
{
    //run in thread
    loadMutex.lock();
    const QStringList &n = records.names;
    DatabaseLookupModel::ItemsList list;
    for (int i = 0; i < records.values.size(); ++i) {
        const QVariantList &r = records.values.at(i);
        //create item data
        QVariantMap m;
        for (int j = 0; j < n.size(); ++j) {
            const QString &fn = n.at(j);
            if (m.contains(fn))
                continue;
            m.insert(fn, r.value(j));
        }
        if (!fixItemDataThr(&m))
            break;
        list.append(m);
    }
    loadMutex.unlock();
    emit _itemsLoaded(list);
}
bool DatabaseLookup::fixItemDataThr(QVariantMap *item)
{
    Q_UNUSED(item)
    return true;
}
void DatabaseLookup::loadRecordsItems(DatabaseLookupModel::ItemsList list)
{
    recordsItems = list;
    modelSyncTimer.start();
}
void DatabaseLookup::loadItems()
{
    static_cast<DatabaseLookupModel *>(model())->syncItems(recordsItems);
}

void DatabaseLookup::triggerItem(QVariantMap modelData)
{
    emit itemTriggered(modelData);
}
