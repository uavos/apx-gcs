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
#include "WaypointTerrain.h"

#include <App/App.h>
#include <Mission/MissionGroup.h>
#include <Mission/UnitMission.h>
#include <Mission/Waypoint.h>

#include <QFuture>
#include <QHash>
#include <QtConcurrent>

#include <cfloat>

WaypointTerrain::WaypointTerrain(Waypoint *wp)
    : ItemTerrain(wp)
    , m_wp(wp)
{
    // rows of the waypoint menu
    f_agl = new Fact(wp, "agl", tr("AGL"), tr("Height above ground level"));
    f_agl->setUnits("m");
    f_agl->setOpt("editor", "EditorInt.qml");
    f_agl->setOpt("extrainfo", "qrc:/ExtraInfoAgl.qml");
    f_agl->move(wp->f_altitude->num() + 1);

    wp->f_altitude->setOpt("extrainfo", "qrc:/ExtraInfoAltitude.qml");

    // altitude and AGL are two views of one value: an edit of either updates the other
    connect(wp->f_altitude, &Fact::valueChanged, this, &WaypointTerrain::updateAgl);
    connect(f_agl, &Fact::valueChanged, this, &WaypointTerrain::calcAltitude);

    auto mt = missionTerrain();
    connect(mt, &MissionTerrain::startElevationChanged, this, &WaypointTerrain::updateAgl);
    connect(mt, &MissionTerrain::startElevationChanged, this, &WaypointTerrain::updateMinMaxHeight);
    connect(mt, &MissionTerrain::startElevationChanged, this, &WaypointTerrain::checkCollision);

    connect(wp->f_amsl, &Fact::valueChanged, this, &WaypointTerrain::recalcAltitude);
    connect(wp->f_amsl, &Fact::valueChanged, this, &WaypointTerrain::updateAgl);
    connect(wp->f_amsl, &Fact::valueChanged, this, &WaypointTerrain::updateAglEnabled);
    connect(wp->f_altitude, &Fact::valueChanged, this, &WaypointTerrain::updateMinMaxHeight);
    connect(f_agl, &Fact::valueChanged, this, &WaypointTerrain::checkCollision);
    connect(wp, &MissionItem::itemDataLoaded, this, &WaypointTerrain::updateAgl);

    connect(this, &ItemTerrain::elevationChanged, this, &WaypointTerrain::updateAgl);
    connect(this, &ItemTerrain::elevationChanged, this, &WaypointTerrain::updateAglEnabled);

    // request the terrain profile shortly after the path was changed
    m_geoPathTimer.setSingleShot(true);
    m_geoPathTimer.setInterval(TIMEOUT);
    connect(wp, &MissionItem::geoPathChanged, &m_geoPathTimer, qOverload<>(&QTimer::start));
    connect(&m_geoPathTimer, &QTimer::timeout, this, &WaypointTerrain::sendTerrainProfileRequest);

    connect(this, &WaypointTerrain::minHeightChanged, mt, &MissionTerrain::updateMinHeight);
    connect(this, &WaypointTerrain::maxHeightChanged, mt, &MissionTerrain::updateMaxHeight);
    connect(this, &WaypointTerrain::collisionChanged, mt, &MissionTerrain::checkCollision);

    connect(&m_watcher,
            &QFutureWatcher<TerrainInfo>::finished,
            this,
            &WaypointTerrain::updateTerrainInfo);
    connect(App::instance(), &App::appQuit, &m_watcher, &QFutureWatcher<TerrainInfo>::cancel);
    connect(&m_pointsWatcher,
            &QFutureWatcher<QList<QGeoCoordinate>>::finished,
            this,
            &WaypointTerrain::insertNewPoints);
    connect(App::instance(),
            &App::appQuit,
            &m_pointsWatcher,
            &QFutureWatcher<QList<QGeoCoordinate>>::cancel);

    // map highlight
    connect(f_agl, &Fact::valueChanged, this, &WaypointTerrain::updateMapAlarm);
    connect(this, &WaypointTerrain::collisionChanged, this, &WaypointTerrain::updateMapAlarm);
    connect(this, &ItemTerrain::elevationChanged, this, &WaypointTerrain::updateMapAlarm);

    // the place in the mission defines the previous waypoint and the takeoff leg
    connect(wp, &Fact::numChanged, this, &WaypointTerrain::watchPrevious);
    connect(wp, &Fact::numChanged, this, &WaypointTerrain::updateFirst);

    watchPrevious();
    updateFirst();
    updateMinMaxHeight();
    updateAgl();
    updateAglEnabled();
}

