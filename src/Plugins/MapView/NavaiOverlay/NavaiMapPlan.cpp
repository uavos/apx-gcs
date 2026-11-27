#include "NavaiMapPlan.h"

#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <Mission/MissionMapItemsModel.h>
#include <Mission/MissionPoint.h>

#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <algorithm>
#include <cmath>

namespace {

// The cameras NAVAI flies with and their horizontal field of view, the same
// list as in navai's map/main.py. The last one takes the value typed in.
struct Camera
{
    const char *name;
    double fovDeg;
};
const Camera kCameras[] = {
    {"Basler acA2040 + 8.5 mm (66.9°)", 66.9},
    {"GST PLUG1212 thermal + 19 mm (44.0°)", 44.0},
    {"GST PLUG1212 thermal + 25 mm (34.2°)", 34.2},
    {"Manual FOV", 0.0},
};

// A ready map tile on the board: the 1024 px picture, about 1.8 MB, and its
// SuperPoint features, about 1.9 MB. Measured on a 254 tile map.
constexpr double kTileMb = 3.7;

// A plan above this is a mistake in the zone, not a map.
constexpr qint64 kMaxTiles = 1000000;

// Flight altitudes from "250, 500, 1000": positive, sorted, without repeats.
QList<double> parseScales(const QString &text, bool *ok)
{
    QList<double> scales;
    *ok = false;
    for (const QString &item : text.split(',', Qt::SkipEmptyParts)) {
        if (item.trimmed().isEmpty())
            continue;
        bool isNumber = false;
        const double scale = item.trimmed().toDouble(&isNumber);
        if (!isNumber || !std::isfinite(scale) || scale <= 0.0)
            return {};
        if (!scales.contains(scale))
            scales.append(scale);
    }
    std::sort(scales.begin(), scales.end());
    *ok = !scales.isEmpty();
    return scales;
}

QString sizeText(double mb)
{
    if (mb < 1024.0)
        return QString("%1 MB").arg(mb, 0, 'f', 0);
    return QString("%1 GB").arg(mb / 1024.0, 0, 'f', 1);
}

} // namespace

NavaiMapPlan::NavaiMapPlan(Fact *parent)
    : Fact(parent,
           "plan",
           tr("Map download"),
           tr("Zone and settings of the map to build on board"),
           Group,
           "map-plus")
{
    f_scales = new Fact(this,
                        "scales",
                        tr("Scales"),
                        tr("Flight altitudes, comma separated"),
                        Text | PersistentValue);
    f_scales->setUnits("m");
    f_scales->setDefaultValue("250, 500, 1000");

    f_camera = new Fact(this,
                        "camera",
                        tr("Camera"),
                        tr("Camera and lens"),
                        Enum | PersistentValue);
    QStringList cameras;
    for (const Camera &camera : kCameras)
        cameras << QString::fromUtf8(camera.name);
    f_camera->setEnumStrings(cameras);
    f_camera->setDefaultValue(cameras.first());

    f_fov = new Fact(this,
                     "fov",
                     tr("Camera FOV"),
                     tr("Horizontal field of view"),
                     Float | PersistentValue);
    f_fov->setUnits("deg");
    f_fov->setMin(1.0);
    f_fov->setMax(170.0);
    f_fov->setPrecision(1);
    f_fov->setDefaultValue(kCameras[0].fovDeg);

    f_overlap = new Fact(this,
                         "overlap",
                         tr("Overlap"),
                         tr("Share of a tile covered by the next one"),
                         Float | PersistentValue);
    f_overlap->setMin(0.0);
    f_overlap->setMax(0.95);
    f_overlap->setIncrement(0.05);
    f_overlap->setPrecision(2);
    f_overlap->setDefaultValue(0.5);

    f_points = new Fact(this, "points", tr("Zone"), tr("Zone vertices"), Group | Count);
    m_pointsModel = new MissionMapItemsModel(f_points);

    f_zone = new Fact(this,
                      "zone",
                      tr("New zone"),
                      tr("Put a zone in the middle of the map"),
                      Action | CloseOnTrigger,
                      "vector-polygon");
    connect(f_zone, &Fact::triggered, this, &NavaiMapPlan::zoneRequested);

    f_remove = new Fact(this, "remove", tr("Remove zone"), "", Action | Remove);
    connect(f_remove, &Fact::triggered, this, [this]() { setZone({}); });

    f_save = new Fact(this,
                      "save",
                      tr("Download"),
                      tr("Save the map plan to a file for the board"),
                      Action | Apply,
                      "download");
    connect(f_save, &Fact::triggered, this, &NavaiMapPlan::save, Qt::QueuedConnection);

    connect(f_camera, &Fact::valueChanged, this, &NavaiMapPlan::updateCamera);
    for (Fact *f : {f_scales, f_fov, f_overlap})
        connect(f, &Fact::valueChanged, this, &NavaiMapPlan::updatePlan);

    updateCamera();
    updatePlan();
}

