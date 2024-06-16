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

bool TelemetryFileReader::is_still_writing()
{
    auto lockFile = TelemetryFileWriter::get_lock_file(fileName());
    if (!lockFile)
        return true;
    delete lockFile;
    return false;
}

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

    _info = TelemetryFileWriter::get_info_from_filename(fileName());

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
    close();
    _reset_data();
    _fhdr = {};
    _info = {};
    return false;
}

bool TelemetryFileReader::fix_name()
{
    // fix rename if necessary
    auto time = QDateTime::fromMSecsSinceEpoch(timestamp(), QTimeZone(utc_offset()));
    auto callsign = info()["vehicle"]["callsign"].toString();
    auto fixedFileName = TelemetryFileWriter::prepare_file_name(time, callsign);
    auto fi = QFileInfo(fileName());
    if (fixedFileName != fi.fileName()) {
        auto newPath = fi.dir().absoluteFilePath(fixedFileName);
        // rename file
        close();

        bool ok = rename(newPath);
        if (!ok) {
            qWarning() << "Failed to rename" << fi.fileName() << "to" << fixedFileName;
            if (QFile::exists(newPath)) {
                qWarning() << "File" << newPath << "already exists";
                // compare two files and remove the old one if equal
                auto hash = get_hash();
                auto newHash = TelemetryFileReader(newPath).get_hash();
                if (hash == newHash) {
                    qDebug() << "Removing duplicate" << fi.fileName();
                    QFile::remove(newPath);
                    ok = rename(newPath);
                } else {
                    // rename with some name suffix
                    fixedFileName = TelemetryFileWriter::prepare_file_name(time,
                                                                           callsign,
                                                                           fi.dir().absolutePath());
                    newPath = fi.dir().absoluteFilePath(fixedFileName);
                    ok = rename(newPath);
                    if (!ok) {
                        qWarning() << "Failed to rename" << fi.fileName() << "to" << fixedFileName;
                    }
                }
            }
        }
        if (ok) {
            qDebug() << "Renamed" << fi.fileName() << "to" << fixedFileName;
        }

        if (!open(newPath))
            return false;
    }
    return true;
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
        if (dspec.spec_ext.dspec != dspec_e::ext || dspec.spec_ext.extid != extid_e::meta) {
            qWarning() << "invalid info spec";
            break;
        }

        bool ok = false;
        auto name = _read_string(&ok);
        if (name.isEmpty() || !ok) {
            qWarning() << "failed to read info data name";
            break;
        }
        auto data = _read_meta_data();
        if (data.isEmpty()) {
            qWarning() << "failed to read info data";
            break;
        }
        // info data read, file at telemetry data start
        return data;
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
    auto hash = get_hash();
    if (hash.isEmpty()) {
        qWarning() << "failed to get file hash";
        return false;
    }
    _info["hash"] = QString(hash.toHex().toUpper());

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

    setProgress(-1);

    if (!ret || !isOpen()) {
        qWarning() << "failed to parse file at" << pos() << size();
        return false;
    }

    // file read successfully

    qDebug() << "file parsed" << _ts_s << _fields.size();

    setProgress(0);

    // update info data
    _info["duration"] = (qint64) _ts_s;
    _info["payload_size"] = pos() - pos_start;
    _info["parsed"] = QDateTime::currentDateTime().toMSecsSinceEpoch();

    QJsonObject counters;
    if (_counters.records)
        counters["records"] = _counters.records;
    if (_counters.fields)
        counters["fields"] = _counters.fields;
    if (_counters.uplink)
        counters["uplink"] = _counters.uplink;
    if (_counters.downlink)
        counters["downlink"] = _counters.downlink;
    if (_counters.evt)
        counters["evt"] = _counters.evt;
    if (_counters.msg)
        counters["msg"] = _counters.msg;
    if (_counters.meta)
        counters["meta"] = _counters.meta;
    if (_counters.raw)
        counters["raw"] = _counters.raw;
    if (_counters.mission)
        counters["mission"] = _counters.mission;
    if (_counters.nodes)
        counters["nodes"] = _counters.nodes;
    if (_counters.conf)
        counters["conf"] = _counters.conf;

    _info["counters"] = counters;

    QJsonArray meta_names;
    for (auto i : _meta_objects.keys())
        meta_names.append(i);
    _info["meta_names"] = meta_names;

    QJsonArray evt_names;
    for (auto i : _evt_names)
        evt_names.append(i);
    _info["evt_names"] = evt_names;

    emit infoUpdated(_info);

    // success
    setProgress(-1);
    emit parsed();
    return true;
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

    _fields.clear();
    _meta_objects.clear();
    _evt_names.clear();

    _downlink_values.clear();
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
    while (ba.size() < MAX_STRLEN) {
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

        _counters.evt++;

        if (!_evt_names.contains(name))
            _evt_names.append(name);

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
