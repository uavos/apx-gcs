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
#include "TelemetryFileReader.h"
#include "TelemetryFileFormat.h"
#include "TelemetryFileWriter.h"

#include <App/App.h>
#include <App/AppDirs.h>

#include <TelemetryValuePack.h>
#include <TelemetryValueUnpack.h>

#include <ApxMisc/JsonHelpers.h>

using namespace telemetry;

TelemetryFileReader::TelemetryFileReader(QIODevice *d, const QString &name)
    : QObject()
{
    init(d, name);
}

bool TelemetryFileReader::is_still_writing()
{
    auto lockFile = TelemetryFileWriter::get_lock_file(name());
    if (!lockFile)
        return true;
    delete lockFile;
    return false;
}

QString TelemetryFileReader::get_hash(QIODevice *d)
{
    if (!d || !d->isOpen())
        return {};

    auto pos_s = d->pos();
    if (!d->seek(0))
        return {};

    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(d->readAll());
    d->seek(pos_s);
    return QString(hash.result().toHex().toUpper());
}

QString TelemetryFileReader::get_hash(QString filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return get_hash(&f);
}

void TelemetryFileReader::setProgress(int value)
{
    if (_progress == value)
        return;
    _progress = value;
    emit progressChanged(value);
}

bool TelemetryFileReader::init(QIODevice *d, const QString &name)
{
    setDevice(d);
    _reset_data();
    _name = name;

    if (!d)
        return false;

    if (!isOpen()) {
        qWarning() << "file not open" << name << d;
        return false;
    }

    if (!seek(0)) {
        qWarning() << "failed to seek to start";
        return false;
    }

    return parse_header();
}

bool TelemetryFileReader::parse_header()
{
    _info = {};
    _fhdr = {};

    if (!isOpen()) {
        qWarning() << "file not open";
        return false;
    }

    do {
        // read file header
        auto &h = _fhdr;
        if (!read(&h, sizeof(h))) {
            qWarning() << "failed to read file header";
            break;
        }

        if (strcmp(h.magic, APXTLM_MAGIC)) {
            qWarning() << "invalid file magic";
            break;
        }

        if (h.version > APXTLM_VERSION) {
            qWarning() << "invalid file version" << h.version << APXTLM_VERSION;
            break;
        }

        if (h.payload_offset > pos()) {
            qWarning() << "invalid file header size" << h.payload_offset << pos();
        }

        if (h.timestamp < QDateTime(QDate(2000, 1, 1), QTime(0, 0)).toMSecsSinceEpoch()) {
            qWarning() << "invalid file timestamp" << h.timestamp;
            break;
        }

        // file format seem to be valid

        // _info = _read_info();

        auto &m = _info;

        auto info = _read_info();
        for (auto i = info.constBegin(); i != info.constEnd(); ++i) {
            m[i.key()] = i.value();
        }

        m["timestamp"] = (qint64) h.timestamp;
        m["utc_offset"] = h.utc_offset;

        m["size"] = size();

        emit infoUpdated(m);
        return true;
    } while (0);

    // some error occured
    _reset_data();
    _fhdr = {};
    _info = {};
    return false;
}

QJsonObject TelemetryFileReader::_read_info()
{
    if (!isOpen() || !_fhdr.payload_offset)
        return {};

    if (!seek(_fhdr.payload_offset)) {
        qWarning() << "failed to seek to payload offset" << _fhdr.payload_offset << size();
        return {};
    }

    auto pos_s = pos();

    do {
        // try read info data at payload offset
        auto dspec = _read_dspec();
        if (dspec.spec_ext.dspec != dspec_e::ext || dspec.spec_ext.extid != extid_e::jso) {
            qWarning() << "invalid info spec";
            break;
        }

        bool ok = false;
        auto name = _read_string(&ok);
        if (name.isEmpty() || !ok) {
            qWarning() << "failed to read info data name";
            break;
        }
        auto jso = _read_jso_content();
        if (jso.isEmpty()) {
            qWarning() << "failed to read info data";
            break;
        }
        // info data read, file at telemetry data start
        return jso;
    } while (0);

    qWarning() << "failed to read info data" << pos_s << pos();
    if (!seek(_fhdr.payload_offset)) {
        qWarning() << "failed to seek back to payload offset" << _fhdr.payload_offset << size();
        return {};
    }
    return {};
}

