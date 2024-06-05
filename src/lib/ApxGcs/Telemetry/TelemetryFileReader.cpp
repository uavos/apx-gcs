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
#include <Vehicles/Vehicle.h>

#include <TelemetryValuePack.h>
#include <TelemetryValueUnpack.h>

using namespace telemetry;

TelemetryFileReader::TelemetryFileReader(QString filePath, QObject *parent)
    : QFile(parent)
{
    setFileName(filePath);
}

TelemetryFileReader::TelemetryFileReader(QObject *parent)
    : QFile(parent)
{}

QByteArray TelemetryFileReader::get_hash()
{
    if (!isOpen()) {
        // open file for reading
        if (!QFile::open(QIODevice::ReadOnly)) {
            qWarning() << "failed to open file" << fileName();
            return {};
        }
    }

    if (!QFile::seek(0))
        return {};

    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(this);
    return hash.result();
}

void TelemetryFileReader::setProgress(int value)
{
    if (_progress == value)
        return;
    _progress = value;
    emit progressChanged(value);
}

bool TelemetryFileReader::open(QString filePath)
{
    _reset_data();

    if (isOpen()) {
        qDebug() << "file break";
        close();
    }

    QFile::setFileName(filePath);

    // open file for reading
    if (!QFile::open(QIODevice::ReadOnly)) {
        qWarning() << "failed to open file" << fileName();
        return false;
    }

    // qDebug() << "file opened" << fileName();
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
        if (read((char *) &h, sizeof(h)) != sizeof(h)) {
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

        if (h.payload_offset > sizeof(h) || h.payload_offset < fhdr_s::SIZE_MIN) {
            qWarning() << "invalid file header size" << h.payload_offset << sizeof(h);
            break;
        }

        // check header crc
        auto hcrc = TelemetryFileWriter::get_hdr_crc(&h);
        if (h.info.hcrc != hcrc) {
            qWarning() << "invalid file header crc" << h.info.hcrc << hcrc;
            break;
        }

        if (h.info.flags.parsed) {
            // check payload size
            auto pld_size = (h.info.stats_offset ? h.info.stats_offset : QFile::size())
                            - h.payload_offset;
            if (h.info.payload_size != pld_size) {
                qWarning() << "invalid file payload size" << h.info.payload_size << pld_size;
                break;
            }

            // check stats offset
            if (h.info.stats_offset > 0
                && (h.info.stats_offset < h.payload_offset
                    || h.info.stats_offset >= (QFile::size() - 4))) {
                qWarning() << "invalid file stats offset" << h.info.stats_offset << h.payload_offset
                           << QFile::size();
                break;
            }
        }

        // file format seem to be valid
        // read all metadata available
        auto &m = _info;

        auto fi = QFileInfo(*this);
        m["name"] = fi.completeBaseName();
        m["path"] = fi.absoluteFilePath();

        m["timestamp"] = (qint64) h.timestamp;
        m["utc_offset"] = h.utc_offset;

        m["sw_version"]
            = QString("%1.%2.%3").arg(h.sw.version[0]).arg(h.sw.version[1]).arg(h.sw.version[2]);
        m["sw_hash"] = QString::number(h.sw.hash, 16);

        // seek back to the beginning of the payload if necessary
        if (h.payload_offset < sizeof(h)) {
            qDebug() << "seeking back to payload:"
                     << (int64_t) h.payload_offset - (int64_t) sizeof(h);
            QFile::seek(h.payload_offset);
        }

        // read tags
        QJsonObject tags;
        XbusStreamReader stream(h.tags, sizeof(h.tags));
        while (stream.available() > 0) {
            auto c = stream.read_string(stream.available());
            if (!c)
                break;
            QString s = QString::fromUtf8((const char *) c);

            QString key, value;
            if (s.contains(':')) {
                key = s.section(':', 0, 0).trimmed();
                value = s.section(':', 1).trimmed();
            } else {
                key = s;
            }

            if (key.isEmpty())
                continue;

            if (m.contains(key)) {
                if (m.value(key) != value)
                    tags[key] = value;
            } else
                m[key] = value;
        }
        if (!tags.empty())
            m["ovr"] = tags;

        // fill info from header
        if (!h.info.flags.parsed) {
            // qDebug() << "file not parsed yet";
            emit infoUpdated(m);
            return true;
        }

        // file parsed and contains stats
        m["duration"] = (qint64) h.info.tmax;
        m["payload_size"] = (qint64) h.info.payload_size;
        m["edited"] = (bool) h.info.flags.edited;
        m["parse_ts"] = (qint64) h.info.parse_ts;

        // read meta data
        auto data = _read_stats();
        if (data.empty()) {
            qWarning() << "failed to read stats";
            h.info.flags.parsed = false;
            h.info.flags.corrupted = true;
        } else {
            m["stats"] = data;
        }

        m["parsed"] = (bool) h.info.flags.parsed;
        m["corrupted"] = (bool) h.info.flags.corrupted;

        // qDebug() << "file header parsed";

        emit infoUpdated(m);
        return true;
    } while (0);

    // some error occured
    close();
    _reset_data();
    _fhdr = {};
    _info = {};
    return false;
}

