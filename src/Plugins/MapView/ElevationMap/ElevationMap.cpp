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
#include "ElevationMap.h"
#include "MissionTerrain.h"
#include "TerrainProfileItem.h"
#include "WaypointMarkersItem.h"
#include "WaypointTerrain.h"
#include <App/App.h>
#include <App/AppSettings.h>
#include <Fleet/Fleet.h>
#include <Fleet/Unit.h>
#include <Mission/MissionTools.h>
#include <Mission/Runway.h>
#include <Mission/UnitMission.h>
#include <Mission/Waypoint.h>

#include <QFileDialog>
#include <QMap>
#include <QQmlEngine>

ElevationMap::ElevationMap(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Elevation Map"),
           tr("Terrain elevation map"),
           Group | FlatModel,
           "elevation-rise")
    , m_elevation(qQNaN())
{
    auto path = AppDirs::db().absolutePath() + "/Elevation";

    f_use = new Fact(this, "use", tr("Use elevation map"), "", Bool, "check");
    f_use->setValue(false);

    f_path = new Fact(this,
                      "open",
                      tr("Path"),
                      tr("Elevation files path"),
                      Text | PersistentValue,
                      "import");
    f_path->setDefaultValue(path);

    f_corridor = new Fact(this,
                          "corridor",
                          tr("Corridor"),
                          tr("Half-width of the terrain profile corridor"),
                          Int | PersistentValue,
                          "arrow-expand-horizontal");
    f_corridor->setUnits("m");
    f_corridor->setMin(0);
    f_corridor->setMax(5000);
    f_corridor->setDefaultValue(100);

    f_showAgl = new Fact(this,
                         "agl",
                         tr("Unit AGL"),
                         tr("Show the height above terrain near the aircraft"),
                         Bool | PersistentValue,
                         "airplane");
    f_showAgl->setDefaultValue(false);

    connect(this, &Fact::pathChanged, this, &ElevationMap::getPluginEnableControl);
    connect(Fleet::instance(), &Fleet::currentChanged, this, &ElevationMap::updateMission);
    connect(f_use, &Fact::valueChanged, this, &ElevationMap::changeExternalsVisibility);
    connect(f_path, &Fact::valueChanged, this, &ElevationMap::createElevationDatabase);
    connect(f_path, &Fact::triggered, this, &ElevationMap::onOpenTriggered);
    connect(f_corridor, &Fact::valueChanged, this, &ElevationMap::onCorridorChanged);
    connect(f_showAgl, &Fact::valueChanged, this, &ElevationMap::updateUnitAgl);
    m_aglTimer.setInterval(AGL_PERIOD);
    connect(&m_aglTimer, &QTimer::timeout, this, &ElevationMap::updateUnitAgl);
    m_coverageTimer.setSingleShot(true);
    m_coverageTimer.setInterval(0);
    connect(&m_coverageTimer, &QTimer::timeout, this, &ElevationMap::updateCoverage);
    createDir(path);
    updateMission();
    createElevationDatabase();
    qmlRegisterType<TerrainProfileItem>("Apx.Elevation", 1, 0, "TerrainProfileItem");
    qmlRegisterType<WaypointMarkersItem>("Apx.Elevation", 1, 0, "WaypointMarkersItem");
    qml = loadQml("qrc:/ElevationPlugin.qml");
    loadQml("qrc:/ElevationMapLayer.qml"); // map layer: elevation under the cursor
    loadQml("qrc:/ElevationMapInfo.qml");  // map info line: the elevation value
}

// without elevation files no request reaches the worker (its thread is never started)
void ElevationMap::setCoordinateWithElevation(const QGeoCoordinate &coordinate)
{
    if (!f_use->value().toBool() || !m_available)
        return;
    m_elevationDB->requestCoordinate(coordinate.latitude(), coordinate.longitude());
}