bool TelemetryFileReader::parse_payload()
{
    _reset_data();

    if (!isOpen()) {
        qWarning() << "file not open";
        return false;
    }

    // get whole file hash
    auto hash = get_hash(device());
    if (hash.isEmpty()) {
        qWarning() << "failed to get file hash";
        return false;
    }
    _info["hash"] = hash;

    if (!seek(_fhdr.payload_offset)) {
        qWarning() << "failed to seek to payload offset" << _fhdr.payload_offset;
        return false;
    }

    // read all data and collect statistics
    auto pos_start = pos();
    bool ret = false;
    while (isOpen() && !atEnd()) {
        setProgress((pos() - pos_start) * 100 / (size() - pos_start));
        if (_interrupted) {
            qWarning() << "interrupted at" << pos();
            break;
        }

        ret = parse_next();

        if (!ret) {
            qWarning() << "failed to read data at" << pos() << "0x" + QString::number(pos(), 16);
            break;
        }
    }

    _commit_values(); // commit values (if any) collected for timestamp

    if (!ret || !isOpen()) {
        qWarning() << "failed to parse file at" << pos() << size();
    } else {
        // file read successfully
        qDebug() << "file parsed" << _ts_s << _field_index.size();
    }

    setProgress(0);

    // update info data
    _info["duration"] = (qint64) _ts_s;
    _info["payload_size"] = pos() - pos_start;
    _info["parsed"] = QDateTime::currentDateTime().toMSecsSinceEpoch();

    QJsonObject counters;

    if (_counters.records > 0)
        counters["records"] = _counters.records;

    if (_counters.uplink > 0)
        counters["uplink"] = _counters.uplink;

    if (_counters.downlink > 0)
        counters["downlink"] = _counters.downlink;

    if (_counters.ts > 0)
        counters["ts"] = _counters.ts;

    if (_counters.dir > 0)
        counters["dir"] = _counters.dir;

    if (_counters.field > 0)
        counters["field"] = _counters.field;

    if (_counters.evtid > 0)
        counters["evtid"] = _counters.evtid;

    int evt_total = 0;
    for (auto [key, value] : _counters.evt_by_name) {
        counters[key] = value;
        evt_total += value;
    }
    if (evt_total > 0)
        counters["evt"] = evt_total;

    int jso_total = 0;
    for (auto [key, value] : _counters.jso_by_name) {
        counters[key] = value;
        jso_total += value;
    }
    if (jso_total > 0)
        counters["jso"] = jso_total;

    int raw_total = 0;
    for (auto [key, value] : _counters.raw_by_name) {
        counters[key] = value;
        raw_total += value;
    }
    if (raw_total > 0)
        counters["raw"] = raw_total;

    _info["counters"] = counters;

    QJsonArray jso_names;
    for (auto i : _jso_s.keys())
        jso_names.append(i);
    _info["jso_names"] = jso_names;

    QJsonArray evt_names;
    for (const auto &i : _evt_index)
        evt_names.append(i.name);
    _info["evt_names"] = evt_names;

    _info = json::fix_numbers(json::filter_names(_info));

    emit infoUpdated(_info);

    // success
    setProgress(-1);
    return ret;
}

bool TelemetryFileReader::parse_next()
{
    if (!isOpen()) {
        qWarning() << "file not open";
        return false;
    }

    auto dspec = _read_dspec();
    if (dspec._raw8 == 0) {
        qWarning() << "stream stop marker at" << pos();
        seek(pos() - 1); // rollback to stop marker (begin of stats)
        _interrupted = true;
        return true;
    }

    _counters.records++;

    bool is_uplink = _next_uplink;
    _next_uplink = false;

    // qDebug() << "@" + QString::number(QFile::pos(), 16) << QString::number(dspec._raw16, 16);

    switch (dspec.spec8.dspec) {
    default: {
        _widx = dspec.spec8.opt8 ? (_widx + dspec.spec8.vidx_delta + 1) : (dspec.spec16.vidx);
        if (_widx >= _field_index.size()) {
            qWarning() << "invalid field index" << _widx << _field_index.size();
            break;
        }

        auto v = _read_value(dspec.spec8.dspec);
        if (v.isNull())
            break;

        if (is_uplink) {
            // if (_uplink_values.contains(_widx)) {
            //     qWarning() << "duplicate uplink value" << _fields.value(_widx).name
            //                << _uplink_values[_widx] << v;
            // }
            _uplink_values[_widx] = v;
        } else {
            // if (_downlink_values.contains(_widx)) {
            //     qWarning() << "duplicate downlink value" << _fields.value(_widx).name
            //                << _downlink_values[_widx] << v;
            // }
            _downlink_values[_widx] = v;
        }

        return true;
    }
    case dspec_e::ext:
        return _read_ext(dspec.spec_ext.extid, is_uplink);
    }

    return false;
}

