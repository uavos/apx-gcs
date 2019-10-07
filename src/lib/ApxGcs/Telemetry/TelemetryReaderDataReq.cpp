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
#include "TelemetryReaderDataReq.h"
#include <Fact/Fact.h>
//=============================================================================
bool TelemetryReaderDataReq::run(QSqlQuery &query)
{
    if (!DBReqTelemetryReadData::run(query))
        return false;
    if (discarded())
        return true;

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

    fieldData_t fieldData;
    times_t times;
    events_t events;

    times.append(0);

    quint64 t0 = 0;
    QHash<quint64, double> fvalues;
    int progress_s = 0;

    f_events = new Fact(nullptr,
                        "events",
                        tr("Events"),
                        tr("Recorded events data"),
                        Fact::Group | Fact::Const);
    f_events->moveToThread(nullptr);

    for (int i = 0; i < records.values.size(); ++i) {
        if (discarded())
            return true;

        const QVariantList &r = records.values.at(i);
        if (r.isEmpty())
            continue;

        //progress and abort
        int vp = i * 100 / records.values.size();
        if (progress_s != vp) {
            progress_s = vp;
            emit progress(telemetryID, vp);
        }

        //time
        quint64 t = r.at(iTime).toULongLong();
        if (i == 0)
            t0 = t;
        t -= t0;
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
            event_t e;
            e.time = t;
            e.name = "uplink";
            e.value = fieldNames.value(fid, QString::number(fid));
            e.uid = r.at(iUid).toString();
            events.append(e);
            addEventFact(e.time, e.name, e.value, e.uid);
        } break;
        case 2:
        case 3: {
            //events
            event_t e;
            e.time = t;
            e.name = r.at(iName).toString();
            e.value = r.at(iValue).toString();
            e.uid = r.at(iUid).toString();
            events.append(e);
            addEventFact(e.time, e.name, e.value, e.uid);
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
            if (!vLat)
                vLat = fieldData.value(fidLat);
            if (!vLon)
                vLon = fieldData.value(fidLon);
            if (!vHmsl)
                vHmsl = fieldData.value(fidHmsl);

            if (!(vLat && vLon && vHmsl))
                break;

            QGeoCoordinate c(vLat->last().y(), vLon->last().y(), vHmsl->last().y());

            if (!c.isValid())
                break;
            if (c.latitude() == 0.0)
                break;
            if (c.longitude() == 0.0)
                break;
            qreal dist = 0;
            if (!path.isEmpty()) {
                QGeoCoordinate c0(path.path().last());
                if (c0.latitude() == c.latitude())
                    break;
                if (c0.longitude() == c.longitude())
                    break;
                dist = c0.distanceTo(c);
                if (dist < 10.0)
                    break;
            }
            path.addCoordinate(c);
            break;
        }
    }

    //final data tail at max time
    qreal tMax = times.isEmpty() ? 0 : times.last();
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

    if (discarded())
        return true;

    emit dataProcessed(telemetryID, cacheID, fieldData, fieldNames, times, events, path, f_events);

    return true;
}
//=============================================================================
void TelemetryReaderDataReq::addEventFact(quint64 time,
                                          const QString &name,
                                          const QString &value,
                                          const QString &uid)
{
    Fact *g = f_events->child(name);
    if (!g)
        g = new Fact(f_events, name, "", "", Fact::Group | Fact::Const);

    Fact *f = nullptr;
    if (name == "uplink") {
        f = g->child(value);
        if (!f) {
            f = new Fact(g, value, "", "", Fact::Const);
            //qDebug() << name << value;
            f->setValue(1);
        } else {
            f->setValue(f->value().toInt() + 1);
        }
    } else if (name == "serial") {
        f = g->child(uid);
        if (!f) {
            f = new Fact(g, uid, "", "", Fact::Const);
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
        f->setStatus(stime);
    }

    if (!f)
        return;
    f->userData = time;
}
//=============================================================================