bool TelemetryFileReader::parse_payload()
{
    _reset_data();

    if (!isOpen()) {
        qWarning() << "file not open";
        return false;
    }

    if (!QFile::seek(_fhdr.payload_offset)) {
        qWarning() << "failed to seek to payload offset" << _fhdr.payload_offset;
        return false;
    }

    // read all data and collect statistics
    quint64 payload_size = _fhdr.info.payload_size ? _fhdr.info.payload_size
                                                   : (QFile::size() - _fhdr.payload_offset);
    quint64 payload_pos_max = _fhdr.payload_offset + payload_size;
    bool ret = false;
    while (isOpen() && !QFile::atEnd() && QFile::pos() < payload_pos_max) {
        setProgress((QFile::pos() - _fhdr.payload_offset) * 100
                    / (QFile::size() - _fhdr.payload_offset));

        auto dspec = _read_dspec();
        if (dspec._raw8 == 0) {
            qWarning() << "stream stop marker at" << QFile::pos();
            QFile::seek(QFile::pos() - 1); // rollback to stop marker (begin of stats)
            _fhdr.info.flags.corrupted = true;
            _fhdr.info.stats_offset = 0; // invalidate stats
            // still ok to return success
            payload_pos_max = QFile::pos();
            ret = true;
            break;
        }

        _counters.records++;

        bool is_uplink = _next_uplink;
        _next_uplink = false;

        // qDebug() << "@" + QString::number(QFile::pos(), 16) << QString::number(dspec._raw16, 16);

        ret = false;
        switch (dspec.spec8.dspec) {
        default: {
            _widx = dspec.spec8.opt8 ? (_widx + dspec.spec8.vidx_delta + 1) : (dspec.spec16.vidx);
            if (_widx >= _fields.size()) {
                qWarning() << "invalid field index" << _widx << _fields.size();
                break;
            }

            auto v = _read_value(dspec.spec8.dspec);
            if (v.isNull())
                break;

            if (is_uplink) {
                if (_uplink_values.contains(_widx))
                    qWarning() << "duplicate uplink value" << _fields.value(_widx).name;
                _uplink_values[_widx] = v;
            } else {
                if (_downlink_values.contains(_widx))
                    qWarning() << "duplicate downlink value" << _fields.value(_widx).name;
                _downlink_values[_widx] = v;
            }

            ret = true;
            break;
        }
        case dspec_e::ext:
            ret = _read_ext(dspec.spec_ext.extid, is_uplink);
            break;
        }

        if (!ret) {
            qWarning() << "failed to read data at" << QFile::pos()
                       << "0x" + QString::number(QFile::pos(), 16);
            break;
        }
    }

    setProgress(-1);

    // check payload_size constraints
    if (QFile::pos() != payload_pos_max) {
        qWarning() << "invalid payload size" << QFile::pos() << payload_pos_max;
        return false;
    }

    if (!ret || !isOpen()) {
        qWarning() << "failed to parse file at" << QFile::pos() << QFile::size();
        return false;
    }

    // read always ends at payload end
    // update file header payload stop marker when needed
    _fhdr.info.payload_size = QFile::pos() - _fhdr.payload_offset;

    if (_fhdr.info.stats_offset && _fhdr.info.stats_offset != QFile::pos()) {
        qWarning() << "invalid stats offset" << _fhdr.info.stats_offset << QFile::pos();
        _fhdr.info.stats_offset = 0;
    }

    // file read successfully
    _commit_values();

    qDebug() << "file parsed" << _ts_s << _fields.size();

    setProgress(0);

    // update file header
    auto parse_ts = QDateTime::currentDateTime().toMSecsSinceEpoch();
    bool was_parsed = _fhdr.info.flags.parsed;
    if (!was_parsed) {
        _fhdr.info.flags.parsed = true;
        _fhdr.info.parse_ts = parse_ts;
    }
    _fhdr.info.tmax = _ts_s;

    _info["duration"] = (qint64) _fhdr.info.tmax;
    _info["payload_size"] = (qint64) _fhdr.info.payload_size;

    // collect and update metadata
    _update_stats();

    // payload sha1
    {
        QCryptographicHash sha1(QCryptographicHash::Sha1);
        QFile::seek(_fhdr.payload_offset);
        auto sz = _fhdr.info.payload_size;
        while (!QFile::atEnd() && sz > 0) {
            auto cnt = sz > 4096 ? 4096 : sz;
            auto data = QFile::read(cnt);
            sha1.addData(data);
            sz -= cnt;
        }
        auto ba = sha1.result();
        if (ba.size() > sizeof(_fhdr.info.sha1))
            ba.resize(sizeof(_fhdr.info.sha1));
        memset(_fhdr.info.sha1, 0, sizeof(_fhdr.info.sha1));
        memcpy(_fhdr.info.sha1, ba.constData(), ba.size());
    }

    // header crc
    auto hcrc = TelemetryFileWriter::get_hdr_crc(&_fhdr);
    if (!was_parsed || _fhdr.info.hcrc != hcrc) {
        qDebug() << "file header update:" << QFileInfo(fileName()).completeBaseName();
        _fhdr.info.parse_ts = parse_ts;
        _fhdr.info.hcrc = TelemetryFileWriter::get_hdr_crc(&_fhdr);

        _update_header();
    }

    // success
    setProgress(-1);
    emit parsed();
    return true;
}