void ElevationMap::setElevationByCoordinate(const QGeoCoordinate &coordinate)
{
    if (!m_available)
        return;
    m_elevationDB->requestElevation(coordinate.latitude(), coordinate.longitude());
}

void ElevationMap::setTerrainProfile(const QGeoPath &path)
{
    if (!f_use->value().toBool() || !m_available)
        return;
    m_elevationDB->requestTerrainProfile(path);
}

QObject *ElevationMap::terrain(QObject *item) const
{
    return ItemTerrain::of(qobject_cast<MissionItem *>(item));
}

QObject *ElevationMap::missionTerrain(QObject *mission) const
{
    return MissionTerrain::of(qobject_cast<UnitMission *>(mission));
}

void ElevationMap::createElevationDatabase()
{
    auto path = f_path->value().toString();
    scanTiles();
    m_elevationDB = QSharedPointer<OfflineElevationDB>::create(path);
    connect(m_elevationDB.data(),
            &OfflineElevationDB::coordinateReceived,
            this,
            &ElevationMap::setCoordinate);
    connect(m_elevationDB.data(),
            &OfflineElevationDB::elevationReceived,
            this,
            &ElevationMap::setElevation);
    connect(m_elevationDB.data(),
            &OfflineElevationDB::terrainProfileReceived,
            this,
            &ElevationMap::setGeoPath);
    connect(m_elevationDB.data(),
            &OfflineElevationDB::terrainProfileMinReceived,
            this,
            &ElevationMap::onTerrainProfileMin);
    connect(m_elevationDB.data(),
            &OfflineElevationDB::areaMaxReceived,
            this,
            &ElevationMap::setUnitTerrain);
    connect(m_elevationDB.data(),
            &OfflineElevationDB::waypointsTerrainReceived,
            this,
            &ElevationMap::onWaypointsTerrain);
    m_elevationDB->setCorridor(f_corridor->value().toDouble());
    m_minProfiles.clear();
    changeExternalsVisibility();
}

void ElevationMap::onCorridorChanged()
{
    m_elevationDB->setCorridor(f_corridor->value().toDouble());
    // profiles depend on the corridor: recompute them all
    auto m = mission();
    for (int i = 0; i < m->f_wp->size(); ++i) {
        if (auto t = WaypointTerrain::of(static_cast<MissionItem *>(m->f_wp->child(i))))
            t->sendTerrainProfileRequest();
    }
    updateUnitAgl();
}

QString ElevationMap::profileKey(const QGeoPath &path)
{
    if (path.size() <= 0)
        return QString();
    auto a = path.coordinateAt(0);
    auto b = path.coordinateAt(path.size() - 1);
    a.setAltitude(0);
    b.setAltitude(0);
    return a.toString(QGeoCoordinate::Degrees) + "|" + b.toString(QGeoCoordinate::Degrees);
}

void ElevationMap::onTerrainProfileMin(QGeoPath path, QList<double> minElevations)
{
    if (path.size() != minElevations.size() || path.size() <= 0)
        return;
    QList<QPointF> profile;
    profile.reserve(path.size());
    double distance = 0;
    for (qsizetype i = 0; i < path.size(); ++i) {
        profile.append(QPointF(distance, minElevations[i]));
        if (i + 1 < path.size())
            distance += path.coordinateAt(i).distanceTo(path.coordinateAt(i + 1));
    }
    m_minProfiles.insert(profileKey(path), profile);
}

QList<QPointF> ElevationMap::minProfile(const QGeoPath &path) const
{
    return m_minProfiles.value(profileKey(path));
}

// ==== Unit AGL (real time)
// The terrain around the unit is requested from the worker thread every
// AGL_PERIOD while the option is on; the AGL itself follows the unit altitude
void ElevationMap::updateUnitAgl()
{
    if (!f_showAgl->value().toBool() || !f_showAgl->enabled() || !unit()) {
        m_aglTimer.stop();
        setUnitTerrain(qQNaN());
        return;
    }
    if (!m_aglTimer.isActive())
        m_aglTimer.start();
    const auto pos = unit()->coordinate();
    if (!pos.isValid())
        return;
    m_elevationDB->requestAreaMax(pos.latitude(), pos.longitude(), f_corridor->value().toDouble());
}

