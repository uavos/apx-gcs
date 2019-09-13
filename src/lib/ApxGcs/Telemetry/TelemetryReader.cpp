/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "TelemetryReader.h"
#include "LookupTelemetry.h"
#include <App/AppRoot.h>
#include <ApxLog.h>
#include <Database/Database.h>
#include <Database/TelemetryReqRead.h>
#include <Database/TelemetryReqWrite.h>

#include <QGeoCoordinate>
//=============================================================================
TelemetryReader::TelemetryReader(LookupTelemetry *lookup, Fact *parent)
    : Fact(parent, "reader", "", "", Group)
    , lookup(lookup)
    , blockNotesChange(false)
    , m_totalSize(0)
    , m_totalTime(0)
    , m_totalDistance(0)
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
    connect(f_reload, &Fact::triggered, this, &TelemetryReader::reloadTriggered);

    //status
    connect(lookup, &LookupTelemetry::recordInfoChanged, this, &TelemetryReader::updateRecordInfo);
    connect(this, &TelemetryReader::totalTimeChanged, this, &TelemetryReader::updateStatus);
    connect(this, &TelemetryReader::totalDistanceChanged, this, &TelemetryReader::updateStatus);

    //load sequence
    connect(lookup, &LookupTelemetry::recordTriggered, this, &TelemetryReader::load);

    connect(&loadEvent, &DelayedEvent::triggered, this, &TelemetryReader::dbLoadData);
    loadEvent.setInterval(500);

    connect(this, &Fact::triggered, lookup, [this]() {
        if (!this->lookup->recordId())
            this->lookup->f_latest->trigger();
    });

    updateRecordInfo();
}
//==============================================================================
void TelemetryReader::updateStatus()
{
    QStringList st;
    st << AppRoot::timeToString(totalTime() / 1000, true);
    if (totalDistance() > 0)
        st << AppRoot::distanceToString(totalDistance());
    setStatus(st.join(' '));
}
//==============================================================================
void TelemetryReader::updateRecordInfo()
{
    QVariantMap info = lookup->recordInfo();
    setTotalTime(info.value("totalTime").toULongLong());
    quint64 downlink = info.value("downlink").toULongLong();
    quint64 uplink = info.value("uplink").toULongLong();
    quint64 events = info.value("events").toULongLong();
    setTotalSize(downlink + uplink + events);
    evtCountMap.clear();
    foreach (QString s, info.value("evtDetails").toString().split(',')) {
        if (s.isEmpty())
            continue;
        QString key = s.left(s.indexOf('='));
        QString v = s.mid(s.indexOf('=') + 1);
        evtCountMap.insert(key, v.toUInt());
    }
    if (uplink)
        evtCountMap.insert("uplink", uplink);

    qint64 t = info.value("time").toLongLong();
    QString title = t > 0 ? QDateTime::fromMSecsSinceEpoch(t).toString("yyyy MMM dd hh:mm:ss")
                          : tr("Current record");
    QString callsign = info.value("callsign").toString();
    QString descr = info.value("comment").toString();
    QString notes = info.value("notes").toString();
    QString stime = AppRoot::timeToString(totalTime() / 1000, true);

    if (!callsign.isEmpty())
        title.append(QString(" | %1").arg(callsign));
    setTitle(title);
    setDescr(descr);
    blockNotesChange = true;
    f_notes->setValue(notes);
    blockNotesChange = false;

    emit statsAvailable();
}
//==============================================================================
void TelemetryReader::load()
{
    f_reload->setEnabled(false);
    quint64 key = lookup->recordId();
    if (!key)
        return;
    setTotalSize(0);
    setTotalTime(0);
    setTotalDistance(0);
    setProgress(0);
    removeAll();
    DBReqTelemetryFindCache *req = new DBReqTelemetryFindCache(key);
    connect(req,
            &DBReqTelemetryFindCache::cacheFound,
            this,
            &TelemetryReader::dbCacheFound,
            Qt::QueuedConnection);
    connect(req,
            &DBReqTelemetryFindCache::cacheNotFound,
            this,
            &TelemetryReader::dbCacheNotFound,
            Qt::QueuedConnection);
    req->exec();
}
//==============================================================================
void TelemetryReader::reloadTriggered()
{
    quint64 key = lookup->recordId();
    if (!key)
        return;
    Database::instance()->telemetry->markCacheInvalid(key);
    load();
}
//==============================================================================
//==============================================================================
void TelemetryReader::dbCacheNotFound(quint64 telemetryID)
{
    if (telemetryID != lookup->recordId())
        return;
    loadEvent.schedule();
}
void TelemetryReader::dbCacheFound(quint64 telemetryID)
{
    if (telemetryID != lookup->recordId())
        return;
    dbLoadData();
}
void TelemetryReader::dbLoadData()
{
    quint64 telemetryID = lookup->recordId();
    if (!telemetryID)
        return;
    setProgress(0);
    //request data
    {
        DBReqTelemetryMakeStats *req = new DBReqTelemetryMakeStats(telemetryID);
        connect(lookup,
                &LookupTelemetry::discardRequests,
                req,
                &DatabaseRequest::discard,
                Qt::QueuedConnection);
        connect(req,
                &DBReqTelemetryMakeStats::statsUpdated,
                this,
                &TelemetryReader::dbStatsUpdated,
                Qt::QueuedConnection);
        connect(req,
                &DBReqTelemetryMakeStats::statsFound,
                this,
                &TelemetryReader::dbStatsFound,
                Qt::QueuedConnection);
        req->exec();
    }
}
//=============================================================================
//=============================================================================
void TelemetryReader::dbStatsFound(quint64 telemetryID, QVariantMap stats)
{
    Q_UNUSED(stats)
    if (telemetryID != lookup->recordId())
        return;
    DBReqTelemetryReadData *req = new DBReqTelemetryReadData(telemetryID);
    connect(lookup,
            &LookupTelemetry::discardRequests,
            req,
            &DatabaseRequest::discard,
            Qt::QueuedConnection);
    connect(req,
            &DBReqTelemetryReadData::dataLoaded,
            this,
            &TelemetryReader::dbResultsData,
            Qt::QueuedConnection);
    connect(req,
            &DBReqTelemetryReadData::progress,
            this,
            &TelemetryReader::dbProgress,
            Qt::QueuedConnection);
    connect(req, &DatabaseRequest::finished, this, [this](DatabaseRequest::Status) {
        setProgress(-1);
    });
    req->exec();
}
void TelemetryReader::dbStatsUpdated(quint64 telemetryID, QVariantMap stats)
{
    if (telemetryID != lookup->recordId())
        return;
    QVariantMap info = lookup->recordInfo();
    foreach (QString key, stats.keys()) {
        info[key] = stats.value(key);
    }
    lookup->setRecordInfo(info);
    updateRecordInfo();
    dbStatsFound(telemetryID, stats);
}
void TelemetryReader::dbResultsData(quint64 telemetryID,
                                    quint64 cacheID,
                                    DatabaseRequest::Records records,
                                    QMap<quint64, QString> fieldNames)
{
    if (telemetryID != lookup->recordId())
        return;
    int iTime = records.names.indexOf("time");
    int iType = records.names.indexOf("type");
    int iName = records.names.indexOf("name");
    int iValue = records.names.indexOf("value");
    int iUid = records.names.indexOf("uid");

    QGeoPath path;
    quint64 fidLat = fieldNames.key("gps_lat");
    quint64 fidLon = fieldNames.key("gps_lon");
    quint64 fidHmsl = fieldNames.key("gps_hmsl");
    QVector<QPointF> *vLat = nullptr;
    QVector<QPointF> *vLon = nullptr;
    QVector<QPointF> *vHmsl = nullptr;

    removeAll();
    f_reload->setEnabled(true);

    foreach (QVector<QPointF> *d, fieldData.values()) {
        delete d;
    }
    fieldData.clear();
    fieldData.reserve(1000);
    times.clear();
    times.reserve(1000000);
    events.clear();
    this->fieldNames.clear();

    times.append(0);
    //quint64 timestamp=lookup->recordTimestamp();
    quint64 totalTime = this->totalTime();
    qreal totalDistance = 0;
    quint64 t0 = 0;
    QHash<quint64, double> fvalues;

    for (int i = 0; i < records.values.size(); ++i) {
        const QVariantList &r = records.values.at(i);
        if (r.isEmpty())
            continue;

        //progress and abort
        int vp = i * 100 / records.values.size();
        if (progress() != vp) {
            setProgress(vp);
        }

        //time
        quint64 t = r.at(iTime).toULongLong();
        if (i == 0)
            t0 = t;
        t -= t0;
        if (totalTime < t)
            totalTime = t;
        double tf = t / 1000.0;
        if (times.last() != tf)
            times.append(tf);

        quint64 fid = 0;

        switch (r.at(iType).toUInt()) {
        case 0: {
            //downlink data
            fid = r.at(iName).toULongLong();
        } break;
        case 1: {
            //uplink data
            fid = r.at(iName).toULongLong();
            events.insertMulti(tf, QString('>').append(fieldNames.value(fid, "uplink")));
            addEventFact(t,
                         "uplink",
                         fieldNames.value(fid, QString::number(fid)),
                         r.at(iUid).toString());
        } break;
        case 2:
        case 3: {
            //events
            events.insertMulti(tf, r.at(iName).toString());
            addEventFact(t, r.at(iName).toString(), r.at(iValue).toString(), r.at(iUid).toString());
        } break;
        }

        if (!fid)
            continue;
        QVector<QPointF> *pts = fieldData.value(fid);
        if (!pts) {
            pts = new QVector<QPointF>;
            pts->reserve(1000000);
            fieldData.insert(fid, pts);
        }
        double v = r.at(iValue).toDouble();
        if (fvalues.contains(fid) && fvalues.value(fid) == v)
            continue;
        fvalues[fid] = v;

        if (pts->size() > 0 && (tf - pts->last().x()) > 0.5) {
            //extrapolate unchanged value tail-1ms
            pts->append(QPointF(tf, pts->last().y()));
        }
        pts->append(QPointF(tf, v));

        //path update
        while (fid && (fid == fidLat || fid == fidLon || fid == fidHmsl)) {
            QGeoCoordinate c;

            if (!vLat)
                vLat = fieldData.value(fidLat);
            if (!vLon)
                vLon = fieldData.value(fidLon);
            if (!vHmsl)
                vHmsl = fieldData.value(fidHmsl);

            if (!(vLat && vLon && vHmsl))
                break;

            c.setLatitude(vLat->last().y());
            c.setLongitude(vLon->last().y());
            c.setAltitude(vHmsl->last().y());

            if (!c.isValid())
                break;
            if (c.latitude() == 0.0)
                break;
            if (c.longitude() == 0.0)
                break;
            if (!path.isEmpty()) {
                QGeoCoordinate c0(path.path().last());
                if (c0.latitude() == c.latitude())
                    break;
                if (c0.longitude() == c.longitude())
                    break;
                if (c0.distanceTo(c) < 10.0)
                    break;
                totalDistance += c0.distanceTo(c);
            }

            path.addCoordinate(c);
            break;
        }
    }

    //final data tail at max time
    double tMax = totalTime / 1000.0;
    for (int i = 0; i < fieldData.values().size(); ++i) {
        QVector<QPointF> *pts = fieldData.values().at(i);
        if (!pts)
            continue;
        if (pts->isEmpty()) {
            pts->append(QPointF(0, 0));
            //continue;
        }
        if (pts->last().x() >= tMax)
            continue;
        pts->append(QPointF(tMax, pts->last().y()));
    }
    setTotalTime(totalTime);
    setTotalDistance(qRound(totalDistance));

    this->fieldNames = fieldNames;

    this->geoPath = path;

    setProgress(-1);
    emit dataAvailable(cacheID);
}
void TelemetryReader::dbProgress(quint64 telemetryID, int v)
{
    if (telemetryID != lookup->recordId())
        return;
    setProgress(v);
}
//=============================================================================
void TelemetryReader::addEventFact(quint64 time,
                                   const QString &name,
                                   const QString &value,
                                   const QString &uid)
{
    //qDebug() << name << value;
    QString stime = QTime(0, 0).addMSecs(time).toString("hh:mm:ss.zzz");
    Fact *g = child(name);
    if (!g)
        g = new Fact(this, name, "", "", Group | Const);

    Fact *f = nullptr;
    if (name == "uplink") {
        f = g->child(value);
        if (!f) {
            f = new Fact(g, value, "", "", Const);
            //qDebug() << name << value;
            f->setValue(1);
        } else {
            f->setValue(f->value().toInt() + 1);
        }
    } else if (name == "serial") {
        f = g->child(uid);
        if (!f) {
            f = new Fact(g, uid, "", "", Const);
            //qDebug() << name << value;
            f->setValue(1);
        } else {
            f->setValue(f->value().toInt() + 1);
        }
    } else {
        QString title = value;
        QString descr = uid;
        if (title.startsWith('[') && title.contains(']')) {
            int i = title.indexOf(']');
            QString s = title.mid(1, i - 1).trimmed();
            title.remove(0, i + 1);
            if (!s.isEmpty())
                descr.prepend(QString("%1/").arg(s));
        }
        f = new Fact(g, name + "#", title, descr);
        f->setStatus(stime);
        connect(f, &Fact::triggered, this, [this, f]() { emit recordFactTriggered(f); });
    }

    if (!f)
        return;
    f->userData = time;
}
//=============================================================================
//=============================================================================
//=============================================================================
void TelemetryReader::notesChanged()
{
    if (blockNotesChange)
        return;
    if (!lookup->recordId())
        return;
    QVariantMap info;
    info.insert("notes", f_notes->text());
    DBReqTelemetryWriteInfo *req = new DBReqTelemetryWriteInfo(lookup->recordId(), info);
    connect(
        req,
        &DBReqTelemetryWriteInfo::finished,
        this,
        [=]() { apxMsg() << tr("Notes recorded").append(':') << title(); },
        Qt::QueuedConnection);
    req->exec();
}
//=============================================================================
//=============================================================================
//=============================================================================
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
quint64 TelemetryReader::totalDistance() const
{
    return m_totalDistance;
}
void TelemetryReader::setTotalDistance(quint64 v)
{
    if (m_totalDistance == v)
        return;
    m_totalDistance = v;
    emit totalDistanceChanged();
}
//=============================================================================
//=============================================================================