WaypointTerrain *WaypointTerrain::of(MissionItem *item)
{
    return qobject_cast<WaypointTerrain *>(ItemTerrain::of(item));
}

void WaypointTerrain::detach()
{
    if (m_watcher.isRunning())
        m_watcher.cancel();
    if (m_pointsWatcher.isRunning())
        m_pointsWatcher.cancel();
    f_agl->deleteFact();
    m_wp->f_altitude->setOpt("extrainfo", QVariant());
    m_wp->setOpt("alarm", QVariant());
    m_wp->setOpt("pathColor", QVariant());
}

Waypoint *WaypointTerrain::prevWaypoint() const
{
    auto group = m_wp->parentFact();
    return group ? qobject_cast<Waypoint *>(group->child(m_wp->indexInParent() - 1)) : nullptr;
}

MissionTerrain *WaypointTerrain::missionTerrain() const
{
    return MissionTerrain::of(m_wp->group->mission);
}

double WaypointTerrain::startHmsl() const
{
    return std::round(missionTerrain()->startElevation());
}

int WaypointTerrain::agl() const
{
    return f_agl->value().toInt();
}

// The waypoint before this one takes part in the collision check of the path
void WaypointTerrain::watchPrevious()
{
    auto prev = prevWaypoint();
    auto prevTerrain = WaypointTerrain::of(prev);
    Fact *altitude = prev ? prev->f_altitude : nullptr;
    Fact *agl = prevTerrain ? prevTerrain->f_agl : nullptr;
    for (auto f : {altitude, agl}) {
        if (f)
            connect(f,
                    &Fact::valueChanged,
                    this,
                    &WaypointTerrain::checkCollision,
                    Qt::UniqueConnection);
    }
}

void WaypointTerrain::updateFirst()
{
    if (m_wp->num() != 0)
        return;
    // a profile left from another place in the mission
    m_terrainProfilePath = QGeoPath();
    m_terrainProfileMin = 0;
    m_terrainProfileMax = 200;
    if (!m_terrainProfile.isEmpty())
        clearTerrainProfile();
    setCollision(false);
    updateMinMaxHeight();
}

// AGL is a height above the terrain, i.e. an absolute height: it can be entered
// only in AMSL mode (a relative altitude would depend on the takeoff elevation,
// which may change) and only when the terrain under the waypoint is known
void WaypointTerrain::updateAglEnabled()
{
    const bool amsl = m_wp->f_amsl->value().toBool();
    f_agl->setEnabled(amsl && !std::isnan(m_elevation));
    f_agl->setDescr(amsl ? tr("Height above ground level")
                         : tr("Height above ground level (AMSL mode only)"));
}

// The map shows a waypoint with the "alarm" option highlighted and draws the
// path to it with the "pathColor" option colour
void WaypointTerrain::updateMapAlarm()
{
    const bool alarm = !std::isnan(m_elevation) && (agl() < UNSAFE_AGL || m_collision);
    m_wp->setOpt("alarm", alarm ? QVariant(true) : QVariant());
    m_wp->setOpt("pathColor", m_collision ? QVariant("red") : QVariant());
}

// ---- altitude and AGL

void WaypointTerrain::calcAltitude()
{
    if (m_aglComputing)
        return;
    const auto v = f_agl->value();
    if (!v.isValid() || v.toString().isEmpty()) {
        // the value was reset, not edited
        updateAgl();
        return;
    }
    if (std::isnan(m_elevation)) {
        // no terrain data: AGL is undefined
        setComputedAgl(0);
        return;
    }

    auto heightAmsl = m_elevation + v.toDouble();
    if (m_wp->f_amsl->value().toBool())
        m_wp->f_altitude->setValue(heightAmsl);
    else
        m_wp->f_altitude->setValue(heightAmsl - startHmsl());
}

// AMSL mode was switched: keep the height, change the number
void WaypointTerrain::recalcAltitude()
{
    auto hmsl = startHmsl();
    auto alt = m_wp->f_altitude->value().toDouble();
    alt += m_wp->f_amsl->value().toBool() ? hmsl : -hmsl;
    m_wp->f_altitude->setValue(alt);
}