void ElevationMap::setUnitTerrain(double elevation)
{
    m_unitTerrain = elevation;
    recalcUnitAgl();
}

void ElevationMap::recalcUnitAgl()
{
    double agl = qQNaN();
    if (!std::isnan(m_unitTerrain) && unit()) {
        auto f = unit()->f_mandala->fact(mandala::est::nav::pos::hmsl::uid);
        if (f)
            agl = f->value().toDouble() - m_unitTerrain;
    }
    if (std::isnan(agl) && std::isnan(m_unitAgl) && std::isnan(m_unitTerrain))
        return;
    m_unitAgl = agl;
    emit unitAglChanged();
}

void ElevationMap::scanTiles()
{
    m_tileNames.clear();
    QDir dir(f_path->value().toString());
    for (const auto &name : dir.entryList({"ASTGTMV003_*_dem.tif"}, QDir::Files))
        m_tileNames.insert(name);
    const bool available = !m_tileNames.isEmpty();
    if (m_available == available)
        return;
    m_available = available;
    emit availableChanged();
}

bool ElevationMap::hasTile(const QGeoCoordinate &c) const
{
    if (!c.isValid())
        return true;
    return m_tileNames.contains(
        OfflineElevationDB::createASTERFileName(c.latitude(), c.longitude()));
}

void ElevationMap::scheduleCoverage()
{
    m_coverageTimer.start();
}

// Checks that the elevation files cover the mission area (at least one item
// lies on an existing file; items outside simply get no elevation) and
// re-evaluates the plugin state. Items are watched for position changes so
// moving the mission outside the map (or back) updates the state as well.
void ElevationMap::updateCoverage()
{
    if (m_rebuilding)
        return;
    auto m = mission();
    int items = 0;
    int coveredItems = 0;
    for (Fact *group : {static_cast<Fact *>(m->f_wp),
                        static_cast<Fact *>(m->f_rw),
                        static_cast<Fact *>(m->f_pi)}) {
        for (int i = 0; i < group->size(); ++i) {
            auto item = static_cast<MissionItem *>(group->child(i));
            if (!item)
                continue;
            connect(item,
                    &MissionItem::coordinateChanged,
                    this,
                    &ElevationMap::scheduleCoverage,
                    Qt::UniqueConnection);
            items++;
            if (hasTile(item->coordinate()))
                coveredItems++;
        }
    }
    const bool covered = items == 0 || coveredItems > 0;
    if (m_covered != covered) {
        m_covered = covered;
        emit coveredChanged();
    }
    updateActive();
}

void ElevationMap::updateActive()
{
    bool useValue{false};
    bool controlValue{false};
    if (f_control && !f_control->busy())
        controlValue = f_control->value().toBool();
    if (f_use)
        useValue = f_use->value().toBool();
    const bool active = controlValue && useValue && m_available && m_covered;
    if (m_active != active) {
        m_active = active;
        emit activeChanged();
    }
    setMissionValues(m_active);
    const bool usable = controlValue && useValue && m_available;
    if (m_usable != usable) {
        m_usable = usable;
        emit usableChanged();
    }
    // unit AGL makes sense only with elevation files in use
    f_showAgl->setEnabled(usable);
    updateUnitAgl();
}

void ElevationMap::onOpenTriggered()
{
    QString path = QFileDialog::getExistingDirectory(nullptr,
                                                     tr("Open Directory"),
                                                     QDir::homePath(),
                                                     QFileDialog::ShowDirsOnly
                                                         | QFileDialog::DontResolveSymlinks);
    if (!path.isEmpty())
        f_path->setValue(path);
}

