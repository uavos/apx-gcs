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
#include "TelemetryReader.h"
#include "TelemetryRecords.h"

#include <App/App.h>
#include <App/AppLog.h>
#include <App/AppRoot.h>

#include <Database/StorageReq.h>

#include <Database/MissionsDB.h>
#include <Database/VehiclesReqVehicle.h>

#include <QGeoCoordinate>

TelemetryReader::TelemetryReader(Fact *parent)
    : Fact(parent, "reader", "", "", Group, "progress-download")
    , blockNotesChange(false)
    , m_totalSize(0)
    , m_totalTime(0)
{
    f_notes = new Fact(parent, "notes", tr("Notes"), tr("Current record notes"), Text);
    f_notes->setIcon("note-text");
    connect(f_notes, &Fact::valueChanged, this, &TelemetryReader::notesChanged);

    //actions
    f_reload = new Fact(this,
                        "reload",
                        tr("Reload"),
                        tr("Invalidate cache"),
                        Action | ShowDisabled,
                        "reload");
    f_reload->setEnabled(false);
    connect(f_reload, &Fact::triggered, this, [this]() { loadRecord(_loadRecordID); });

    //info
    connect(this, &TelemetryReader::totalTimeChanged, this, &TelemetryReader::updateStatus);
}

void TelemetryReader::updateStatus()
{
    const QString &s = AppRoot::timeToString(totalTime() / 1000, true);
    setValue(s);
}

void TelemetryReader::loadRecord(quint64 id)
{
    f_reload->setEnabled(false);

    setTotalSize(0);
    setTotalTime(0);
    setProgress(0);

    _loadRecordID = id;
    _fields.clear();
    _geoPath = {};
    _totalDistance = 0;
    _index_lat = -1;
    _index_lon = -1;
    _index_hmsl = -1;
    _geoPos = {};

    deleteChildren();
    _recordInfo = {};

    auto req = new db::storage::TelemetryLoadFile(id);
    connect(req, &DatabaseRequest::finished, this, [this]() {
        setProgress(-1);
        emit rec_finished();
        f_reload->setEnabled(true);
        emit geoPathCollected(_geoPath, _totalDistance);
        _geoPath = {};
    });
    auto reader = req->reader();
    connect(reader, &TelemetryFileReader::progressChanged, this, [this](int v) { setProgress(v); });
    connect(req, &db::storage::TelemetryLoadFile::recordInfo, this, &TelemetryReader::setRecordInfo);
    connect(req, &db::storage::TelemetryLoadFile::fileOpened, this, [this](QString path) {
        _recordFilePath = path;
    });

    // forward info to other facts (lists)
    connect(req,
            &db::storage::TelemetryLoadInfo::recordInfo,
            this,
            &TelemetryReader::recordInfoUpdated);

    connect(reader, &TelemetryFileReader::field, this, &TelemetryReader::rec_field);
    connect(reader, &TelemetryFileReader::values, this, &TelemetryReader::rec_values);
    connect(reader, &TelemetryFileReader::evt, this, &TelemetryReader::rec_evt);
    connect(reader, &TelemetryFileReader::jso, this, &TelemetryReader::rec_jso);
    connect(reader, &TelemetryFileReader::raw, this, &TelemetryReader::rec_raw);

    // processed by reader
    connect(reader, &TelemetryFileReader::field, this, &TelemetryReader::do_rec_field);
    connect(reader, &TelemetryFileReader::values, this, &TelemetryReader::do_rec_values);
    connect(reader, &TelemetryFileReader::evt, this, &TelemetryReader::do_rec_evt);
    connect(reader, &TelemetryFileReader::jso, this, &TelemetryReader::do_rec_jso);

    // start parsing
    emit rec_started();
    req->exec();
}

void TelemetryReader::do_rec_field(Field field)
{
    _fields.append(field);
    auto name = field.name;
    auto index = _fields.size() - 1;

    if (name == "est.pos.lat")
        _index_lat = index;
    else if (name == "est.pos.lon")
        _index_lon = index;
    else if (name == "est.pos.hmsl")
        _index_hmsl = index;
}
void TelemetryReader::do_rec_values(quint64 timestamp_ms, Values data, bool uplink)
{
    if (uplink) {
        for (auto [index, value] : data) {
            const auto &field = _fields[index];
            QJsonObject jso;
            jso["path"] = field.name;
            jso["title"] = field.info.value(0);
            jso["value"] = QJsonValue::fromVariant(value);
            addEventFact(timestamp_ms, "uplink", jso, uplink);
        }
        return;
    }

    // collect flight path
    for (auto [index, value] : data) {
        // normally all three components should be present in one record
        if (index == _index_lat)
            _geoPos.setLatitude(value.toDouble());
        else if (index == _index_lon)
            _geoPos.setLongitude(value.toDouble());
        else if (index == _index_hmsl)
            _geoPos.setAltitude(value.toDouble());
    }
    if (_geoPos.isValid()) {
        if (_geoPath.size() == 0)
            _geoPath.addCoordinate(_geoPos);
        else {
            auto dist = _geoPath.coordinateAt(_geoPath.size() - 1).distanceTo(_geoPos);
            _totalDistance += dist;
            if (dist >= 10)
                _geoPath.addCoordinate(_geoPos);
        }
        _geoPos = {};
    }
}
void TelemetryReader::do_rec_evt(quint64 timestamp_ms, QString name, QJsonObject data, bool uplink)
{
    addEventFact(timestamp_ms, name, data, uplink);
}