void WaypointTerrain::updateAgl()
{
    if (std::isnan(m_elevation)) {
        setComputedAgl(0);
        return;
    }
    int diff = m_wp->f_altitude->value().toInt() - static_cast<int>(m_elevation);
    if (!m_wp->f_amsl->value().toBool())
        diff += startHmsl();
    setComputedAgl(diff);
}

void WaypointTerrain::setComputedAgl(int v)
{
    m_aglComputing = true;
    f_agl->setValue(v);
    m_aglComputing = false;
}

// ---- terrain profile

void WaypointTerrain::setTerrainProfile(const QList<QPointF> &v)
{
    if (m_terrainProfile == v)
        return;
    m_terrainProfile = v;
    emit terrainProfileChanged();
}

void WaypointTerrain::clearTerrainProfile()
{
    m_terrainProfile.clear();
    emit terrainProfileChanged();
}

void WaypointTerrain::sendTerrainProfileRequest()
{
    if (m_wp->num() == 0)
        return;
    emit requestTerrainProfile(m_wp->geoPath());
}

void WaypointTerrain::buildTerrainProfile(const QGeoPath &path)
{
    if (m_wp->num() == 0)
        return;
    auto geoPath = m_wp->geoPath();
    if (geoPath.size() <= 0 || path.size() <= 0)
        return;
    auto first = geoPath.coordinateAt(0);
    auto last = geoPath.coordinateAt(geoPath.size() - 1);
    auto firstIn = path.coordinateAt(0);
    auto lastIn = path.coordinateAt(path.size() - 1);

    first.setAltitude(0);
    last.setAltitude(0);
    firstIn.setAltitude(0);
    lastIn.setAltitude(0);

    if (first != firstIn || last != lastIn)
        return;

    if (m_watcher.isRunning())
        m_watcher.cancel();

    // no elevation data for this path
    if (geoPath == path) {
        clearTerrainProfile();
        setCollision(false);
        return;
    }
    // the previous profile stays visible until the new one is ready

    QFuture<TerrainInfo> future;
    future = QtConcurrent::run(createTerrainInfo, path);
    m_watcher.setFuture(future);
}

void WaypointTerrain::createTerrainInfo(QPromise<TerrainInfo> &promise, const QGeoPath &path)
{
    QPointF pt;
    double ptDistance{0};
    double ptElevation{0};
    QGeoCoordinate current;
    QGeoCoordinate next;
    TerrainInfo info;
    info.terrainProfile = {};
    info.minHeight = 0;
    info.maxHeight = 0;

    auto lastIndex = path.size() - 1;
    for (qsizetype i = 0; i < lastIndex; ++i) {
        promise.suspendIfRequested();
        if (promise.isCanceled()) {
            return;
        }
        current = path.coordinateAt(i);
        next = path.coordinateAt(i + 1);
        ptElevation = current.altitude();
        info.terrainProfile.append(QPointF(ptDistance, ptElevation));
        ptDistance += current.distanceTo(next);
        if (qIsNaN(ptDistance))
            continue;
        info.minHeight = qMin(info.minHeight, ptElevation);
        info.maxHeight = qMax(info.maxHeight, ptElevation);
    }
    ptElevation = path.coordinateAt(lastIndex).altitude();
    info.terrainProfilePath = path;
    info.terrainProfile.append(QPointF(ptDistance, ptElevation));
    info.minHeight = qMin(info.minHeight, ptElevation);
    info.maxHeight = qMax(info.maxHeight, ptElevation);
    promise.addResult(info);
}

void WaypointTerrain::updateTerrainInfo()
{
    if (m_watcher.isCanceled() || m_watcher.future().resultCount() <= 0)
        return;
    auto result = m_watcher.result();
    m_terrainProfilePath = result.terrainProfilePath;
    m_terrainProfileMin = result.minHeight;
    m_terrainProfileMax = result.maxHeight;
    updateMinMaxHeight();
    setTerrainProfile(result.terrainProfile);
    checkCollision();
}