void TelemetryFileReader::_reset_data()
{
    _ts_s = 0;
    _widx = 0;
    _next_uplink = false;

    _counters = {};

    _fields.clear();
    _meta_objects.clear();

    _downlink_values.clear();
    _downlink_values.clear();

    _uplink_values.clear();
    _uplink_values.clear();
}

dspec_s TelemetryFileReader::_read_dspec()
{
    if (!isOpen() || QFile::atEnd())
        return {}; // i.e. stream stop marker

    dspec_s dspec{};
    if (QFile::read((char *) &dspec, 1) != 1) {
        qWarning() << "failed to read data spec";
        return {};
    }
    if (dspec.spec8.dspec == dspec_e::ext)
        return dspec; // always 8-bit

    // read high byte if necessary
    if (!dspec.spec8.opt8) {
        if (QFile::read((char *) &dspec + 1, 1) != 1) {
            qWarning() << "failed to read data spec high byte";
            return {};
        }
    }
    return dspec;
}

QString TelemetryFileReader::_read_string(bool *ok)
{
    *ok = false;
    if (!isOpen() || QFile::atEnd())
        return {};

    QByteArray ba;
    while (ba.size() < 1024) {
        char c;
        if (QFile::read(&c, 1) != 1) {
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

QJsonObject TelemetryFileReader::_read_meta_data()
{
    if (!isOpen() || QFile::atEnd())
        return {};

    uint32_t size;
    if (QFile::read((char *) &size, sizeof(size)) != sizeof(size)) {
        qWarning() << "failed to read meta data size";
        return {};
    }

    auto fsize = QFile::size() - QFile::pos();
    if (size > fsize) {
        qWarning() << "invalid meta data size" << size << fsize;
        return {};
    }

    auto zip_data = QFile::read(size);
    if (zip_data.size() != size) {
        qWarning() << "failed to read meta data" << zip_data.size() << size;
        return {};
    }

    auto data = qUncompress(zip_data);
    if (data.isEmpty()) {
        qWarning() << "failed to uncompress meta data";
        return {};
    }

    QJsonParseError error;
    auto obj = QJsonDocument::fromJson(data, &error).object();
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "failed to parse meta data" << error.errorString();
        return {};
    }

    return obj;
}

QJsonObject TelemetryFileReader::_read_stats()
{
    if (!_fhdr.info.stats_offset)
        return {};

    if (!QFile::seek(_fhdr.info.stats_offset)) {
        qWarning() << "failed to seek to stats offset" << _fhdr.info.stats_offset;
        return {};
    }
    auto dspec = _read_dspec();
    if (dspec.spec_ext.dspec != dspec_e::ext || dspec.spec_ext.extid != extid_e::stop) {
        qWarning() << "invalid stats spec";
        return {};
    }
    auto data = _read_meta_data();
    if (data.isEmpty()) {
        qWarning() << "failed to read meta data";
        return {};
    }
    return data;
}

QVariant TelemetryFileReader::_read_value(telemetry::dspec_e dspec)
{
    if (!isOpen() || QFile::atEnd())
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
        v = mandala::from_gps(_read_raw<uint32_t>(&ok));
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
    if (!isOpen() || QFile::atEnd())
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
        if (ts <= _ts_s) {
            // must never happen
            qWarning() << "invalid timestamp" << ts << _ts_s;
            break;
        }
        _commit_values();
        _ts_s = ts;
        return true;
    }
    case extid_e::uplink:
        _next_uplink = true;
        return true;

    case extid_e::field: { // [name,title,units] strings of used fields sequence
        if (is_uplink)
            break;
        auto name = _read_string(&ok);
        if (name.isEmpty() || !ok)
            break;
        auto title = _read_string(&ok);
        if (!ok)
            break;
        auto units = _read_string(&ok);
        if (!ok)
            break;

        _fields.append({name, title, units});
        _counters.fields++;

        emit field(name, title, units);
        return true;
    }

    case extid_e::evt: { // [name,value,uid] event data
        auto name = _read_string(&ok);
        if (name.isEmpty() || !ok)
            break;
        auto value = _read_string(&ok);
        if (!ok)
            break;
        auto uid = _read_string(&ok);
        if (!ok)
            break;
        auto end = _read_raw<uint8_t>(&ok);
        if (!ok || end)
            break;

        _counters.evt++;

        if (name == "conf")
            _counters.conf++;

        // qDebug() << "evt" << name << value << uid << is_uplink;
        emit evt(_ts_s, name, value, uid, is_uplink);
        return true;
    }

    case extid_e::msg: { // [text,subsystem] message data
        if (is_uplink)
            break;

        auto text = _read_string(&ok);
        if (!ok)
            break;
        auto subsystem = _read_string(&ok);
        if (!ok)
            break;
        auto end = _read_raw<uint8_t>(&ok);
        if (!ok || end)
            break;

        _counters.msg++;

        // qDebug() << "msg" << text << subsystem;
        emit msg(_ts_s, text, subsystem);
        return true;
    }

    case extid_e::meta: { // [name,size(32),meta_zip(...)] (nodes,mission)
        auto pos_s = QFile::pos();
        auto name = _read_string(&ok);
        if (name.isEmpty() || !ok) {
            qWarning() << "failed to read meta data name";
            break;
        }
        auto data = _read_meta_data();
        if (data.isEmpty()) {
            qWarning() << "failed to read meta data";
            break;
        }
        // merge with previous data
        if (_meta_objects.contains(name)) {
            QJsonObject result;
            _json_patch(_meta_objects[name], data, result);
            if (result.isEmpty()) {
                qWarning() << "failed to patch meta data";
                break;
            }
            data.swap(result);
        }
        _meta_objects[name] = data;

        _counters.meta++;
        if (name == "nodes") {
            _counters.nodes++;
        } else if (name == "mission") {
            _counters.mission++;
        }

        // qDebug() << "meta" << name << data.size() << is_uplink;
        emit meta(name, data, is_uplink);
        return true;
    }

    case extid_e::raw: { // [id(16),size(16),data(...)] raw data (serial vcp)
        auto id = _read_raw<uint16_t>(&ok);
        if (!ok)
            break;
        auto size = _read_raw<uint16_t>(&ok);
        if (!ok)
            break;
        auto fsize = QFile::size() - QFile::pos();
        if (size == 0 || size > fsize) {
            qWarning() << "invalid raw data size" << size << fsize;
            break;
        }
        auto data = QFile::read(size);
        if (data.size() != size) {
            qWarning() << "failed to read raw data" << data.size() << size;
            break;
        }

        _counters.raw++;

        // qDebug() << "raw" << id << size << is_uplink;
        emit raw(_ts_s, id, data, is_uplink);
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

void TelemetryFileReader::_json_patch(const QJsonObject &orig,
                                      const QJsonObject &patch,
                                      QJsonObject &result)
{
    for (auto it = patch.begin(); it != patch.end(); ++it) {
        auto key = it.key();
        auto value = it.value();

        if (value.isObject()) {
            if (!orig.contains(key)) {
                result[key] = value;
                continue;
            }
            QJsonObject sub;
            _json_patch(orig[key].toObject(), value.toObject(), sub);
            if (!sub.isEmpty())
                result[key] = sub;
        } else if (value.isArray()) {
            if (!orig.contains(key)) {
                result[key] = value;
                continue;
            }
            auto orig_a = value.toArray();
            auto patch_a = patch[key].toArray();
            QJsonArray sub;
            for (int i = 0; i < patch_a.size(); ++i) {
                QJsonObject d;
                _json_patch(orig_a.at(i).toObject(), patch_a.at(i).toObject(), d);
                sub.append(d);
            }
            if (!sub.isEmpty())
                result[key] = sub;
        } else if (value.isNull()) {
            result[key] = orig[key];
        } else {
            result[key] = value;
        }
    }
}

bool TelemetryFileReader::_update_header()
{
    auto lock_file = TelemetryFileWriter::get_lock_file(fileName());
    if (!lock_file)
        return false;

    // rewrite header
    bool ret = false;
    do {
        QFile::close();
        if (!QFile::open(QIODevice::ReadWrite)) {
            qWarning() << "failed to open file for writing";
            break;
        }
        if (QFile::write((const char *) &_fhdr, sizeof(_fhdr)) != sizeof(_fhdr)) {
            qWarning() << "failed to write file header data";
            break;
        }
        QFile::close();
        if (!QFile::open(QIODevice::ReadOnly)) {
            qWarning() << "failed to reopen file for reading";
            break;
        }
    } while (0);

    lock_file->unlock();

    if (!ret) {
        qWarning() << "failed to write file header";
        QFile::close();
        return false;
    }

    return true;
}

bool TelemetryFileReader::_update_stats()
{
    // update stats object
    QJsonObject counters;
    counters["records"] = _counters.records;
    counters["fields"] = _counters.fields;
    counters["uplink"] = _counters.uplink;
    counters["downlink"] = _counters.downlink;
    counters["evt"] = _counters.evt;
    counters["msg"] = _counters.msg;
    counters["meta"] = _counters.meta;
    counters["raw"] = _counters.raw;
    counters["mission"] = _counters.mission;
    counters["nodes"] = _counters.nodes;
    counters["conf"] = _counters.conf;

    auto stats_orig = _info["stats"].toObject();
    auto stats = stats_orig;
    stats["counters"] = counters;

    if (stats == stats_orig) // no changes
        return true;

    _info["stats"] = stats;

    auto stats_offset = _fhdr.info.stats_offset;

    // update meta data
    qDebug() << "file metadata" << (stats_offset ? "update" : "create")
             << QFileInfo(fileName()).completeBaseName();

    // write meta data
    if (!stats_offset) {
        stats_offset = _fhdr.payload_offset + _fhdr.info.payload_size;
        _fhdr.info.stats_offset = stats_offset;
    }

    emit infoUpdated(_info);

    // rewrite stats at tail

    auto lock_file = TelemetryFileWriter::get_lock_file(fileName());
    if (!lock_file)
        return false;

    bool ret = false;
    do {
        QFile::close();

        {
            TelemetryFileWriter writer;
            writer.setFileName(fileName());

            if (!writer.open(QIODevice::ReadWrite)) {
                qWarning() << "failed to open file for writing";
                break;
            }
            if (!writer.seek(stats_offset)) {
                qWarning() << "failed to seek to meta data offset" << stats_offset;
                break;
            }

            writer.write_stats(stats);
            writer.close();
        }

        if (!QFile::open(QIODevice::ReadOnly)) {
            qWarning() << "failed to reopen file for reading";
            break;
        }
        ret = true;
    } while (0);

    lock_file->unlock();

    if (!ret) {
        qWarning() << "failed to write file header";
        setProgress(-1);
        return false;
    }
    return true;
}