Unit *ElevationMap::unit() const
{
    return Fleet::instance()->current();
}

UnitMission *ElevationMap::mission() const
{
    return unit()->f_mission;
}

MissionTools *ElevationMap::missionTools() const
{
    return mission()->f_tools;
}

Fact *ElevationMap::aglset() const
{
    auto group = missionTools()->child("aglset");
    return group ? group->child("agl") : nullptr;
}

// Mission tools of the plugin ("AGL set", "All paths correction") are added to
// the mission tools menu by the plugin itself, once per mission.
void ElevationMap::createMissionTools()
{
    auto tools = missionTools();
    if (tools->child("aglset"))
        return;

    auto group = new Fact(tools, "aglset", tr("AGL set"), tr("Set all waypoints height AGL"), Group);
    group->setIcon("arrow-expand-vertical");
    group->setVisible(false);
    auto value = new Fact(group, "agl", tr("AGL value"), "", Int);
    value->setUnits("m");
    value->setIcon(group->icon());
    value->setMin(0);
    auto apply = new Fact(group,
                          "apply",
                          tr("Apply"),
                          "",
                          Action | Apply | CloseOnTrigger | ShowDisabled);
    apply->setEnabled(false);
    connect(value, &Fact::valueChanged, apply, [value, apply]() {
        apply->setEnabled(value->value().toInt() != 0);
    });
    connect(apply, &Fact::triggered, this, &ElevationMap::setMissionAgl);

    auto correct = new Fact(tools,
                            "pathscorrect",
                            tr("All paths correction"),
                            tr("Correct mission paths with unsafe agl"),
                            CloseOnTrigger,
                            "puzzle-edit-outline");
    correct->setVisible(false);
    connect(correct, &Fact::triggered, this, [this]() {
        // a correction interrupted by a mission change never completes: start over
        m_isCorrect = false;
        m_correctionPass = 0;
        startPathsCorrection();
    });

    // keep them right after the altitude tools, before "Reverse"
    const int index = tools->f_reverse->num();
    group->move(index);
    correct->move(index + 1);
    App::jsync(tools);
}

void ElevationMap::updateMission()
{
    // called on every Fleet::currentChanged - connections must not accumulate
    connect(mission(),
            &UnitMission::missionSizeChanged,
            this,
            &ElevationMap::changeExternalsVisibility,
            Qt::UniqueConnection);
    connect(mission(),
            &UnitMission::missionSizeChanged,
            MissionTerrain::of(mission()),
            &MissionTerrain::checkCollision,
            Qt::UniqueConnection);
    connect(mission(),
            &UnitMission::startPointChanged,
            this,
            &ElevationMap::setStartPointElevation,
            Qt::UniqueConnection);
    connect(missionTools()->f_reverseApply,
            &Fact::triggered,
            this,
            &ElevationMap::changeExternalsVisibility,
            Qt::UniqueConnection);
    createMissionTools();
    // real-time AGL of the current unit
    if (auto f = unit()->f_mandala->fact(mandala::est::nav::pos::hmsl::uid))
        connect(f, &Fact::valueChanged, this, &ElevationMap::recalcUnitAgl, Qt::UniqueConnection);
    setUnitTerrain(qQNaN());
    changeExternalsVisibility();
    updateRefPoint();
}

void ElevationMap::startPathsCorrection()
{
    QTimer::singleShot(100, this, &ElevationMap::correctUnsafePaths);
}

void ElevationMap::updateRefPoint()
{
    f_refHmsl = unit()->f_mandala->fact(mandala::est::nav::ref::hmsl::uid);
    f_refStatus = unit()->f_mandala->fact(mandala::est::nav::ref::status::uid);
    if (f_refHmsl)
        connect(f_refHmsl, &Fact::valueChanged, this, &ElevationMap::setStartPointElevation);
    if (f_refStatus)
        connect(f_refStatus, &Fact::valueChanged, this, &ElevationMap::setStartPointElevation);
}

