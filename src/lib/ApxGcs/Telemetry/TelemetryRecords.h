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

#include <Fact/Fact.h>

class DatabaseModel;

class TelemetryRecords : public Fact
{
    Q_OBJECT
    Q_PROPERTY(quint64 recordsCount READ recordsCount NOTIFY recordsCountChanged)
    Q_PROPERTY(quint64 recordNum READ recordNum NOTIFY recordNumChanged)

public:
    explicit TelemetryRecords(Fact *parent);

    Fact *f_restore;
    Fact *f_latest;
    Fact *f_prev;
    Fact *f_next;
    Fact *f_remove;

private:
    DatabaseModel *_dbmodel;

private slots:
    void updateActions();
    void updateStatus();

    //database
public slots:
    void setRecordsCount(quint64 v);
    void setRecordNum(quint64 v);
    void recordInfoUpdated(quint64 id) { emit dbRequestRecordInfo(id); }

private slots:
    void dbRequestRecordsList();
    void dbRequestRecordInfo(quint64 id);

    void dbLoadLatest();

    void dbLoadPrev();
    void dbLoadNext();
    void dbRemove();

signals:
    void discardRequests(); //to stop loading on action
    void recordTriggered(quint64 id);

    //PROPERTIES
public:
    quint64 recordsCount() const;
    quint64 recordNum() const;

private:
    quint64 m_recordsCount{};
    quint64 m_recordNum{};
signals:
    void recordsCountChanged();
    void recordNumChanged();
};
