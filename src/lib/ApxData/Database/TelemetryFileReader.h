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

#include <QtCore>

#include "TelemetryFileFormat.h"
#include "TelemetryFileWriter.h"

class XbusStreamReader;

class TelemetryFileReader : public QObject, private QDataStream
{
    Q_OBJECT

public:
    // datatypes used in data signals
    using Field = TelemetryFileWriter::FieldBase;
    using Values = std::map<size_t, QVariant>;
    using Event = TelemetryFileWriter::Event;

    explicit TelemetryFileReader(QIODevice *d = nullptr, const QString &name = {});

    bool init(QIODevice *d, const QString &name);

    bool is_still_writing();

    const auto &name() const { return _name; }
    const auto &info() const { return _info; }

    bool parse_header();
    bool parse_payload();

    auto timestamp() const { return _fhdr.timestamp; }
    auto utc_offset() const { return _fhdr.utc_offset; }
    auto is_parsed() const { return _info["parsed"].toBool(); }

    static QString get_hash(QIODevice *d);
    static QString get_hash(QString filePath);

    void abort() { _interrupted = true; } // abort reading (can be called from another thread)
    bool interrupted() const { return _interrupted; }

    // replay support
    bool parse_next();
    quint32 current_time() const { return _ts_s; }

    using QDataStream::atEnd;

private:
    QString _name;
    QJsonObject _info;

    telemetry::fhdr_s _fhdr;

    std::atomic<bool> _interrupted{};

    bool isOpen() const { return device() && device()->isOpen(); }
    bool read(void *data, size_t len) { return readRawData((char *) data, len) == len; }
    bool seek(qint64 pos) { return device()->seek(pos); }
    qint64 pos() const { return device()->pos(); }
    qint64 size() const { return device()->size(); }

    // helpers
    telemetry::dspec_s _read_dspec();
    QString _read_string(bool *ok);
    QJsonObject _read_jso_content();
    QJsonObject _read_info();
    QStringList _read_reg();

    QVariant _read_value(telemetry::dspec_e dspec);
    bool _read_ext(telemetry::extid_e extid, bool is_uplink);
    std::pair<QString, QJsonObject> _read_jso();

    template<typename T>
    T _read_raw(bool *ok, size_t sz = sizeof(T))
    {
        T v{};
        if (!read(&v, sz)) {
            qWarning() << "failed to read value";
            *ok = false;
            return {};
        }
        *ok = true;
        return v;
    };

    // monitor changes and updates
    quint32 _ts_s;
    uint16_t _widx;
    bool _next_uplink;
    QHash<QString, QJsonObject> _jso_s;

    // data counters to be saved in metadata
    struct
    {
        qint64 records; // total number of records in file

        qint64 uplink;   // uplink records
        qint64 downlink; // downlink records

        // each record type counter
        qint64 ts;
        qint64 dir;
        qint64 field;
        qint64 evtid;

        std::map<QString, qint64> jso_by_name;
        std::map<QString, qint64> evt_by_name;
        std::map<QString, qint64> raw_by_name;

    } _counters;

    // fields data
    std::vector<Field> _field_index; // used internally
    std::vector<Event> _evt_index;   // used internally

    // values data
    Values _downlink_values;
    Values _uplink_values;
    void _commit_values();

    void _reset_data();

    void setProgress(int value);
    int _progress{-1};

signals:
    void progressChanged(int value);

    // called by parse_payload
    void field(Field field);
    void values(quint64 timestamp_ms, Values data, bool uplink);
    void evt(quint64 timestamp_ms, QString name, QJsonObject data, bool uplink);
    void jso(quint64 timestamp_ms, QString name, QJsonObject data, bool uplink);
    void raw(quint64 timestamp_ms, QString name, QByteArray data, bool uplink);

    // called by parse_header
    void infoUpdated(QJsonObject data);
};