void ElevationMap::getPluginEnableControl()
{
    f_control = AppSettings::instance()->findChild("application.plugins.elevationmap");
    if (f_control)
        connect(f_control, &Fact::valueChanged, this, &ElevationMap::changeExternalsVisibility);
}

void ElevationMap::setMissionAgl()
{
    auto m = mission();
    for (int i = 0; i < m->f_wp->size(); ++i) {
        auto wp = static_cast<Waypoint *>(m->f_wp->child(i));
        if (!wp)
            continue;

        auto t = WaypointTerrain::of(wp);
        if (!t || qIsNaN(t->elevation()))
            continue;
        auto elevation = t->elevation();

        auto agl = aglset();
        if (!agl)
            return;
        int v = agl->value().toInt();
        v += static_cast<int>(elevation);
        wp->f_amsl->setValue(true);
        wp->f_altitude->setValue(v);
    }
}

void ElevationMap::changeExternalsVisibility()
{
    updateCoverage();
}

// Active: every item of the current mission gets its terrain object (see
// MissionTerrain.h), which adds the plugin rows and widgets to the item.
// Passive: all of them are removed, the mission is left as the library made it.
void ElevationMap::setMissionValues(bool b)
{
    auto aglset = missionTools()->child("aglset");
    if (aglset)
        aglset->setVisible(b);

    auto pathsCorrect = missionTools()->child("pathscorrect");
    if (pathsCorrect)
        pathsCorrect->setVisible(b);

    if (!b) {
        detachTerrains();
        return;
    }
    auto m = mission();
    attachItems(m->f_rw);
    attachItems(m->f_pi);
    attachWaypoints();
    if (!m_attached)
        return;
    m_attached = false;
    setStartPointElevation();
    emit terrainsChanged();
}

void ElevationMap::detachTerrains()
{
    if (m_terrains.isEmpty())
        return;
    for (const auto &t : std::as_const(m_terrains)) {
        if (!t)
            continue;
        t->detach();
        delete t;
    }
    m_terrains.clear();
    emit terrainsChanged();
}

// common wiring of a new terrain object
void ElevationMap::attachItem(ItemTerrain *t)
{
    m_terrains.removeIf([](const QPointer<ItemTerrain> &p) { return p.isNull(); });
    m_terrains.append(t);
    m_attached = true;
    connect(this, &ElevationMap::coordinateChanged, t, &ItemTerrain::extractElevation);
    connect(t, &ItemTerrain::requestElevation, this, &ElevationMap::setCoordinateWithElevation);
    // the map info line shows the terrain under the item while it is dragged
    auto item = t->item();
    connect(item, &MissionItem::coordinateChanged, t, [this, item]() {
        setElevationByCoordinate(item->coordinate());
    });
    // requests are queued and coalesced by the elevation worker
    setCoordinateWithElevation(item->coordinate());
}

// runways and points of interest: the terrain elevation next to their HMSL editor
void ElevationMap::attachItems(Fact *group)
{
    for (int i = 0; i < group->size(); ++i) {
        auto item = static_cast<MissionItem *>(group->child(i));
        if (ItemTerrain::of(item))
            continue;
        auto t = new ItemTerrain(item);
        if (auto hmsl = item->child("hmsl"))
            t->setInfoField(hmsl);
        attachItem(t);
    }
}

