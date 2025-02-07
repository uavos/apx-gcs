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

class XbusStreamWriter;

class TelemetryFileWriter : private QDataStream
{
public:
    struct FieldBase
    {
        QString name;
        QStringList info;
    };
    struct Field : FieldBase
    {
        telemetry::dspec_e dspec;
    };
    using Values = std::map<const Field *, QVariant>; // map is sorted by field ptr key

    struct Event : FieldBase
    {};

    explicit TelemetryFileWriter();
    virtual ~TelemetryFileWriter();

    bool init(QIODevice *d, const QString &name, quint64 time_utc, QJsonObject info);
    void close();

    const auto &name() const { return _name; }

    // methods to write content
    void write_timestamp(quint32 timestamp_ms);
    void write_values(quint32 timestamp_ms, const Values &values, bool dir = false);
    void write_value(const Field *field, const QVariant &value, bool dir = false);

    void write_evt(quint32 timestamp_ms,
                   const Event *evt,
                   const QStringList &values,
                   bool dir,
                   uint skip_cache_cnt = 0);
    void write_jso(quint32 timestamp_ms, const QString &name, const QJsonObject &data, bool dir);
    void write_raw(quint32 timestamp_ms, const QString &name, const QByteArray &data, bool dir);

    // helpers
    static QLockFile *get_lock_file(QString fileName);
    static telemetry::dspec_e dspec_for_mpath(const QString &mpath);

private:
    QString _name;
    QLockFile *_lock_file{};

    bool isOpen() const { return device() && device()->isOpen() && device()->isWritable(); }
    bool write(const void *src, int len) { return writeRawData((const char *) src, len) == len; }
    void flush();

    // helpers
    void _write_string(const char *s);
    void _write_string_cached(const char *s, bool skip_cache = false);
    void _write_dir();

    void _write_reg(telemetry::extid_e extid, QString name, QStringList stings);

    // monitor stream changes and updates
    std::map<const Field *, size_t> _field_index; // field indexes in file
    std::map<const Event *, size_t> _evt_index;   // evt index in file
    std::map<const Field *, double> _values_s;    // monitor value changes
    std::vector<std::string> _str_cache;          // strings cache

    std::map<QString, QJsonObject> _jso_s;

    std::map<size_t, QSet<telemetry::dspec_e>> _stats_values;

    quint32 _ts_s{};
    uint16_t _widx{};
};
