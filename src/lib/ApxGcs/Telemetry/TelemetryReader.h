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

#include <ApxMisc/DelayedEvent.h>
#include <Fact/Fact.h>
#include <QGeoPath>
#include <QtCore>

#include <Database/StorageReq.h>

class TelemetryRecords;

class TelemetryReader : public Fact
{
    Q_OBJECT
    Q_PROPERTY(quint64 totalSize READ totalSize NOTIFY totalSizeChanged)
    Q_PROPERTY(quint64 totalTime READ totalTime NOTIFY totalTimeChanged)

public:
    explicit TelemetryReader(Fact *parent);

    Fact *f_notes;
    Fact *f_reload;

    // datatypes for data signals
    using Field = TelemetryFileReader::Field;
    using Values = TelemetryFileReader::Values;

    const auto &recordInfo() const { return _recordInfo; }
    const auto &fields() const { return _fields; }
    const auto &recordFilePath() const { return _recordFilePath; }

private:
    quint64 _loadRecordID{};
    QList<Field> _fields;
    QJsonObject _recordInfo;
    QString _recordFilePath;

    QGeoPath _geoPath;
    quint64 _totalDistance;
    int _fidx_lat;
    int _fidx_lon;
    int _fidx_hmsl;
    QGeoCoordinate _geoPos;

    QSet<quint64> _importedMeta; // keep track of imported nodes and missions

    bool blockNotesChange;

    void addEventFact(quint64 time,
                      const QString &name,
                      const QString &value,
                      const QString &uid,
                      const QVariant &data = {});

private slots:
    void notesChanged();
    void updateStatus();

    void setRecordInfo(quint64 id, QJsonObject info, QString notes);

    void do_rec_field(QString name, QString title, QString units);
    void do_rec_values(quint64 timestamp_ms, Values data, bool uplink);
    void do_rec_evt(quint64 timestamp_ms, QString name, QString value, QString uid, bool uplink);
    void do_rec_meta(quint64 timestamp_ms, QString name, QJsonObject data, bool uplink);

signals:
    // forwarded signals from file reader
    void rec_started();
    void rec_finished();
    void rec_field(QString name, QString title, QString units);
    void rec_values(quint64 timestamp_ms, Values data, bool uplink);
    void rec_evt(quint64 timestamp_ms, QString name, QString value, QString uid, bool uplink);
    void rec_msg(quint64 timestamp_ms, QString text, QString subsystem);
    void rec_meta(quint64 timestamp_ms, QString name, QJsonObject data, bool uplink);
    void rec_raw(quint64 timestamp_ms, uint16_t id, QByteArray data, bool uplink);

    // called when file parsed and header info collected
    void recordInfoUpdated(quint64 id, QJsonObject info);
    void geoPathCollected(QGeoPath path, quint64 totalDistance);

    // stats text changed
    void recordInfoChanged();

    // user triggers event fact (child with stats)
    void statsFactTriggered(Fact *f);

public slots:
    void loadRecord(quint64 id);

    //PROPERTIES
public:
    quint64 totalSize() const;
    void setTotalSize(quint64 v);
    quint64 totalTime() const;
    void setTotalTime(quint64 v);

private:
    quint64 m_totalSize;
    quint64 m_totalTime;

signals:
    void totalSizeChanged();
    void totalTimeChanged();
};