void ElevationMap::attachWaypoints()
{
    auto m = mission();
    for (int i = 0; i < m->f_wp->size(); ++i) {
        auto wp = static_cast<Waypoint *>(m->f_wp->child(i));
        auto t = WaypointTerrain::of(wp);
        if (!t) {
            t = new WaypointTerrain(wp);
            attachItem(t);
            connect(t,
                    &WaypointTerrain::requestTerrainProfile,
                    this,
                    &ElevationMap::setTerrainProfile);
            connect(this, &ElevationMap::geoPathChanged, t, &WaypointTerrain::buildTerrainProfile);
            connect(t,
                    &WaypointTerrain::responseCorrectPath,
                    this,
                    &ElevationMap::getCorrectPathResponse);
            connect(t,
                    &WaypointTerrain::requestPointsTerrain,
                    this,
                    [this, t](QList<QGeoCoordinate> points) { requestPointsTerrain(t, points); });
        }
        // Check wp terrain profile has changes
        if (t->terrainProfileNeedUpdate())
            t->sendTerrainProfileRequest();
    }

    // For the first load from a file
    // when there is no elevation data at all
    auto mt = MissionTerrain::of(m);
    mt->updateMinHeight();
    mt->updateMaxHeight();
}

void ElevationMap::setGeoPath(const QGeoPath &v)
{
    // Always notify: waypoints subscribe to this signal to receive their
    // terrain profile, a repeated (identical) result must still reach them
    m_geoPath = v;
    emit geoPathChanged(m_geoPath);
}

QGeoPath ElevationMap::geoPath() const
{
    return m_geoPath;
}

QGeoCoordinate ElevationMap::coordinate() const
{
    return m_coordinate;
}

void ElevationMap::setCoordinate(const QGeoCoordinate &coordinate)
{
    // Always notify: mission items extract their elevation from this signal,
    // a re-created item at the same position must get its value again
    m_coordinate = coordinate;
    emit coordinateChanged(m_coordinate);
}

double ElevationMap::elevation() const
{
    return m_elevation;
}

void ElevationMap::setElevation(double v)
{
    if (m_elevation == v)
        return;

    m_elevation = v;
    emit elevationChanged();
}

// ==== Mission analize
void ElevationMap::setStartPointElevation()
{
    auto m = mission();
    auto hHmsl = getRefPointHmsl();
    if (m->f_rw->size() > 0) {
        auto startPoint = m->startPoint();
        auto runway = static_cast<Runway *>(m->f_rw->child(0));
        for (int i = 0; i < m->f_rw->size(); ++i) {
            auto rw = static_cast<Runway *>(m->f_rw->child(i));
            if (rw && startPoint == rw->endPoint()) {
                runway = rw;
            }
        }
        if (!runway) {
            MissionTerrain::of(m)->setStartElevation(hHmsl);
            return;
        }
        auto runwayTerrain = ItemTerrain::of(runway);
        if (runwayTerrain)
            connect(runwayTerrain,
                    &ItemTerrain::elevationChanged,
                    this,
                    &ElevationMap::setStartPointElevation,
                    Qt::UniqueConnection);
        connect(runway->f_hmsl,
                &Fact::valueChanged,
                this,
                &ElevationMap::setStartPointElevation,
                Qt::UniqueConnection);
        auto rwHmsl = runway->f_hmsl->value().toInt();
        hHmsl = rwHmsl;
        // If hmsl default
        if (rwHmsl == 0) {
            // If runway has elevation
            auto rwElevation = runwayTerrain ? runwayTerrain->elevation() : qQNaN();
            if (!std::isnan(rwElevation)) {
                hHmsl = rwElevation;
            }
            // If refpoint initialized
            auto refHmsl = getRefPointHmsl();
            if (refHmsl != 0) {
                hHmsl = refHmsl;
            }
        }
    }
    MissionTerrain::of(m)->setStartElevation(hHmsl);
}

double ElevationMap::getRefPointHmsl()
{
    double refPointHmsl{0};
    f_refHmsl = unit()->f_mandala->fact(mandala::est::nav::ref::hmsl::uid);
    f_refStatus = unit()->f_mandala->fact(mandala::est::nav::ref::status::uid);
    if (!f_refStatus)
        return refPointHmsl;
    if (!f_refHmsl)
        return refPointHmsl;
    if (f_refStatus->value().toInt() != mandala::ref_status_initialized)
        return refPointHmsl;
    refPointHmsl = f_refHmsl->value().toDouble();
    return refPointHmsl;
}