QString NavaiMapPlan::scaleColor(int index)
{
    static const QStringList colors = {
        "#e11d48",
        "#f59e0b",
        "#16a34a",
        "#0891b2",
        "#2563eb",
        "#7c3aed",
    };
    return colors[qMax(0, index) % colors.size()];
}

void NavaiMapPlan::updateCamera()
{
    // a camera from the list sets its own field of view
    double fov = 0.0;
    for (const Camera &camera : kCameras) {
        if (f_camera->text() == QString::fromUtf8(camera.name))
            fov = camera.fovDeg;
    }
    f_fov->setEnabled(fov <= 0.0);
    if (fov > 0.0)
        f_fov->setValue(fov);
}

void NavaiMapPlan::setZone(const QVariantList &coordinates)
{
    f_points->deleteChildren();
    for (const QVariant &v : coordinates) {
        const QGeoCoordinate c = v.value<QGeoCoordinate>();
        if (c.isValid())
            addPoint(c);
    }
    updatePolygon();
}

void NavaiMapPlan::addPoint(QGeoCoordinate c, int n)
{
    auto pt = new MissionPoint(f_points, tr("Zone vertex"), c);
    if (n >= 0)
        pt->move(n, false);

    connect(pt, &MissionPoint::coordinateChanged, this, &NavaiMapPlan::updatePolygon);
    updatePolygon();
}

void NavaiMapPlan::removePoint(int n)
{
    if (n < 0 || n >= f_points->size() || f_points->size() <= 3)
        return;

    f_points->child(n)->deleteFact();
    updatePolygon();
}

void NavaiMapPlan::updatePolygon()
{
    QGeoPolygon p;
    for (auto i : f_points->facts()) {
        auto pt = qobject_cast<MissionPoint *>(i);
        if (pt)
            p.addCoordinate(pt->coordinate());
    }
    m_polygon = p;
    emit polygonChanged();
    updatePlan();
}

QGeoCoordinate NavaiMapPlan::labelCoordinate() const
{
    const QList<QGeoCoordinate> zone = m_polygon.perimeter();
    if (zone.isEmpty())
        return QGeoCoordinate();

    // the longitudes are taken from the first vertex, so a zone across the
    // 180th meridian keeps its east side
    const double lon0 = zone.first().longitude();
    double south = zone.first().latitude();
    double east = 0.0;
    for (const QGeoCoordinate &c : zone) {
        south = qMin(south, c.latitude());
        east = qMax(east, std::remainder(c.longitude() - lon0, 360.0));
    }
    return QGeoCoordinate(south, std::remainder(lon0 + east, 360.0));
}

void NavaiMapPlan::updatePlan()
{
    _scales.clear();
    _tilesCount = 0;

    const QList<QGeoCoordinate> zone = m_polygon.perimeter();
    bool ok = false;
    const QList<double> altitudes = parseScales(f_scales->value().toString(), &ok);

    QStringList lines;
    if (!ok) {
        lines << tr("Scales: invalid value");
    } else if (zone.size() >= 3) {
        lines << tr("Zone: %1 km²").arg(NavaiMapGrid::areaM2(zone) / 1e6, 0, 'f', 2);
        for (double altitude : altitudes) {
            const NavaiScalePlan scale = NavaiMapGrid::plan(zone,
                                                            altitude,
                                                            f_fov->value().toDouble(),
                                                            f_overlap->value().toDouble());
            lines << QString("<font color=\"%1\">■</font> ").arg(scaleColor(_scales.size()))
                         + tr("%1 m: tile %2 m, %3 tiles")
                               .arg(altitude)
                               .arg(scale.footprintM, 0, 'f', 0)
                               .arg(scale.tilesCount);
            _tilesCount += scale.tilesCount;
            _scales.append(scale);
        }
        lines << tr("Total: %1 tiles, about %2")
                     .arg(_tilesCount)
                     .arg(sizeText(_tilesCount * kTileMb));
    }

    const QString summary = lines.join("<br>");
    if (m_summary != summary) {
        m_summary = summary;
        emit summaryChanged();
    }

    f_remove->setEnabled(!zone.isEmpty());
    f_save->setEnabled(_tilesCount > 0);
}

