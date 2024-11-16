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

class XbusStreamWriter;

class TelemetryFileWriter : private QDataStream
{
public:
    struct Field
    {
        QString name;
        QString title;
        QString units;
        telemetry::dspec_e dspec;
    };
    using Fields = QHash<mandala::uid_t, Field>;
    using Values = PBase::Values;

    explicit TelemetryFileWriter();
    explicit TelemetryFileWriter(const Fields &fields);
    virtual ~TelemetryFileWriter();

    void setFields(const Fields &fields) { _fields = fields; }
    const auto &fields() const { return _fields; }

    bool init(QIODevice *d, const QString &name, quint64 time_utc, QJsonObject info);
    void close();

    const QString &name() const { return _name; }

    void write_timestamp(quint32 timestamp_ms);
    void write_values(quint32 timestamp_ms, const Values &values, bool uplink = false);
    void write_value(mandala::uid_t uid, QVariant value, bool uplink = false);

    void write_evt(quint32 timestamp_ms,
                   const QString &name,
                   const QString &value,
                   const QString &uid,
                   bool uplink = false);
    void write_msg(quint32 timestamp_ms, const QString &text, const QString &subsystem);
    void write_meta(const QString &name, const QJsonObject &data, bool uplink = false);
    void write_raw(quint32 timestamp_ms, uint16_t id, const QByteArray &data, bool uplink = false);

    void print_stats();

    // helpers
    static void json_diff(const QJsonObject &prev, const QJsonObject &next, QJsonObject &diff);

    static QLockFile *get_lock_file(QString fileName);

private:
    QString _name;
    Fields _fields;

    QLockFile *_lock_file{};

    bool isOpen() const { return device() && device()->isOpen() && device()->isWritable(); }
    bool write(const void *src, int len) { return writeRawData((const char *) src, len) == len; }
    void flush();

    // helpers
    void _write_string(const char *s);
    void _write_uplink();

    void _write_field(QString name, QString title, QString units);

    // monitor changes and updates
    std::map<mandala::uid_t, uint16_t> _fields_map;
    std::map<mandala::uid_t, double> _values_s;
    std::map<QString, QJsonObject> _meta_objects;

    std::map<mandala::uid_t, QSet<telemetry::dspec_e>> _stats_values;

    quint32 _ts_s{};
    uint16_t _widx{};
};