// Correct unsafe mission paths
void ElevationMap::correctUnsafePaths()
{
    if (m_isCorrect)
        return;

    auto m = mission();
    auto wpsSize = m->f_wp->size();
    if (wpsSize <= 0)
        return;

    apxMsg() << tr("Mission correction started");

    m_isCorrect = true;
    m_correction.clear();
    // correctPath touches Facts and must run in the GUI thread;
    // the heavy part (route points calculation) is scheduled by the waypoint terrain
    for (int i = 0; i < wpsSize; i++) {
        auto t = WaypointTerrain::of(static_cast<MissionItem *>(m->f_wp->child(i)));
        if (t)
            t->correctPath(true);
        else
            getCorrectPathResponse(QList<QGeoCoordinate>(), i);
    }
}

void ElevationMap::getCorrectPathResponse(QList<QGeoCoordinate> v, int index)
{
    auto m = mission();
    m_correction.insert(index, v);
    if (m_correction.size() != m->f_wp->size())
        return;
    insertMissionWaypoints();
}

void ElevationMap::insertMissionWaypoints()
{
    // Check new points for empty lists to insert
    // If all values ​​are empty, then we do not insert
    for (auto k : m_correction.keys()) {
        if (!m_correction.value(k).empty())
            break;
        else if (k != m_correction.lastKey())
            continue;
        apxMsg() << tr("Nothing to correct. Mission correction completed");
        m_isCorrect = false;
        return;
    }

    // The altitude of a new point comes from the terrain along the path. As a waypoint
    // it must also clear the terrain around it (the turn circle that ends its profile),
    // which is requested here, before the points are created.
    QList<QGeoCoordinate> points;
    for (const auto &list : std::as_const(m_correction))
        points.append(list);
    requestPointsTerrain(this, points);
}

void ElevationMap::requestPointsTerrain(QObject *requester, const QList<QGeoCoordinate> &points)
{
    m_pointsRequests.append(requester);
    if (m_available && f_use->value().toBool()) {
        m_elevationDB->requestWaypointsTerrain(points);
        return;
    }
    // no data: the points keep their altitude
    onWaypointsTerrain(QList<double>(points.size(), qQNaN()));
}

// replies come in the order of the requests (one worker queue)
void ElevationMap::onWaypointsTerrain(QList<double> elevations)
{
    if (m_pointsRequests.isEmpty())
        return;
    auto requester = m_pointsRequests.takeFirst();
    if (!requester)
        return;
    if (auto t = qobject_cast<WaypointTerrain *>(requester.data())) {
        t->insertPoints(elevations);
        return;
    }
    if (requester != this || !m_isCorrect)
        return;

    auto m = mission();
    if (m_correction.size() != m->f_wp->size()) {
        // the mission was changed meanwhile
        m_isCorrect = false;
        return;
    }
    QList<QGeoCoordinate> points;
    for (const auto &list : std::as_const(m_correction))
        points.append(list);
    WaypointTerrain::liftPoints(points, elevations);
    qsizetype k = 0;
    for (auto &list : m_correction)
        for (auto &point : list)
            point = points.at(k++);
    createCorrectedMission();
}