void TelemetryReader::do_rec_jso(quint64 timestamp_ms, QString name, QJsonObject data, bool uplink)
{
    addEventFact(timestamp_ms, name, data, uplink);

    if (_importedMeta.contains(_loadRecordID))
        return;

    // save meta objects to databases
    if (name == "mission") {
        // addEventFact(timestamp_ms, name, data["name"].toString(), data["uid"].toString(), data);
        // auto req = new DBReqMissionsSave(data.toVariantMap());
        // req->exec();
    } else if (name == "nodes") {
        // auto req = new DBReqImportVehicleConfig(data.toVariantMap());
        // req->exec();
        // qDebug("%s", QJsonDocument(data).toJson(QJsonDocument::Indented).constData());
    }
}

void TelemetryReader::setRecordInfo(quint64 id, QJsonObject info, QString notes)
{
    _recordInfo = info;
    const auto &m = info;

    setTotalTime(m["duration"].toInteger());

    const auto &cnt = m["counters"].toObject();

    setTotalSize(cnt["records"].toInteger());

    quint64 downlink = cnt["downlink"].toInteger();
    quint64 uplink = cnt["uplink"].toInteger();
    quint64 events = cnt["evt"].toInteger();

    qint64 t = m["timestamp"].toInteger();
    QString title = t > 0 ? QDateTime::fromMSecsSinceEpoch(t).toString("yyyy MMM dd hh:mm:ss")
                          : tr("Telemetry Data");
    QString unitName = m["unit"]["name"].toString();
    QString unitType = m["unit"]["type"].toString();
    QString comment = m["conf"].toString();
    QString stime = AppRoot::timeToString(totalTime() / 1000, true);

    QStringList descr;
    if (!unitType.isEmpty() && unitType != unitName && unitType != "UAV")
        descr.append(unitType);
    if (!unitName.isEmpty())
        descr.append(unitName);
    if (!comment.isEmpty() && comment != unitName)
        descr.append(comment);

    descr.append(QString("%1/%2/%3").arg(downlink).arg(uplink).arg(events));

    setTitle(title);
    setDescr(descr.join(" | "));

    blockNotesChange = true;
    f_notes->setValue(notes);
    blockNotesChange = false;

    // qDebug("%s", QJsonDocument(info).toJson(QJsonDocument::Indented).constData());

    // create info fact with text
    auto f = new Fact(this, "info", tr("Record Info"), tr("Telemetry record metadata"), Group);
    f->move(0);
    f->setIcon("information");
    f->setText("{}");
    f->setOpt("page", "Menu/FactMenuPageInfoText.qml");
    f->setOpt("info", QJsonDocument(_recordInfo).toJson(QJsonDocument::Indented).constData());

    _importedMeta.insert(id);

    emit recordInfoChanged();
}

void TelemetryReader::addEventFact(quint64 time, QString name, QJsonObject data, bool uplink)
{
    Fact *g = child(name);
    if (!g) {
        g = new Fact(this, name, "", "", Fact::Group | Fact::Count);
    }

    name.replace('.', '_');

    Fact *f = nullptr;
    if (name == "uplink") {
        auto path = data.value("path").toString();
        auto title = data.value("title").toString();
        auto value = data.value("value").toString();
        f = g->child(path);
        if (!f) {
            f = new Fact(g, path, title, path);
            //qDebug() << name << value;
            f->setValue(1);
        } else {
            f->setValue(f->value().toInt() + 1);
        }
    } else if (name == "vcp") {
        auto id = data.value("id").toString();
        f = g->childByTitle(id);
        if (!f) {
            f = new Fact(g, id, id, uplink ? "TX" : "RX");
            //qDebug() << name << value;
            f->setValue(1);
        } else {
            f->setValue(f->value().toInt() + 1);
        }
    } else if (name == telemetry::EVT_MSG.name) {
        auto msg = data.value("txt").toString();
        auto src = data.value("src").toString();
        f = new Fact(g, name + "#", msg, src);
    } else if (name == telemetry::EVT_CONF.name) {
        auto param = data.value("param").toString();
        auto value = data.value("value").toString();
        auto uid = data.value("uid").toString();
        auto title = QString("%1=%2").arg(param).arg(value);
        f = new Fact(g, name + "#", title, uid);
    } else {
        static const QHash<QString, QString> title_keys = {
            // {"info", "name_create"},
        };
        auto key = title_keys.value(name, "title");
        auto title = data.value(key).toString();
        f = new Fact(g, name + "#", title, {});
        f->setOpt("page", "Menu/FactMenuPageInfoText.qml");
        f->setOpt("info", QJsonDocument(data).toJson(QJsonDocument::Indented).constData());
    }

    if (!f)
        return;

    if (f->value().isNull())
        f->setValue(QTime(0, 0).addMSecs(time).toString("hh:mm:ss.zzz"));

    f->setProperty("time", QVariant::fromValue(time));
    connect(f, &Fact::triggered, this, [this, f]() { emit statsFactTriggered(f); });
}

void TelemetryReader::notesChanged()
{
    if (blockNotesChange)
        return;
    if (!_loadRecordID)
        return;
    QJsonObject recordInfo;
    recordInfo["notes"] = f_notes->text();
    auto req = new db::storage::TelemetryWriteRecordFields(_loadRecordID, recordInfo);
    connect(
        req,
        &db::storage::TelemetryWriteRecordFields::finished,
        this,
        [this]() { apxMsg() << tr("Notes recorded").append(':') << title(); },
        Qt::QueuedConnection);
    req->exec();
}

quint64 TelemetryReader::totalSize() const
{
    return m_totalSize;
}
void TelemetryReader::setTotalSize(quint64 v)
{
    if (m_totalSize == v)
        return;
    m_totalSize = v;
    emit totalSizeChanged();
}
quint64 TelemetryReader::totalTime() const
{
    return m_totalTime;
}
void TelemetryReader::setTotalTime(quint64 v)
{
    if (m_totalTime == v)
        return;
    m_totalTime = v;
    emit totalTimeChanged();
}
