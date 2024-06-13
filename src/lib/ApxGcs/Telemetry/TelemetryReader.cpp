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
#include "TelemetryDBReq.h"
#include "TelemetryRecords.h"

#include <App/App.h>
#include <App/AppLog.h>
#include <App/AppRoot.h>

#include <Database/Database.h>
#include <Database/TelemetryReqRead.h>
#include <Database/TelemetryReqWrite.h>

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
    deleteChildren();
    setProgress(0);

    _loadRecordID = id;
    _fields.clear();
    _geoPath = {};
    _totalDistance = 0;
    _fidx_lat = -1;
    _fidx_lon = -1;
    _fidx_hmsl = -1;
    _geoPos = {};

    auto req = new DBReqTelemetryLoadFile(id);
    connect(req, &DatabaseRequest::finished, this, [this]() {
        setProgress(-1);
        emit rec_finished();
        f_reload->setEnabled(true);
        emit geoPathCollected(_geoPath, _totalDistance);
        _geoPath = {};
    });
    auto reader = req->reader();
    connect(reader, &TelemetryFileReader::progressChanged, this, [this](int v) { setProgress(v); });
    connect(req, &DBReqTelemetryLoadFile::recordInfo, this, &TelemetryReader::setRecordInfo);

    // forward info to other facts (lists)
    connect(req, &DBReqTelemetryRecordInfo::recordInfo, this, &TelemetryReader::recordInfoUpdated);
    connect(reader, &TelemetryFileReader::field, this, &TelemetryReader::rec_field);
    connect(reader, &TelemetryFileReader::values, this, &TelemetryReader::rec_values);
    connect(reader, &TelemetryFileReader::evt, this, &TelemetryReader::rec_evt);
    connect(reader, &TelemetryFileReader::msg, this, &TelemetryReader::rec_msg);
    connect(reader, &TelemetryFileReader::meta, this, &TelemetryReader::rec_meta);
    connect(reader, &TelemetryFileReader::raw, this, &TelemetryReader::rec_raw);

    // processed by reader
    connect(reader, &TelemetryFileReader::field, this, &TelemetryReader::do_rec_field);
    connect(reader, &TelemetryFileReader::values, this, &TelemetryReader::do_rec_values);
    connect(reader, &TelemetryFileReader::evt, this, &TelemetryReader::do_rec_evt);

    // start parsing
    emit rec_started();
    req->exec();
}

void TelemetryReader::do_rec_field(QString name, QString title, QString units)
{
    _fields.append({name, title, units});

    if (name == "est.pos.lat")
        _fidx_lat = _fields.size() - 1;
    else if (name == "est.pos.lon")
        _fidx_lon = _fields.size() - 1;
    else if (name == "est.pos.hmsl")
        _fidx_hmsl = _fields.size() - 1;
}
void TelemetryReader::do_rec_values(quint64 timestamp_ms, Values data, bool uplink)
{
    if (uplink) {
        for (auto [idx, value] : data.asKeyValueRange()) {
            const auto &f = _fields.value(idx);
            addEventFact(timestamp_ms, "uplink", f.title, f.name);
        }
        return;
    }

    // collect flight path
    for (auto [idx, value] : data.asKeyValueRange()) {
        if (idx == _fidx_lat)
            _geoPos.setLatitude(value.toDouble());
        else if (idx == _fidx_lon)
            _geoPos.setLongitude(value.toDouble());
        else if (idx == _fidx_hmsl)
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
void TelemetryReader::do_rec_evt(
    quint64 timestamp_ms, QString name, QString value, QString uid, bool uplink)
{
    addEventFact(timestamp_ms, name, value, uid);
}

void TelemetryReader::setRecordInfo(quint64 id, QJsonObject info)
{
    _info = info;
    const auto &m = info;

    setTotalTime(m["duration"].toInteger());

    const auto &cnt = m["info"]["counters"].toObject();

    setTotalSize(cnt["records"].toInteger());

    quint64 downlink = cnt["downlink"].toInteger();
    quint64 uplink = cnt["uplink"].toInteger();
    quint64 events = cnt["evt"].toInteger();

    qint64 t = m["time"].toInteger();
    QString title = t > 0 ? QDateTime::fromMSecsSinceEpoch(t).toString("yyyy MMM dd hh:mm:ss")
                          : tr("Telemetry Data");
    QString callsign = m["callsign"].toString();
    QString comment = m["comment"].toString();
    QString notes = m["notes"].toString();
    QString stime = AppRoot::timeToString(totalTime() / 1000, true);

    QStringList descr;
    if (!callsign.isEmpty())
        descr.append(callsign);
    if (!comment.isEmpty() && comment != callsign)
        descr.append(comment);

    descr.append(QString("%1/%2/%3").arg(downlink).arg(uplink).arg(events));

    setTitle(title);
    setDescr(descr.join(" | "));

    blockNotesChange = true;
    f_notes->setValue(notes);
    blockNotesChange = false;

    // qDebug("%s", QJsonDocument(info).toJson(QJsonDocument::Indented).constData());

    // create info fact with text
    auto f = new Fact(this, "info", tr("Info"), "", Group);
    f->move(0);
    f->setIcon("information");
    f->setOpt("page", "Menu/FactMenuPageInfoText.qml");
    f->setText(QJsonDocument(info).toJson(QJsonDocument::Indented).constData());

    emit recordInfoChanged();
}

void TelemetryReader::addEventFact(quint64 time,
                                   const QString &name,
                                   const QString &value,
                                   const QString &uid)
{
    Fact *g = child(name);
    if (!g) {
        g = new Fact(this, name, "", "", Fact::Group | Fact::Count);
    }

    Fact *f = nullptr;
    if (name == "uplink") {
        f = g->child(uid);
        if (!f) {
            f = new Fact(g, uid, uid, value);
            //qDebug() << name << value;
            f->setValue(1);
        } else {
            f->setValue(f->value().toInt() + 1);
        }
    } else if (name == "serial") {
        f = g->childByTitle(uid);
        if (!f) {
            f = new Fact(g, uid, uid, "");
            //qDebug() << name << value;
            f->setValue(1);
        } else {
            f->setValue(f->value().toInt() + 1);
        }
    } else {
        QString title;
        if (name == "xpdr")
            title = name;
        else
            title = value;
        QString descr = uid;
        if (title.startsWith('[') && title.contains(']')) {
            int i = title.indexOf(']');
            QString s = title.mid(1, i - 1).trimmed();
            title.remove(0, i + 1);
            if (!s.isEmpty())
                descr.prepend(QString("%1/").arg(s));
        }
        f = new Fact(g, name + "#", title, descr);
        QString stime = QTime(0, 0).addMSecs(time).toString("hh:mm:ss.zzz");
        f->setValue(stime);
    }

    if (!f)
        return;
    f->setProperty("time", QVariant::fromValue(time));
    connect(f, &Fact::triggered, this, [this, f]() { emit statsFactTriggered(f); });
}

void TelemetryReader::notesChanged()
{
    /*if (blockNotesChange)
        return;
    if (!records->recordId())
        return;
    QVariantMap info;
    info.insert("notes", f_notes->text());
    DBReqTelemetryWriteInfo *req = new DBReqTelemetryWriteInfo(records->recordId(), info);
    connect(
        req,
        &DBReqTelemetryWriteInfo::finished,
        this,
        [this]() { apxMsg() << tr("Notes recorded").append(':') << title(); },
        Qt::QueuedConnection);
    req->exec();*/
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