void ElevationMap::createCorrectedMission()
{
    // Create new waypoints array
    auto m = mission();
    QJsonArray jsa;
    QJsonObject jso;
    QList<QGeoCoordinate> newWps;
    for (int i = 0; i < m->f_wp->size(); ++i) {
        auto wp = static_cast<Waypoint *>(m->f_wp->child(i));

        // Add first waypoint
        if (i == 0) {
            jsa.append(wp->toJson());
            continue;
        }

        if (!m_correction.contains(i)) {
            jsa.append(wp->toJson());
            continue;
        }

        // Append new waypoints
        newWps = m_correction[i];
        auto prevWp = static_cast<Waypoint *>(m->f_wp->child(i - 1));
        auto prevCoordinate = prevWp->coordinate();
        for (int j = 0; j < newWps.size(); ++j) {
            if (j == 0) {
                // Check if first point equal prev waypoint
                auto latDiff = std::abs(newWps[j].latitude() - prevCoordinate.latitude());
                auto lonDiff = std::abs(newWps[j].longitude() - prevCoordinate.longitude());
                if (latDiff <= DBL_EPSILON && lonDiff <= DBL_EPSILON) {
                    auto prevAmsl = prevWp->f_amsl->value().toBool();
                    prevWp->f_amsl->setValue(true);
                    prevWp->f_altitude->setValue(newWps[j].altitude());
                    prevWp->f_amsl->setValue(prevAmsl);
                    jsa.removeLast();
                    jsa.append(prevWp->toJson());
                    continue;
                }
            }
            // Append new point
            jso[wp->f_amsl->name()] = true;
            jso[wp->f_altitude->name()] = static_cast<int>(newWps[j].altitude());
            jso["lat"] = newWps[j].latitude();
            jso["lon"] = newWps[j].longitude();
            jso[wp->f_atrack->name()] = wp->f_atrack->value().toBool();
            jso[wp->f_xtrack->name()] = wp->f_xtrack->value().toBool();
            jsa.append(jso);
        }
        // Append current waypoint
        jsa.append(wp->toJson());
    }

    // the waypoints are re-created: their terrain is attached when all of them are loaded
    m_rebuilding = true;
    m->f_wp->fromJson(jsa);
    m_rebuilding = false;
    changeExternalsVisibility();

    auto lastTerrain = m->f_wp->size() > 0
                           ? WaypointTerrain::of(static_cast<MissionItem *>(m->f_wp->facts().last()))
                           : nullptr;
    if (lastTerrain) {
        connect(lastTerrain,
                &WaypointTerrain::terrainProfileChanged,
                this,
                &ElevationMap::completeCorrection);
        return;
    }
    apxMsg() << tr("Mission correction completed");
    m_isCorrect = false;
}

void ElevationMap::completeCorrection()
{
    if (!m_isCorrect)
        return;

    auto wps = mission()->f_wp;
    auto lastTerrain = wps->size() > 0
                           ? WaypointTerrain::of(static_cast<MissionItem *>(wps->facts().last()))
                           : nullptr;
    if (!lastTerrain)
        return;

    // Terrain profile not empty and waypoint have elevation
    // (elevation map for waypoint exists)
    if (lastTerrain->terrainProfile().empty() && !std::isnan(lastTerrain->elevation()))
        return;

    apxMsg() << tr("Mission correction completed");
    m_isCorrect = false;
    QTimer::singleShot(1000, this, &ElevationMap::checkCorrectionResult);
}

void ElevationMap::checkCorrectionResult()
{
    auto m = mission();
    QString wpWarnings;
    for (int i = 0; i < m->f_wp->size(); ++i) {
        auto t = WaypointTerrain::of(static_cast<MissionItem *>(m->f_wp->child(i)));
        if (!t || !t->collision())
            continue;
        if (!wpWarnings.isEmpty())
            wpWarnings += ",";
        wpWarnings += QString::number(i + 1);
    }
    if (wpWarnings.isEmpty())
        return;
    if (++m_correctionPass < CORRECTION_PASSES) {
        startPathsCorrection();
        return;
    }
    apxMsgW() << tr("The path of points %1 has been changed or could not be corrected. "
                    "Check  these points and try again")
                     .arg(wpWarnings);
}

void ElevationMap::createDir(const QString &path)
{
    QDir dir(path);
    if (dir.exists())
        return;
    if (!AppDirs::db().mkdir(dir.dirName()))
        apxMsgW() << tr("Failed to create default elevation dir");
}