void TelemetryFileReader::_reset_data()
{
    _ts_s = 0;
    _widx = 0;
    _next_uplink = false;

    _counters = {};
    _interrupted = false;

    _downlink_values.clear();
    _uplink_values.clear();

    _field_index.clear();
    _evt_index.clear();

    _jso_s.clear();
}

dspec_s TelemetryFileReader::_read_dspec()
{
    if (!isOpen() || atEnd())
        return {}; // i.e. stream stop marker

    dspec_s dspec{};
    if (!read(&dspec, 1)) {
        qWarning() << "failed to read data spec";
        return {};
    }
    if (dspec.spec8.dspec == dspec_e::ext)
        return dspec; // always 8-bit

    // read high byte if necessary
    if (!dspec.spec8.opt8) {
        if (!read((char *) &dspec + 1, 1)) {
            qWarning() << "failed to read data spec high byte";
            return {};
        }
    }
    return dspec;
}

QString TelemetryFileReader::_read_string(bool *ok)
{
    *ok = false;
    if (!isOpen() || atEnd())
        return {};

    QByteArray ba;
    while (ba.size() < MAX_STRLEN) {
        char c;
        if (!read(&c, 1)) {
            break;
        }
        if (c == 0) {
            *ok = true;
            return QString::fromUtf8(ba);
        }
        ba.append(c);
    }
    qWarning() << "failed to read string data";
    return {};
}

QJsonObject TelemetryFileReader::_read_jso_content()
{
    if (!isOpen() || atEnd())
        return {};

    uint32_t size;
    if (!read(&size, sizeof(size))) {
        qWarning() << "failed to read jso data size";
        return {};
    }

    auto fsize = this->size() - pos();
    if (size > fsize) {
        qWarning() << "invalid jso data size" << size << fsize;
        return {};
    }

    QByteArray zip_data(size, 0);
    if (!read(zip_data.data(), size)) {
        qWarning() << "failed to read jso data" << size;
        return {};
    }

    auto data = qUncompress(zip_data);
    if (data.isEmpty()) {
        qWarning() << "failed to uncompress jso data";
        return {};
    }

    QJsonParseError error;
    auto obj = QJsonDocument::fromJson(data, &error).object();
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "failed to parse jso data" << error.errorString();
        return {};
    }

    return obj;
}

QVariant TelemetryFileReader::_read_value(telemetry::dspec_e dspec)
{
    if (!isOpen() || atEnd())
        return {};

    bool ok = false;
    QVariant v;

    switch (dspec) {
    default:
        break;

    case dspec_e::u8:
        v = _read_raw<uint8_t>(&ok);
        break;
    case dspec_e::u16:
        v = _read_raw<uint16_t>(&ok);
        break;
    case dspec_e::u24:
        v = _read_raw<uint32_t>(&ok, 3);
        break;
    case dspec_e::u32:
        v = _read_raw<uint32_t>(&ok);
        break;
    case dspec_e::u64:
        v = _read_raw<uint64_t>(&ok);
        break;

    case dspec_e::f16:
        v = xbus::telemetry::float_from_f16(_read_raw<uint16_t>(&ok));
        break;
    case dspec_e::f32:
        v = _read_raw<float>(&ok);
        break;
    case dspec_e::f64:
        v = _read_raw<double>(&ok);
        break;

    case dspec_e::null:
        v = QVariant(0u);
        ok = true;
        break;

    case dspec_e::a16:
        v = xbus::telemetry::float_from_angle(_read_raw<int16_t>(&ok), 180.f);
        break;
    case dspec_e::a32:
        v = mandala::a32_to_deg(_read_raw<uint32_t>(&ok));
        break;
    }

    if (ok)
        return v;

    QString s = (uint) dspec < sizeof(dspec_names) / sizeof(dspec_names[0])
                    ? dspec_names[(uint) dspec]
                    : "unknown";
    qWarning() << "failed to read data value" << (uint) dspec << s;
    return {};
}