bool WaypointTerrain::terrainProfileNeedUpdate() const
{
    if (m_wp->num() == 0)
        return false;

    if (m_geoPathTimer.isActive())
        return false;

    const auto geoPath = m_wp->geoPath();
    if (geoPath.isEmpty())
        return false;

    if (m_terrainProfilePath.isEmpty())
        return true;

    auto p1 = geoPath.coordinateAt(0);
    auto p2 = m_terrainProfilePath.coordinateAt(0);
    p1.setAltitude(0);
    p2.setAltitude(0);
    if (p1 != p2)
        return true;

    p1 = geoPath.coordinateAt(geoPath.size() - 1);
    p2 = m_terrainProfilePath.coordinateAt(m_terrainProfilePath.size() - 1);
    p1.setAltitude(0);
    p2.setAltitude(0);
    if (p1 != p2)
        return true;

    if (m_terrainProfile.isEmpty())
        return true;

    double distance = m_terrainProfile.last().x();
    double length = m_terrainProfilePath.length(0, m_terrainProfilePath.size() - 1);
    if (!qFuzzyCompare(distance, length))
        return true;

    return false;
}

// ---- height range and collision

void WaypointTerrain::setMinHeight(double v)
{
    if (m_minHeight == v)
        return;
    m_minHeight = v;
    emit minHeightChanged();
}

void WaypointTerrain::setMaxHeight(double v)
{
    if (m_maxHeight == v)
        return;
    m_maxHeight = v;
    emit maxHeightChanged();
}

void WaypointTerrain::setCollision(bool v)
{
    if (m_collision == v)
        return;
    m_collision = v;
    emit collisionChanged();
}

void WaypointTerrain::updateMinMaxHeight()
{
    double alt = m_wp->f_altitude->value().toDouble();
    if (!m_wp->f_amsl->value().toBool())
        alt += startHmsl();
    setMinHeight(!qIsNaN(alt) ? qMin(m_terrainProfileMin, alt) : m_terrainProfileMin);
    setMaxHeight(!qIsNaN(alt) ? qMax(m_terrainProfileMax, alt) : m_terrainProfileMax);
}

void WaypointTerrain::checkCollision()
{
    if (!m_wp->reachable()) {
        setCollision(true);
        return;
    }

    if (m_terrainProfile.empty()) {
        setCollision(false);
        return;
    }

    double prevAlt{0};
    auto hmsl = startHmsl();
    auto prevWp = prevWaypoint();
    auto prevTerrain = WaypointTerrain::of(prevWp);

    // Checking the first point
    if (prevWp) {
        prevAlt = prevWp->f_altitude->value().toInt();
        if (!prevWp->f_amsl->value().toBool())
            prevAlt += hmsl;
    } else {
        prevAlt = hmsl != 0 ? hmsl : m_terrainProfile.first().y();
    }

    // Checking points on top of each other
    auto dst = m_terrainProfile.last().x();
    if (dst == 0) {
        bool prevCollision = prevTerrain ? (prevTerrain->agl() < UNSAFE_AGL) : false;
        bool currentCollision = (agl() < UNSAFE_AGL);
        setCollision(currentCollision && prevCollision);
        return;
    }

    auto alt = m_wp->f_altitude->value().toInt();
    if (!m_wp->f_amsl->value().toBool())
        alt += hmsl;

    double tan = static_cast<double>((alt - prevAlt) / dst);
    for (const auto &tp : m_terrainProfile) {
        // proportional increase in safe AGL for the first point
        double k = !prevWp ? (tp.x() / dst) : 1;
        auto safeHeight = tp.y() + UNSAFE_AGL * k;
        auto routeHeight = prevAlt + tp.x() * tan;
        auto diff = std::abs(safeHeight - routeHeight);
        if (routeHeight < safeHeight && ALT_EPS < diff) {
            setCollision(true);
            return;
        }
    }
    setCollision(false);
}

// ---- path correction

