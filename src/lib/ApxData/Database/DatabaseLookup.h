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
#ifndef DatabaseLookup_H
#define DatabaseLookup_H
//=============================================================================
#include "DatabaseLookupModel.h"
#include "DatabaseRequest.h"
#include <Fact/Fact.h>
#include <QtCore>
//=============================================================================
class DatabaseLookup : public Fact
{
    Q_OBJECT
    Q_PROPERTY(DatabaseLookupModel *dbModel READ dbModel CONSTANT)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(
        bool filterEnabled READ filterEnabled WRITE setFilterEnabled NOTIFY filterEnabledChanged)

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

protected:
    DatabaseSession *db;
    DatabaseRequest::Records records;
    DatabaseLookupModel::ItemsList recordsItems;

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

    //---------------------------------------
    // PROPERTIES
public:
    DatabaseLookupModel *dbModel() const;
    QString filter() const;
    void setFilter(const QString &v);
    bool filterEnabled() const;
    void setFilterEnabled(const bool &v);

private:
    DatabaseLookupModel *m_dbModel;
    QString m_filter;
    bool m_filterEnabled;
signals:
    void filterChanged();
    void filterEnabledChanged();
};
//=============================================================================
#endif