bool TelemetryFileReader::_read_ext(telemetry::extid_e extid, bool is_uplink)
{
    if (!isOpen() || atEnd())
        return {};

    bool ok = false;
    switch (extid) {
    default:
        break;

    case extid_e::ts: { // [ms] u32 timestamp update relative to file
        if (is_uplink)
            break;
        auto ts = _read_raw<uint32_t>(&ok);
        if (!ok)
            break;
        if (ts == _ts_s) {
            // should never happen
            qWarning() << "duplicate timestamp" << ts;
        }
        _commit_values();
        _ts_s = ts;
        _counters.ts++;
        return true;
    }
    case extid_e::dir:
        _next_uplink = true;
        _counters.dir++;
        return true;

    case extid_e::field: { // [name,N,str1,...strN] field defs
        if (is_uplink)
            break;
        auto info = _read_reg();
        if (info.isEmpty())
            break;

        _counters.field++;
        auto name = info.takeFirst();

        Field f{name, info};
        _field_index.push_back(f);
        emit field(f);
        return true;
    }
    case extid_e::evtid: {
        if (is_uplink)
            break;
        auto info = _read_reg();
        if (info.isEmpty())
            break;
        auto name = info.takeFirst();
        _evt_index.push_back({name, info});
        _counters.evtid++;
        return true;
    }

    case extid_e::evt: { // [evtid,str1,...strN] event data
        // find eventid
        auto evt_index = _read_raw<uint8_t>(&ok);
        if (!ok)
            break;
        if (evt_index >= _evt_index.size()) {
            qWarning() << "invalid event index" << evt_index << _evt_index.size();
            break;
        }
        const auto &evtid = _evt_index[evt_index];
        auto name = evtid.name;
        // read values
        QJsonObject jso;
        for (int i = 0; i < evtid.info.size(); ++i) {
            auto value = _read_string(&ok);
            if (!ok)
                break;
            jso[evtid.info[i]] = value;
        }

        _counters.evt_by_name[name]++;

        // qDebug() << "evt" << name << value << uid << is_uplink;
        emit evt(_ts_s, name, jso, is_uplink);
        return true;
    }

    case extid_e::jso: { // [name,size(32),jso_zip(...)] (nodes,mission)
        auto name = _read_string(&ok);
        if (name.isEmpty() || !ok) {
            qWarning() << "failed to read jso data name";
            break;
        }
        auto jso_data = _read_jso_content();
        if (jso_data.isEmpty()) {
            qWarning() << "failed to read jso data";
            break;
        }
        // merge with previous data
        if (_jso_s.contains(name)) {
            auto jso_merged = json::merge(_jso_s[name], jso_data);
            if (jso_merged.isEmpty()) {
                qWarning() << "failed to patch jso data";
                break;
            }
            jso_data.swap(jso_merged);
        }
        _jso_s[name] = jso_data;

        _counters.jso_by_name[name]++;

        // qDebug() << "jso" << name << data.size() << is_uplink;
        emit jso(_ts_s, name, jso_data, is_uplink);
        return true;
    }

    case extid_e::raw:
    case extid_e::zip: { // [id(16),size(16),data(...)] raw data (serial vcp)
        const auto name = _read_string(&ok);
        if (name.isEmpty() || !ok) {
            qWarning() << "failed to read jso data name";
            break;
        }

        bool zip = extid == extid_e::zip;

        auto size = zip ? _read_raw<uint32_t>(&ok) : _read_raw<uint16_t>(&ok);
        if (!ok)
            break;
        auto fsize = this->size() - pos();
        if (size == 0 || size > fsize) {
            qWarning() << "invalid raw data size" << size << fsize;
            break;
        }
        QByteArray data(size, 0);
        if (!read(data.data(), size)) {
            qWarning() << "failed to read raw data" << size;
            break;
        }
        if (zip) {
            data = qUncompress(data);
            if (data.isEmpty()) {
                qWarning() << "failed to uncompress raw data";
                break;
            }
        }

        _counters.raw_by_name[name]++;

        // qDebug() << "raw" << id << size << is_uplink;
        emit raw(_ts_s, name, data, is_uplink);
        return true;
    }

        // switch end
    }

    qWarning() << "failed to read ext data" << (uint) extid << is_uplink;
    return false;
}

void TelemetryFileReader::_commit_values()
{
    if (!_downlink_values.empty()) {
        emit values(_ts_s, _downlink_values, false);
        _downlink_values.clear();
        _counters.downlink++;
    }
    if (!_uplink_values.empty()) {
        emit values(_ts_s, _uplink_values, true);
        _uplink_values.clear();
        _counters.uplink++;
    }
}

QStringList TelemetryFileReader::_read_reg()
{
    bool ok = false;
    auto name = _read_string(&ok);
    if (name.isEmpty() || !ok)
        return {};

    QStringList strings{{name}};
    size_t N = _read_raw<uint8_t>(&ok);
    while (ok && N--) {
        strings.append(_read_string(&ok));
    }
    if (!ok)
        return {};
    return strings;
}
