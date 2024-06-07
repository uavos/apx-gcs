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

#include <Database/DatabaseLookup.h>
#include <Fact/Fact.h>
#include <QtCore>

class TelemetryRecords : public DatabaseLookup
{
    Q_OBJECT
    Q_PROPERTY(quint64 recordsCount READ recordsCount NOTIFY recordsCountChanged)
    Q_PROPERTY(quint64 recordNum READ recordNum NOTIFY recordNumChanged)
    Q_PROPERTY(quint64 recordId READ recordId NOTIFY recordIdChanged)
    Q_PROPERTY(quint64 recordTimestamp READ recordTimestamp NOTIFY recordTimestampChanged)
    Q_PROPERTY(QVariantMap recordInfo READ recordInfo NOTIFY recordInfoChanged)

public:
    explicit TelemetryRecords(Fact *parent);

    Fact *f_latest;
    Fact *f_prev;
    Fact *f_next;
    Fact *f_remove;

    void jumpToRecord(quint64 v);

    //DatabaseLookup override
    void defaultLookup() override;

protected:
    bool fixItemDataThr(QVariantMap *item) override;

private:
    QString filterQuery() const;
    QVariantList filterValues() const;
    QString filterTrash() const;

    QMutex mutexRecordId;

    quint64 _findNumId{};

private slots:
    void updateActions();
    void updateStatus();
    void loadItem(QVariantMap modelData);

    //database
public slots:
    void dbLoadInfo();

private slots:
    void dbFindNum();

    void dbLoadLatest();
    void dbLoadPrev();
    void dbLoadNext();
    void dbRemove();

    void dbResultsLookup(DatabaseRequest::Records records);
    void dbResultsLatest(DatabaseRequest::Records records);
    void dbResultsPrevNext(DatabaseRequest::Records records);
    void dbResultsInfo(DatabaseRequest::Records records);
    void dbResultsNum(DatabaseRequest::Records records);
    void dbResultsNumNext(DatabaseRequest::Records records);

signals:
    void discardRequests(); //to stop loading on action
    void recordTriggered(quint64 telemetryID);

    //PROPERTIES
public:
    quint64 recordsCount() const;
    void setRecordsCount(quint64 v);
    quint64 recordNum() const;
    void setRecordNum(quint64 v);
    quint64 recordId();
    void setRecordId(quint64 v);
    quint64 recordTimestamp() const;
    void setRecordTimestamp(quint64 v);
    QVariantMap recordInfo() const;
    void setRecordInfo(const QVariantMap &v);

private:
    quint64 m_recordsCount{};
    quint64 m_recordNum{};
    quint64 m_recordId{};
    quint64 m_recordTimestamp{};
    QVariantMap m_recordInfo{};
signals:
    void recordsCountChanged();
    void recordNumChanged();
    void recordIdChanged();
    void recordTimestampChanged();
    void recordInfoChanged();
};
