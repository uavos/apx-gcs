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

#include <Mandala/Mandala.h>
#include <Mandala/MandalaContainers.h>

#include "TelemetryFileFormat.h"

class XbusStreamReader;

class TelemetryFileReader : public QFile
{
    Q_OBJECT

public:
    explicit TelemetryFileReader(QObject *parent = nullptr);
    explicit TelemetryFileReader(QString filePath, QObject *parent = nullptr);

    bool open(QString filePath);

    auto info() const { return _info; }

    bool parse_payload();
    bool parse_header();

    auto callsign() const { return _info["call"].toString(); }
    auto timestamp() const { return _info["timestamp"].toInteger(); }
    auto utc_offset() const { return _info["utc_offset"].toInteger(); }
    auto is_parsed() const { return _info["parsed"].toBool(); }

    QByteArray get_hash();

private:
    QJsonObject _info;

    telemetry::fhdr_s _fhdr;

    // helpers
    telemetry::dspec_s _read_dspec();
    QString _read_string(bool *ok);
    QJsonObject _read_meta_data();

    QJsonObject _read_stats();

    QVariant _read_value(telemetry::dspec_e dspec);
    bool _read_ext(telemetry::extid_e extid, bool is_uplink);
    std::pair<QString, QJsonObject> _read_meta();

    template<typename T>
    T _read_raw(bool *ok, size_t sz = sizeof(T))
    {
        T v{};
        if (QFile::read((char *) &v, sz) != sz) {
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
    QHash<QString, QJsonObject> _meta_objects;

    // data counters to be saved in metadata
    struct
    {
        qint64 records;
        qint64 fields;

        qint64 uplink;
        qint64 downlink;

        qint64 evt;
        qint64 msg;
        qint64 meta;
        qint64 raw;

        qint64 mission;
        qint64 nodes;
        qint64 conf;
    } _counters;

    // fields data
    struct Field
    {
        QString name;
        QString title;
        QString units;
    };
    QList<Field> _fields; // used internally

    // values data
    using Values = QHash<size_t, QVariant>;
    Values _downlink_values;
    Values _uplink_values;
    void _commit_values();

    void _reset_data();
    void _json_patch(const QJsonObject &orig, const QJsonObject &patch, QJsonObject &result);

    // write updates to the file
    bool _update_header();
    bool _update_stats();

    void setProgress(int value);
    int _progress{-1};

signals:
    void progressChanged(int value);

    // called by parse_payload
    void field(QString name, QString title, QString units);
    void values(quint64 timestamp_ms, Values data, bool uplink);
    void evt(quint64 timestamp_ms, QString name, QString value, QString uid, bool uplink);
    void msg(quint64 timestamp_ms, QString text, QString subsystem);
    void meta(QString name, QJsonObject data, bool uplink);
    void raw(quint64 timestamp_ms, uint16_t id, QByteArray data, bool uplink);

    void parsed();

    // called by parse_header
    void infoUpdated(QJsonObject data);
};