void WaypointTerrain::correctPath(double maxSlope)
{
    // Skip waypoint without collision or unreachable
    auto prevWp = prevWaypoint();
    if (!m_collision || !m_wp->reachable() || !prevWp || m_terrainProfile.isEmpty()) {
        emit responseCorrectPath(QList<QGeoCoordinate>(), m_wp->indexInParent());
        return;
    }

    // Correct point altitude
    auto alt = m_wp->f_altitude->value().toInt();
    if (agl() < UNSAFE_AGL) {
        alt += UNSAFE_AGL - agl();
        m_wp->f_altitude->setValue(alt);
    }

    auto hmsl = startHmsl();
    const bool amsl = m_wp->f_amsl->value().toBool();

    // The profile ends with the highest terrain around the point (not only right
    // under it) and the descent to the point may be limited: the waypoint must be
    // at the required height of the end of its path
    const auto need = requiredHeights(m_terrainProfilePath, maxSlope);
    {
        const int altAmsl = amsl ? alt : alt + static_cast<int>(hmsl);
        const int lack = static_cast<int>(std::ceil(need.last())) - altAmsl;
        if (lack > 0) {
            alt += lack;
            m_wp->f_altitude->setValue(alt);
        }
    }

    if (!amsl)
        alt += hmsl;

    // the previous point is corrected by its own turn of the mission correction
    // (or lifted by this one when the climb right after it is too steep)
    auto prevAlt = prevWp->f_altitude->value().toInt();
    if (!prevWp->f_amsl->value().toBool())
        prevAlt += hmsl;

    QFuture<QList<QGeoCoordinate>> future;
    future = QtConcurrent::run(getCorrectRoutePoints, m_terrainProfilePath, need, prevAlt, alt);
    m_pointsWatcher.setFuture(future);
}

QList<double> WaypointTerrain::requiredHeights(const QGeoPath &path, double maxSlope)
{
    const qsizetype n = path.size();
    QList<double> need;
    need.reserve(n);
    for (qsizetype i = 0; i < n; ++i)
        need.append(path.coordinateAt(i).altitude() + UNSAFE_AGL);
    if (maxSlope <= 0 || n < 2)
        return need;
    // descent limit: after a high point the route can only go down so fast
    for (qsizetype i = 1; i < n; ++i) {
        const double d = path.coordinateAt(i - 1).distanceTo(path.coordinateAt(i));
        need[i] = std::max(need[i], need[i - 1] - maxSlope * d);
    }
    // climb limit: to reach a high point the route must already be high before it
    for (qsizetype i = n - 2; i >= 0; --i) {
        const double d = path.coordinateAt(i).distanceTo(path.coordinateAt(i + 1));
        need[i] = std::max(need[i], need[i + 1] - maxSlope * d);
    }
    return need;
}

void WaypointTerrain::insertNewPoints()
{
    if (m_pointsWatcher.isCanceled() || m_pointsWatcher.future().resultCount() <= 0)
        return;
    emit responseCorrectPath(m_pointsWatcher.result(), m_wp->indexInParent());
}

// Lifts a point above the terrain around it (NaN: no data, the point is left as is)
void WaypointTerrain::liftPoints(QList<QGeoCoordinate> &points, const QList<double> &terrain)
{
    if (points.size() != terrain.size())
        return;
    for (qsizetype i = 0; i < points.size(); ++i) {
        const double e = terrain.at(i);
        if (std::isnan(e))
            continue;
        const double safe = std::ceil(e) + UNSAFE_AGL;
        if (points[i].altitude() < safe)
            points[i].setAltitude(safe);
    }
}

