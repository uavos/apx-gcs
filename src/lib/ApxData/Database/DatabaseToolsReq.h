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

#include "DatabaseRequest.h"
#include "DatabaseSession.h"

namespace db {

class MakeTable : public DatabaseRequest
{
public:
    explicit MakeTable(DatabaseSession *db,
                       const QString &tableName,
                       const QStringList &fields,
                       const QString &tail = QString())
        : DatabaseRequest(db)
        , tableName(tableName)
        , fields(fields)
        , tail(tail)
    {
        exec();
    }

protected:
    bool run(QSqlQuery &query);

private:
    QString tableName;
    QStringList fields;
    QString tail;
};
class MakeIndex : public DatabaseRequest
{
public:
    explicit MakeIndex(DatabaseSession *db,
                       const QString &tableName,
                       const QString &indexName,
                       bool unique)
        : DatabaseRequest(db)
        , tableName(tableName)
        , indexName(indexName)
        , unique(unique)
    {
        exec();
    }

protected:
    bool run(QSqlQuery &query);

private:
    QString tableName;
    QString indexName;
    bool unique;
};
class Vacuum : public DatabaseRequest
{
public:
    explicit Vacuum(DatabaseSession *db)
        : DatabaseRequest(db)
        , name(QFileInfo(db->fileName).baseName())
    {}

protected:
    bool run(QSqlQuery &query);

private:
    QString name;
};
class Analyze : public DatabaseRequest
{
public:
    explicit Analyze(DatabaseSession *db)
        : DatabaseRequest(db)
        , name(QFileInfo(db->fileName).baseName())
    {}

protected:
    bool run(QSqlQuery &query);

private:
    QString name;
};

} // namespace db
