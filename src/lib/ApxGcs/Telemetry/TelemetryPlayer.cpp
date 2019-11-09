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
#include "TelemetryPlayer.h"
#include "LookupTelemetry.h"
#include "Telemetry.h"
#include "TelemetryReader.h"

#include <App/App.h>
#include <App/AppLog.h>
#include <Database/Database.h>
#include <Database/TelemetryReqRead.h>

#include <Mission/MissionStorage.h>
#include <Mission/VehicleMission.h>
#include <Nodes/Nodes.h>
//=============================================================================
TelemetryPlayer::TelemetryPlayer(Telemetry *telemetry, Fact *parent)
    : Fact(parent, "player", tr("Player"), tr("Telemetry data player"), Group)
    , telemetry(telemetry)
    , vehicle(telemetry->vehicle)
    , cacheID(0)
    , setTime0(0)
    , blockTimeChange(false)
{
    setIcon("play-circle-outline");

    connect(telemetry->f_reader,
            &TelemetryReader::dataAvailable,
            this,
            &TelemetryPlayer::setCacheId);
    connect(this, &Fact::activeChanged, this, &TelemetryPlayer::updateActive);

    f_time = new Fact(this, "time", tr("Time"), tr("Current postition"), Int);
    f_time->setMin((quint64) 0);
    f_time->setUnits("ms");
    connect(f_time, &Fact::valueChanged, this, &TelemetryPlayer::updateTime);
    connect(f_time, &Fact::valueChanged, this, &TelemetryPlayer::updateStatus);

    f_speed = new Fact(this, "speed", tr("Speed"), tr("Playback speed"), Float);
    f_speed->setMin(0.1);
    f_speed->setMax(10);
    f_speed->setPrecision(1);
    f_speed->setValue(1);
    connect(f_speed, &Fact::valueChanged, this, &TelemetryPlayer::updateSpeed);

    //actions
    f_play = new Fact(this, "play", tr("Play"), tr("Play stream"), Action | Apply, "play");
    connect(f_play, &Fact::triggered, this, &TelemetryPlayer::play);

    f_stop = new Fact(this, "stop", tr("Stop"), tr("Pause playing"), Action | Stop, "stop");
    connect(f_stop, &Fact::triggered, this, &TelemetryPlayer::stop);

    f_rewind = new Fact(this, "rewind", tr("Rewind"), tr("Jump back"), Action, "rewind");
    connect(f_rewind, &Fact::triggered, this, &TelemetryPlayer::rewind);

    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, &TelemetryPlayer::next);

    connect(telemetry->f_lookup, &LookupTelemetry::recordIdChanged, this, &TelemetryPlayer::reset);
    connect(telemetry->f_reader, &TelemetryReader::totalTimeChanged, this, [=]() {
        f_time->setMax(telemetry->f_reader->totalTime());
    });

    connect(this, &Fact::activeChanged, this, &TelemetryPlayer::updateActions);

    connect(Vehicles::instance(), &Vehicles::vehicleSelected, this, [=](Vehicle *v) {
        if (v != vehicle)
            stop();
    });
    updateSpeed();
    updateStatus();
    updateActions();
}
//==============================================================================
void TelemetryPlayer::updateActions()
{
    bool enb = cacheID;
    bool playing = active();
    f_play->setEnabled(enb && (!playing));
    f_stop->setEnabled(enb && (playing));
    f_rewind->setEnabled(enb);
}
//==============================================================================
void TelemetryPlayer::updateStatus()
{
    setStatus(AppRoot::timeToString(f_time->value().toULongLong() / 1000, true));
}
//==============================================================================
void TelemetryPlayer::reset()
{
    stop();
    f_time->setValue(0);
    f_speed->setValue(1.0);
    cacheID = 0;
    updateActions();
}
void TelemetryPlayer::setCacheId(quint64 v)
{
    if (cacheID)
        reset();
    cacheID = v;
    updateActions();
}
//=============================================================================
void TelemetryPlayer::updateSpeed()
{
    _speed = f_speed->value().toDouble();
    if (active()) {
        playTime0 = _time;
        playTime.start();
        next();
    }
}
void TelemetryPlayer::updateActive()
{
    if ((!active())) {
        emit discardRequests();
    }
}
void TelemetryPlayer::updateTime()
{
    if (blockTimeChange)
        return;
    setTime0 = _time = f_time->value().toULongLong();
    if (active()) {
        stop();
        play();
    }
}
//=============================================================================
void TelemetryPlayer::play()
{
    if (active())
        return;
    if (!telemetry->f_lookup->recordId())
        return;
    vehicle->f_select->trigger();
    setActive(true);
    playTime0 = _time;
    playTime.start();

    //fill facts map
    if (factsMap.isEmpty()) {
        TelemetryDB::TelemetryFieldsMap map = Database::instance()->telemetry->fieldsMap();
        foreach (quint64 key, map.keys()) {
            Fact *f = vehicle->f_mandala->child(map.value(key));
            if (f)
                factsMap.insert(key, f);
        }
    }

    //collect data samples for t0
    double t = _time / 1000.0;
    foreach (quint64 fieldID, telemetry->f_reader->fieldData.keys()) {
        Fact *f = factsMap.value(fieldID);
        if (!f)
            continue;
        f->setValue(sampleValue(fieldID, t));
    }
    tNext = _time;
    dbRequestEvents(tNext);
}
void TelemetryPlayer::stop()
{
    setActive(false);
    emit discardRequests();
    events.clear();
}
void TelemetryPlayer::rewind()
{
    bool bPlaying = active();
    stop();
    if (_time == setTime0)
        f_time->setValue(0);
    else {
        f_time->setValue(setTime0);
        if (bPlaying)
            play();
    }
}
//==============================================================================
//==============================================================================
void TelemetryPlayer::dbRequestEvents(quint64 t)
{
    {
        DBReqTelemetryReadEvent *req = new DBReqTelemetryReadEvent(cacheID, t, "mission");
        connect(this, &TelemetryPlayer::discardRequests, req, &DatabaseRequest::discard);
        connect(req,
                &DBReqTelemetryReadEvent::eventLoaded,
                this,
                &TelemetryPlayer::missionDataLoaded,
                Qt::QueuedConnection);
        req->exec();
    }
    {
        DBReqTelemetryReadEvent *req = new DBReqTelemetryReadEvent(cacheID, t, "nodes");
        connect(this, &TelemetryPlayer::discardRequests, req, &DatabaseRequest::discard);
        connect(req,
                &DBReqTelemetryReadEvent::eventLoaded,
                this,
                &TelemetryPlayer::nodesDataLoaded,
                Qt::QueuedConnection);
        req->exec();
    }
    {
        DBReqTelemetryReadConfData *req = new DBReqTelemetryReadConfData(cacheID, t);
        connect(this, &TelemetryPlayer::discardRequests, req, &DatabaseRequest::discard);
        connect(req,
                &DBReqTelemetryReadConfData::confLoaded,
                this,
                &TelemetryPlayer::nodesConfUpdatesLoaded,
                Qt::QueuedConnection);
        req->exec();
    }
    {
        //must be the last as will begin after its finish
        DBReqTelemetryReadEvents *req = new DBReqTelemetryReadEvents(cacheID, t);
        connect(this, &TelemetryPlayer::discardRequests, req, &DatabaseRequest::discard);
        connect(req,
                &DBReqTelemetryReadEvents::eventsLoaded,
                this,
                &TelemetryPlayer::eventsLoaded,
                Qt::QueuedConnection);
        req->exec();
    }
}
void TelemetryPlayer::eventsLoaded(DatabaseRequest::Records records)
{
    if (!active())
        return;
    events = records;
    iEventRec = 0;
    playTime.start();
    next();
}
void TelemetryPlayer::missionDataLoaded(QString value, QString uid, bool uplink)
{
    Q_UNUSED(value)
    Q_UNUSED(uplink)
    vehicle->f_mission->storage->loadMission(uid);
}
void TelemetryPlayer::nodesDataLoaded(QString value, QString uid, bool uplink)
{
    Q_UNUSED(value)
    Q_UNUSED(uplink)
    vehicle->f_nodes->storage->loadConfiguration(uid);
}
void TelemetryPlayer::nodesConfUpdatesLoaded(DatabaseRequest::Records records)
{
    if (!active())
        return;
    const QStringList &n = records.names;
    int iUid = n.indexOf("uid");
    int iValue = n.indexOf("value");
    for (int i = 0; i < records.values.size(); ++i) {
        const QVariantList &r = records.values.at(i);
        vehicle->f_nodes->loadConfValue(r.at(iUid).toByteArray(), r.at(iValue).toString());
    }
}
//==============================================================================
void TelemetryPlayer::next()
{
    if (!active())
        return;
    quint64 t = playTime.elapsed();
    if (_speed > 0 && _speed != 1.0)
        t = t * _speed;
    t += playTime0;

    uint updCnt = 0;
    if (tNext <= t) {
        quint64 tNextMin = tNext;
        //mandala data
        foreach (quint64 fieldID, telemetry->f_reader->fieldData.keys()) {
            QVector<QPointF> *pts = telemetry->f_reader->fieldData.value(fieldID);
            if (!pts)
                continue;
            for (int i = dataPosMap.value(fieldID); i < pts->size(); ++i) {
                const QPointF &p = pts->at(i);
                quint64 tP = p.x() * 1000.0;
                if (tP > tNext) {
                    if (tNextMin == tNext || tNextMin > tP)
                        tNextMin = tP;
                    break;
                }
                dataPosMap[fieldID] = i;
                Fact *f = factsMap.value(fieldID);
                if (f) {
                    if (f->setValue(p.y()))
                        updCnt++;
                }
            }
        }
        //events
        const QStringList &n = events.names;
        for (; iEventRec < events.values.size(); ++iEventRec) {
            const QVariantList &r = events.values.at(iEventRec);
            quint64 tP = r.at(n.indexOf("time")).toULongLong();
            if (tP > t) {
                if (tNextMin == tNext || tNextMin > tP)
                    tNextMin = tP;
                break;
            }
            //show event
            QString sv = r.at(n.indexOf("value")).toString();
            uint type = r.at(n.indexOf("type")).toUInt();
            if (type == 1) {
                //uplink data
                quint64 fieldID = r.at(n.indexOf("name")).toULongLong();
                if (!fieldID)
                    continue;
                Fact *f = factsMap.value(fieldID);
                if (!f)
                    continue;
                if (f->setValue(sv))
                    updCnt++;
                QString s = f->title();
                if (!(s.startsWith("rc_") || s.startsWith("camcmd_"))) {
                    vehicle->message(QString("%1: %2 = %3").arg(">").arg(s).arg(f->text()),
                                     AppNotify::Important);
                    //apxMsg() << QString("[replay]%1: %2 = %3").arg(">").arg(s).arg(f->text());
                }
            } else if (type == 2 || type == 3) {
                const QString &evt = r.at(n.indexOf("name")).toString();
                const QString &uid = r.at(n.indexOf("uid")).toString();
                if (evt == "msg") {
                    QString s = sv;
                    QString sub;
                    if (s.startsWith('[')) {
                        s.remove(0, 1);
                        sub = s.left(s.indexOf(']'));
                        s.remove(0, sub.size() + 1);
                    }
                    vehicle->message(QString("<: %1").arg(s),
                                     AppNotify::FromVehicle | AppNotify::Important,
                                     sub);
                    //apxMsg() << QString("<[replay]%1").arg(sv);
                    App::sound(sv);
                } else {
                    if (evt == "mission") {
                        //emit discardRequests();
                        vehicle->f_mission->storage->loadMission(uid);
                    } else if (evt == "nodes") {
                        //emit discardRequests();
                        vehicle->f_nodes->storage->loadConfiguration(uid);
                    } else if (evt == "conf") {
                        vehicle->f_nodes->loadConfValue(uid, sv);
                        QString fn = sv.left(sv.indexOf('='));
                        if (sv.size() > (fn.size() + 32) || sv.contains('\n')) {
                            sv = fn + "=<data>";
                        }
                    } else if (evt == "serial") {
                        qDebug() << evt << sv;
                        continue;
                    }
                    AppNotify::NotifyFlags flags = AppNotify::Important;
                    if (type != 3)
                        flags |= AppNotify::FromVehicle;
                    QString s = QString("%1: %2").arg(type == 3 ? ">" : "<").arg(evt);
                    if (!sv.isEmpty())
                        s.append(QString(" (%1)").arg(sv));
                    vehicle->message(s, flags);
                    //apxMsg() << s;
                }
            }
        }

        //update states
        if (updCnt) {
            vehicle->downstreamDataReceived();
            //qDebug()<<updCnt<<tNext;
        }
        if (_time != t) {
            _time = t;
            blockTimeChange = true;
            f_time->setValue(_time);
            blockTimeChange = false;
        }

        //continue next time event
        if (tNextMin == tNext) {
            apxMsg() << tr("Replay finished");
            stop();
        } else {
            tNext = tNextMin;
        }
    }
    if (!active())
        return;
    if (tNext > t) {
        timer.start(tNext - t);
    } else
        next();
}
//==============================================================================
//=============================================================================
double TelemetryPlayer::sampleValue(quint64 fieldID, double t)
{
    dataPosMap[fieldID] = 0;
    QVector<QPointF> *pts = telemetry->f_reader->fieldData.value(fieldID);
    if (!pts)
        return 0;

    if (pts->size() < 50) {
        for (int i = 0; i < pts->size(); ++i) {
            if (pts->at(i).x() >= t) {
                dataPosMap[fieldID] = i;
                return pts->at(i).y();
            }
        }
        return 0;
    }
    int ts = pts->size() / 2;
    int tx = ts;
    bool bFound = false, bFwd = false;
    while (1) {
        const QPointF &p = pts->at(tx);
        double vx = p.x();
        ts >>= 1;
        if (ts == 0) {
            if (bFound) {
                dataPosMap[fieldID] = tx;
                return p.y();
            }
            ts = 1;
        }
        if (vx < t) {
            tx += ts;
            if (tx >= pts->size()) {
                tx = pts->size() - 1;
                if (ts == 1)
                    bFound = true;
            }
            bFwd = true;
        } else if (vx > t) {
            if (ts == 1 && bFwd == true)
                bFound = true;
            if (tx > ts)
                tx -= ts;
            else {
                tx = 0;
                if (ts == 1)
                    bFound = true;
            }
            bFwd = false;
        } else {
            dataPosMap[fieldID] = tx;
            return p.y();
        }
    }
}
//=============================================================================
