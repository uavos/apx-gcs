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

#include <Database/NodesReqUnit.h>
#include <Database/StorageReq.h>

#include <QGeoCoordinate>

#include <Fleet/Fleet.h>
#include <Mandala/MandalaAliases.h>

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
    if (_loadFileReq) {
        _loadFileReq->discard();

        _loadFileReq->disconnect();
        _loadFileReq->reader()->disconnect();

        _loadFileReq = nullptr;
    }
    QTimer::singleShot(0, this, [this, id]() { do_loadRecord(id); });
}

void TelemetryReader::do_loadRecord(quint64 id)
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
    _jsoData.clear();

    auto req = new db::storage::TelemetryLoadFile(id);
    auto reader = req->reader();
    connect(reader, &TelemetryFileReader::progressChanged, this, [this](int v) { setProgress(v); });
    connect(req, &db::storage::TelemetryLoadFile::finished, this, &TelemetryReader::do_rec_finished);
    connect(req, &db::storage::TelemetryLoadFile::recordInfo, this, &TelemetryReader::setRecordInfo);
    connect(req, &db::storage::TelemetryLoadFile::fileOpened, this, [this](QString path) {
        _recordFilePath = path;
    });

    // forward info to other facts (lists)
    connect(req,
            &db::storage::TelemetryLoadInfo::recordInfo,
            this,
            &TelemetryReader::recordInfoUpdated);

    // forward as-is
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
    _loadFileReq = req;
    emit rec_started();
    req->exec();
}

MandalaFact *TelemetryReader::fieldFact(const Field &field)
{
    auto it = mandala::ALIAS_MAP.find(field.name.toStdString());
    if (it != mandala::ALIAS_MAP.end()) {
        auto uid = it->second;
        return Fleet::replay()->f_mandala->fact(uid);
    }
    return Fleet::replay()->f_mandala->fact(field.name, true);
}

void TelemetryReader::do_rec_field(Field field)
{
    auto f = fieldFact(field);
    if (f) {
        // update field to match latest mandala info
        auto name = f->mpath();
        auto title = f->title();
        auto units = f->units();
        if (name != field.name)
            title = QString("%1 (%2)").arg(title, field.name);
        field = {name, {title, units}};
    }

    _fields.append(field);
    auto name = field.name;
    auto index = _fields.size() - 1;

    if (name == "est.pos.lat")
        _index_lat = index;
    else if (name == "est.pos.lon")
        _index_lon = index;
    else if (name == "est.pos.hmsl")
        _index_hmsl = index;

    emit rec_field(field);
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

    // save meta objects to databases
    if (_importedMeta.contains(_loadRecordID))
        return;

    if (name == "nodes") {
        auto req = new db::nodes::UnitImportConf(data);
        req->exec();
        // qDebug("%s", QJsonDocument(data).toJson(QJsonDocument::Indented).constData());
    }
}

void TelemetryReader::do_rec_finished()
{
    emit rec_finished();
    emit geoPathCollected(_geoPath, _totalDistance);
    _geoPath = {};
    f_reload->setEnabled(true);
    setProgress(-1);

    _loadFileReq = nullptr;
}

void TelemetryReader::setRecordInfo(quint64 id, QJsonObject info, QString notes)
{
    _recordInfo = info;
    const auto &m = info;

    setTotalTime(m["duration"].toVariant().toULongLong());

    const auto &cnt = m["counters"].toObject();

    setTotalSize(cnt["records"].toVariant().toULongLong());

    quint64 downlink = cnt["downlink"].toVariant().toULongLong();
    quint64 uplink = cnt["uplink"].toVariant().toULongLong();
    quint64 events = cnt["evt"].toVariant().toULongLong();

    qint64 t = m["timestamp"].toVariant().toULongLong();
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
        name = QString(path).replace('.', '_');
        f = g->child(name);
        if (!f) {
            f = new Fact(g, name, title, path);
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
        auto uid = data.value("uid").toString();
        auto descr = uid.isEmpty() ? src : (src + " | " + uid);
        f = new Fact(g, name + "#", msg, descr);
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
        // loadable data vs shown as page
        if (name == "nodes" || name == "mission") {
            auto zip = qCompress(QJsonDocument(data).toJson(QJsonDocument::Compact), 9);
            _jsoData.insert(f, zip);
        } else {
            f->setOpt("page", "Menu/FactMenuPageInfoText.qml");
            f->setOpt("info", QJsonDocument(data).toJson(QJsonDocument::Indented).constData());
        }
    }

    if (!f)
        return;

    f->setProperty("time", QVariant::fromValue(time));

    if (f->opts().contains("page"))
        return;

    if (f->value().isNull())
        f->setValue(QTime(0, 0).addMSecs(time).toString("hh:mm:ss.zzz"));

    connect(f, &Fact::triggered, this, [this, f]() {
        emit statsFactTriggered(f, QJsonDocument::fromJson(qUncompress(_jsoData.value(f))).object());
    });
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