QJsonObject NavaiMapPlan::toPlanJson() const
{
    const QList<QGeoCoordinate> zone = m_polygon.perimeter();

    QJsonArray jsZone;
    for (const QGeoCoordinate &c : zone)
        jsZone.append(QJsonObject{{"latitude", c.latitude()}, {"longitude", c.longitude()}});

    // every scale lists its tile centres with the keys of navai's own map
    // manifest, so the board only has to download them
    QJsonArray jsScales;
    for (const NavaiScalePlan &scale : _scales) {
        QJsonArray jsTiles;
        for (const NavaiGridSpan &span : scale.spans) {
            for (int x = span.x0; x <= span.x1; ++x) {
                double latitude = 0.0;
                double longitude = 0.0;
                NavaiMapGrid::tileCenter(zone.first(),
                                         scale.strideM,
                                         x,
                                         span.y,
                                         &latitude,
                                         &longitude);
                jsTiles.append(QJsonObject{
                    {"latitude", latitude},
                    {"longitude", longitude},
                    {"grid_x2", 2 * x},
                    {"grid_y2", 2 * span.y},
                });
            }
        }
        jsScales.append(QJsonObject{
            {"altitude_m", scale.altitudeM},
            {"footprint_m", scale.footprintM},
            {"overlap", scale.overlap},
            {"grid_step_m", scale.strideM},
            {"tiles_count", scale.tilesCount},
            {"tiles", jsTiles},
        });
    }

    return QJsonObject{
        {"type", "navai_map_plan"},
        {"version", 1},
        {"created", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"mode", "zone_grid"},
        {"zone", jsZone},
        {"camera", f_camera->text()},
        {"fov_deg", f_fov->value().toDouble()},
        {"overlap", f_overlap->value().toDouble()},
        {"tiles_count", _tilesCount},
        {"estimated_size_mb", std::round(_tilesCount * kTileMb)},
        {"scales", jsScales},
    };
}

void NavaiMapPlan::save()
{
    if (_tilesCount <= 0) {
        apxMsgW() << tr("Map plan has no tiles: draw a zone and check the scales");
        return;
    }
    if (_tilesCount > kMaxTiles) {
        apxMsgW() << tr("Map plan is too large: %1 tiles, the limit is %2")
                         .arg(_tilesCount)
                         .arg(kMaxTiles);
        return;
    }

    QDir dir(AppDirs::user().absoluteFilePath("Navai"));
    if (!dir.exists())
        dir.mkpath(".");

    QFileDialog dlg(nullptr, f_save->descr(), dir.absolutePath());
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setOption(QFileDialog::DontConfirmOverwrite, false);
    dlg.setViewMode(QFileDialog::Detail);
    dlg.setNameFilters({tr("Map plan") + " (*.json)", tr("Any files") + " (*)"});
    dlg.setDefaultSuffix("json");
    dlg.selectFile(QString("navai-map-%1.json")
                       .arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmm")));
    if (!dlg.exec() || dlg.selectedFiles().size() != 1)
        return;

    const QString fileName = dlg.selectedFiles().first();
    const QByteArray data = QJsonDocument(toPlanJson()).toJson(QJsonDocument::Compact);

    QSaveFile file(fileName);
    if (!file.open(QFile::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        apxMsgW() << tr("Cannot write file").append(':') << fileName << file.errorString();
        return;
    }
    apxMsg() << tr("Map plan saved").append(':') << fileName;
}