void WaypointTerrain::getCorrectRoutePoints(QPromise<QList<QGeoCoordinate>> &promise,
                                            const QGeoPath &path,
                                            const QList<double> &need,
                                            int hFirst,
                                            int hLast)
{
    int pathSize = path.size();
    if (pathSize < 2 || need.size() != pathSize) {
        promise.addResult(QList<QGeoCoordinate>());
        return;
    }
    // Anchors of the route: the ends and, step by step, the points where a straight
    // line between the neighbouring anchors goes below the required height.
    // Each anchor has its height: the required one, or more when a violation too
    // close to it (no room for a new waypoint) is covered by lifting the anchor.
    const int last = pathSize - 1;
    QList<int> indexes{0, last};
    QHash<int, double> height;
    // the previous waypoint itself may have to be higher: the climb right after it
    height[0] = std::max<double>(hFirst, need.first());
    height[last] = hLast;

    int count{0};
    bool hasCollision = true;
    while (hasCollision && count < pathSize) {
        hasCollision = false;
        QList<int> tmp;
        for (int i = 0; i < indexes.size() - 1; i++) {
            auto begin = indexes[i];
            auto end = indexes[i + 1];

            // heights of the anchors and the slope between them
            const double h1 = height[begin];
            const double h2 = height[end];
            const double dst = path.length(begin, end);
            const double tan = (h2 - h1) / dst;

            // the worst point between them
            int unsafeIndex{-1};
            double unsafeHeightDiff{0};
            double unsafeDst{0};
            for (int j = begin + 1; j < end; j++) {
                const double dj = path.length(begin, j);
                const double routeHeight = h1 + dj * tan;
                const double diff = need[j] - routeHeight;
                if (diff > ALT_EPS && unsafeHeightDiff < diff) {
                    unsafeHeightDiff = diff;
                    unsafeIndex = j;
                    unsafeDst = dj;
                }
            }
            if (unsafeIndex < 0)
                continue;
            hasCollision = true;

            // room for a waypoint: a new anchor
            if (unsafeDst >= MIN_SPACING && dst - unsafeDst >= MIN_SPACING) {
                tmp.append(unsafeIndex);
                continue;
            }
            // too close to an anchor: lift the nearer one so the line clears the point
            const bool nearBegin = unsafeDst < dst - unsafeDst;
            const double share = nearBegin ? 1.0 - unsafeDst / dst : unsafeDst / dst;
            const int k = nearBegin ? begin : end;
            height[k] += unsafeHeightDiff / std::max(share, 0.01);
        }
        for (auto idx : std::as_const(tmp))
            height[idx] = need[idx];
        indexes.append(tmp);
        std::sort(indexes.begin(), indexes.end());
        count++;
    }
    const double firstAlt = height[0];

    // Find the first point of a straight section of a path
    int linesFirstIndex{-1};
    const double epsAz = 0.001;
    auto lastPoint = path.coordinateAt(pathSize - 1);
    for (int i = 0; i < path.size() - 1; i++) {
        auto point = path.coordinateAt(i);
        auto nextPoint = path.coordinateAt(i + 1);
        auto az = point.azimuthTo(lastPoint);
        auto nextAz = nextPoint.azimuthTo(lastPoint);
        if (linesFirstIndex >= 0)
            break;
        if (std::abs(az - nextAz) < epsAz)
            linesFirstIndex = i + 1;
    }

    QList<QGeoCoordinate> newPoints;

    // Anchors inside the turn after the previous waypoint can not become waypoints:
    // the turn is flown at the required height of its highest point instead
    double alt4Correct = linesFirstIndex >= 0 ? need[linesFirstIndex] : 0;
    for (int i = 1; i < indexes.size() - 1; i++) {
        if (linesFirstIndex <= 0)
            break;

        // If the point belongs to a straight section
        if (linesFirstIndex < indexes[i])
            continue;

        // First point for altitude correction append
        // and max required height on the interval
        if (newPoints.size() == 0) {
            newPoints.append(path.coordinateAt(0));
            for (int j = 0; j < linesFirstIndex; j++)
                alt4Correct = std::max(alt4Correct, need[j]);
        }
        alt4Correct = std::max(alt4Correct, height[indexes[i]]);

        // Correct first point altitude if needed
        if (indexes[i] < linesFirstIndex && linesFirstIndex < indexes[i + 1]) {
            auto alt1 = height[indexes[i]];
            auto alt2 = height[indexes[i + 1]];
            auto dst = path.length(indexes[i], indexes[i + 1]);
            auto indexDst = path.length(indexes[i], linesFirstIndex);
            auto altCorrection = alt1 + (alt2 - alt1) * indexDst / dst;
            alt4Correct = std::max(alt4Correct, altCorrection);
        }
    }

    // Altitudes are stored as whole metres: round them up, a point even a metre
    // below its required height would keep the collision for ever
    // Add second point for correction
    if (newPoints.size() != 0) {
        auto point = path.coordinateAt(linesFirstIndex);
        point.setAltitude(std::ceil(alt4Correct));
        newPoints.append(point);
        newPoints[0].setAltitude(std::ceil(std::max(firstAlt, alt4Correct)));
    } else if (firstAlt > hFirst) {
        // nothing in the turn, but the previous waypoint has to be lifted
        auto point = path.coordinateAt(0);
        point.setAltitude(std::ceil(firstAlt));
        newPoints.append(point);
    }

    // Add new points
    for (int i = 1; i < indexes.size() - 1; i++) {
        if (indexes[i] <= linesFirstIndex)
            continue;
        auto point = path.coordinateAt(indexes[i]);
        point.setAltitude(std::ceil(height[indexes[i]]));
        newPoints.append(point);
    }

    // the waypoint itself has to be lifted: the last point of the result
    if (height[last] > hLast + ALT_EPS) {
        auto point = path.coordinateAt(last);
        point.setAltitude(std::ceil(height[last]));
        newPoints.append(point);
    }

    promise.addResult(newPoints);
}
