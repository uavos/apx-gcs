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
#include "ElevationWorker.h"
#include "ElevationDB.h"
#include "ElevationTile.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <QFile>
#include <QMutexLocker>

#include <cmath>

ElevationWorker::ElevationWorker(const QString &dbPath, QObject *parent)
    : QThread(parent)
    , m_dbPath(dbPath)
{
    setObjectName("ElevationWorker");
    connect(App::instance(), &App::appQuit, this, &ElevationWorker::stop, Qt::DirectConnection);
}

ElevationWorker::~ElevationWorker()
{
    stop();
}

void ElevationWorker::stop()
{
    m_quit = true;
    m_cancelCurrent = true;
    m_wait.wakeAll();
    wait();
}

void ElevationWorker::wake()
{
    if (!isRunning())
        start();
    m_wait.wakeAll();
}

ElevationWorker::Job ElevationWorker::makeProfileJob(const QGeoPath &path)
{
    Job job;
    job.type = Job::Profile;
    job.path = path;
    if (path.size() > 0) {
        job.first = path.coordinateAt(0);
        job.last = path.coordinateAt(path.size() - 1);
        job.first.setAltitude(0);
        job.last.setAltitude(0);
    }
    return job;
}

bool ElevationWorker::sameEndpoints(const Job &a, const Job &b)
{
    return a.first == b.first && a.last == b.last;
}

void ElevationWorker::requestElevation(double lat, double lon)
{
    Job job;
    job.type = Job::Elevation;
    job.lat = lat;
    job.lon = lon;
    {
        QMutexLocker lock(&m_mutex);
        // only the latest hover position is interesting
        for (auto it = m_points.begin(); it != m_points.end();) {
            if (it->type == Job::Elevation)
                it = m_points.erase(it);
            else
                ++it;
        }
        m_points.push_back(job);
    }
    wake();
}

void ElevationWorker::requestCoordinate(double lat, double lon)
{
    Job job;
    job.type = Job::Coordinate;
    job.lat = lat;
    job.lon = lon;
    {
        QMutexLocker lock(&m_mutex);
        for (const auto &p : m_points) {
            if (p.type == Job::Coordinate && p.lat == lat && p.lon == lon)
                return; // already queued
        }
        m_points.push_back(job);
    }
    wake();
}

void ElevationWorker::requestTerrainProfile(const QGeoPath &path)
{
    if (path.size() <= 0)
        return;
    Job job = makeProfileJob(path);
    {
        QMutexLocker lock(&m_mutex);
        // a newer path between the same endpoints supersedes the queued one
        for (auto it = m_profiles.begin(); it != m_profiles.end();) {
            if (sameEndpoints(*it, job))
                it = m_profiles.erase(it);
            else
                ++it;
        }
        m_profiles.push_back(job);
        if (m_hasCurrent && m_current.type == Job::Profile && sameEndpoints(m_current, job))
            m_cancelCurrent = true;
    }
    wake();
}

bool ElevationWorker::takeJob(Job &job)
{
    QMutexLocker lock(&m_mutex);
    m_hasCurrent = false;
    while (!m_quit) {
        if (!m_points.empty()) {
            job = m_points.front();
            m_points.pop_front();
            break;
        }
        if (!m_profiles.empty()) {
            job = m_profiles.front();
            m_profiles.pop_front();
            break;
        }
        m_wait.wait(&m_mutex);
    }
    if (m_quit)
        return false;
    m_current = job;
    m_hasCurrent = true;
    m_cancelCurrent = false;
    return true;
}

void ElevationWorker::run()
{
    Job job;
    while (takeJob(job)) {
        if (job.type == Job::Profile)
            processProfile(job);
        else
            processPoint(job);
    }
    m_tiles.clear();
    m_tilesLru.clear();
    m_missing.clear();
}

ElevationTile *ElevationWorker::tile(double lat, double lon)
{
    const auto name = OfflineElevationDB::createASTERFileName(lat, lon);
    auto it = m_tiles.find(name);
    if (it != m_tiles.end()) {
        if (m_tilesLru.last() != name) {
            m_tilesLru.removeOne(name);
            m_tilesLru.append(name);
        }
        return it->get();
    }
    if (m_missing.contains(name))
        return nullptr;

    const auto filePath = QString("%1/%2").arg(m_dbPath, name);
    if (!QFile::exists(filePath)) {
        m_missing.insert(name);
        return nullptr;
    }
    auto t = ElevationTile::open(filePath);
    if (!t) {
        m_missing.insert(name);
        apxMsgW() << tr("Failed to read elevation file").append(": ") << filePath;
        return nullptr;
    }
    while (m_tiles.size() >= MAX_TILES && !m_tilesLru.isEmpty()) {
        m_tiles.remove(m_tilesLru.takeFirst());
    }
    auto *ptr = t.get();
    m_tiles.insert(name, std::shared_ptr<ElevationTile>(std::move(t)));
    m_tilesLru.append(name);
    return ptr;
}

double ElevationWorker::elevationAt(double lat, double lon)
{
    auto *t = tile(lat, lon);
    if (!t)
        return NAN;
    return t->elevationAt(lat, lon);
}

void ElevationWorker::processPoint(const Job &job)
{
    const double elevation = elevationAt(job.lat, job.lon);
    if (job.type == Job::Elevation) {
        emit elevationReady(elevation);
        return;
    }
    if (std::isnan(elevation))
        emit coordinateReady(QGeoCoordinate(job.lat, job.lon));
    else
        emit coordinateReady(QGeoCoordinate(job.lat, job.lon, elevation));
}

void ElevationWorker::processProfile(const Job &job)
{
    QGeoPath route = OfflineElevationDB::prepareRoute(job.path);
    for (qsizetype i = 0; i < route.size(); ++i) {
        if (m_cancelCurrent)
            return;
        auto point = route.coordinateAt(i);
        const double elevation = elevationAt(point.latitude(), point.longitude());
        if (std::isnan(elevation)) {
            // no data for this segment: report the path as is
            emit terrainProfileReady(job.path);
            return;
        }
        point.setAltitude(elevation);
        route.replaceCoordinate(i, point);
    }
    if (m_cancelCurrent)
        return;
    emit terrainProfileReady(route);
}
