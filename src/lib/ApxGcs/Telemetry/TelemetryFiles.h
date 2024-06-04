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

#include "TelemetryFilesModel.h"
#include "TelemetryFilesWorker.h"

class TelemetryFiles : public Fact
{
    Q_OBJECT
    Q_PROPERTY(quint64 recordsCount READ recordsCount NOTIFY recordsCountChanged)
    Q_PROPERTY(quint64 recordNum READ recordNum NOTIFY recordNumChanged)
    Q_PROPERTY(quint64 recordTimestamp READ recordTimestamp NOTIFY recordTimestampChanged)
    Q_PROPERTY(QVariantMap recordInfo READ recordInfo NOTIFY recordInfoChanged)

public:
    explicit TelemetryFiles(Fact *parent);

    Fact *f_latest;
    Fact *f_prev;
    Fact *f_next;
    Fact *f_remove;

private:
    TelemetryFilesWorker *_worker;
    TelemetryFilesModel *_filesModel;

private slots:
    void updateActions();
    void updateStatus();

public slots:
    void triggerItem(QVariantMap modelData);

signals:
    void loadfile(QString filePath);

    //PROPERTIES
public:
    quint64 recordsCount() const;
    void setRecordsCount(quint64 v);

    quint64 recordNum() const;
    void setRecordNum(quint64 v);

    quint64 recordTimestamp() const;
    void setRecordTimestamp(quint64 v);

    QVariantMap recordInfo() const;
    void setRecordInfo(const QVariantMap &v);

private:
    quint64 m_recordsCount;
    quint64 m_recordNum;
    quint64 m_recordTimestamp;
    QVariantMap m_recordInfo;

signals:
    void recordsCountChanged();
    void recordNumChanged();
    void recordTimestampChanged();
    void recordInfoChanged();
};
