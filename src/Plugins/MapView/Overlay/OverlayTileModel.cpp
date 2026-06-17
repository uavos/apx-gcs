#include "OverlayTileModel.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QStandardPaths>
#include <QUrl>
#include <QtMath>

#include <cmath>

static constexpr double PI_VALUE = 3.14159265358979323846;

OverlayTileWorker::OverlayTileWorker(QObject *parent)
    : QObject(parent)
{
}

void OverlayTileWorker::resetSession(
    const QString &sessionDir,
    int nativeZoom,
    int minZoom,
    int tileSize)
{
    flush();

    _sessionDir = sessionDir;
    _nativeZoom = nativeZoom;
    _minZoom = minZoom;
    _tileSize = tileSize;

    _tileCache.clear();
    _dirtyTiles.clear();

    QDir().mkpath(_sessionDir);
}

void OverlayTileWorker::drawSample(
    double lat,
    double lon,
    double altitude,
    const QString &source)
{
    drawFootprint(
        lat,
        lon,
        altitude,
        source
    );

    maybeFlush();
}

void OverlayTileWorker::drawSegment(
    double lat1,
    double lon1,
    double lat2,
    double lon2,
    double altitude,
    const QString &source)
{
    Q_UNUSED(altitude)

    if (source != "CHARGE" &&
        source != "NEUTRAL" &&
        source != "ENGINE")
        return;

    const QGeoCoordinate a(lat1, lon1);
    const QGeoCoordinate b(lat2, lon2);

    const double distance =
        a.distanceTo(b);

    if (distance > 1000.0) {
        drawFootprint(
            lat2,
            lon2,
            0.0,
            source
        );

        maybeFlush();
        return;
    }

    const double step = 8.0;

    const int steps =
        qMax(
            1,
            static_cast<int>(
                std::ceil(distance / step)
            )
        );

    const double azimuth =
        a.azimuthTo(b);

    for (int i = 0; i <= steps; ++i) {
        const double d =
            distance *
            static_cast<double>(i) /
            static_cast<double>(steps);

        const QGeoCoordinate p =
            a.atDistanceAndAzimuth(d, azimuth);

        drawFootprint(
            p.latitude(),
            p.longitude(),
            0.0,
            source
        );
    }

    maybeFlush();
}

double OverlayTileWorker::footprintMeters(double altitude) const
{
    Q_UNUSED(altitude)

    return 20.0;
}

void OverlayTileWorker::drawFootprint(
    double lat,
    double lon,
    double altitude,
    const QString &source)
{
    if (_sessionDir.isEmpty())
        return;

    if (source != "CHARGE" &&
        source != "NEUTRAL" &&
        source != "ENGINE")
        return;

    if (lat == 0.0 || lon == 0.0)
        return;

    QColor color(
        colorForSource(source)
    );

    color.setAlpha(150); 

    const double mpp =
        metersPerPixel(lat, _nativeZoom);

    if (mpp <= 0.0)
        return;

    const double fpPixels =
        qMax(
            2.0,
            footprintMeters(altitude) / mpp
        );

    const double cx =
        lonToGlobalPixelX(lon, _nativeZoom);

    const double cy =
        latToGlobalPixelY(lat, _nativeZoom);

    const QRectF globalRect(
        cx - fpPixels / 2.0,
        cy - fpPixels / 2.0,
        fpPixels,
        fpPixels
    );

    const int minTileX =
        static_cast<int>(
            std::floor(globalRect.left() / _tileSize)
        );

    const int maxTileX =
        static_cast<int>(
            std::floor(globalRect.right() / _tileSize)
        );

    const int minTileY =
        static_cast<int>(
            std::floor(globalRect.top() / _tileSize)
        );

    const int maxTileY =
        static_cast<int>(
            std::floor(globalRect.bottom() / _tileSize)
        );

    for (int ty = minTileY; ty <= maxTileY; ++ty) {
        for (int tx = minTileX; tx <= maxTileX; ++tx) {
            drawFootprintIntoTile(
                _nativeZoom,
                tx,
                ty,
                globalRect,
                color,
                source
            );
        }
    }
}

void OverlayTileWorker::drawFootprintIntoTile(
    int z,
    int x,
    int y,
    const QRectF &globalRect,
    const QColor &color,
    const QString &source)
{
    if (x < 0 || y < 0)
        return;

    const int maxIndex =
        static_cast<int>(
            qPow(2.0, z)
        ) - 1;

    if (x > maxIndex || y > maxIndex)
        return;

    const QRectF tileGlobalRect(
        x * _tileSize,
        y * _tileSize,
        _tileSize,
        _tileSize
    );

    const QRectF clipped =
        globalRect.intersected(tileGlobalRect);

    if (clipped.isEmpty())
        return;

    const QString key =
        tileKey(z, x, y);

    const QString path =
        tilePath(z, x, y, true);

    QImage img;

    if (_tileCache.contains(key)) {
        img = _tileCache.value(key);
    } else if (QFile::exists(path)) {
        img.load(path);
    }

    if (img.isNull() ||
        img.width() != _tileSize ||
        img.height() != _tileSize) {

        img =
            QImage(
                _tileSize,
                _tileSize,
                QImage::Format_ARGB32_Premultiplied
            );

        img.fill(Qt::transparent);
    }

    QRectF localRect =
        clipped.translated(
            -tileGlobalRect.left(),
            -tileGlobalRect.top()
        );

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(localRect, color);
    p.end();

    _tileCache.insert(key, img);

    DirtyTile dirty;
    dirty.z = z;
    dirty.x = x;
    dirty.y = y;
    dirty.source = source;
    dirty.color = color.name(QColor::HexRgb);
    dirty.path = path;

    _dirtyTiles.insert(key, dirty);
}

void OverlayTileWorker::maybeFlush()
{
    const qint64 now =
        QDateTime::currentMSecsSinceEpoch();

    if (_dirtyTiles.size() >= _maxDirtyBeforeFlush ||
        now - _lastFlushMs >= _flushIntervalMs) {
        flush();
    }
}

void OverlayTileWorker::flush()
{
    if (_dirtyTiles.isEmpty())
        return;

    const auto dirtyCopy =
        _dirtyTiles;

    _dirtyTiles.clear();

    for (auto it = dirtyCopy.constBegin();
         it != dirtyCopy.constEnd();
         ++it) {

        const QString key =
            it.key();

        const DirtyTile dirty =
            it.value();

        if (!_tileCache.contains(key))
            continue;

        QImage img =
            _tileCache.value(key);

        img.save(dirty.path, "PNG");

        emit tileUpdated(
            dirty.z,
            dirty.x,
            dirty.y,
            dirty.source,
            dirty.color,
            dirty.path
        );
    }

    QSet<QString> rebuilt;

    for (auto it = dirtyCopy.constBegin();
         it != dirtyCopy.constEnd();
         ++it) {

        const DirtyTile dirty =
            it.value();

        buildPyramidFromChild(
            dirty.z,
            dirty.x,
            dirty.y,
            &rebuilt
        );
    }

    _lastFlushMs =
        QDateTime::currentMSecsSinceEpoch();

    trimCache();
}

void OverlayTileWorker::buildPyramidFromChild(
    int childZ,
    int childX,
    int childY,
    QSet<QString> *rebuilt)
{
    const int parentZ =
        childZ - 1;

    if (parentZ < _minZoom)
        return;

    const int parentX =
        childX / 2;

    const int parentY =
        childY / 2;

    buildParentTile(
        parentZ,
        parentX,
        parentY,
        rebuilt
    );

    buildPyramidFromChild(
        parentZ,
        parentX,
        parentY,
        rebuilt
    );
}

void OverlayTileWorker::buildParentTile(
    int z,
    int x,
    int y,
    QSet<QString> *rebuilt)
{
    const QString key =
        tileKey(z, x, y);

    if (rebuilt->contains(key))
        return;

    rebuilt->insert(key);

    QImage parent(
        _tileSize,
        _tileSize,
        QImage::Format_ARGB32_Premultiplied
    );

    parent.fill(Qt::transparent);

    bool hasAnyChild = false;

    QPainter p(&parent);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    const int childZ =
        z + 1;

    for (int dy = 0; dy < 2; ++dy) {
        for (int dx = 0; dx < 2; ++dx) {
            const int childX =
                x * 2 + dx;

            const int childY =
                y * 2 + dy;

            const QString childKey =
                tileKey(
                    childZ,
                    childX,
                    childY
                );

            const QString childPath =
                tilePath(
                    childZ,
                    childX,
                    childY,
                    false
                );

            QImage child;

            if (_tileCache.contains(childKey)) {
                child = _tileCache.value(childKey);
            } else if (QFile::exists(childPath)) {
                child.load(childPath);
            }

            if (child.isNull())
                continue;

            hasAnyChild = true;

            const QRect target(
                dx * (_tileSize / 2),
                dy * (_tileSize / 2),
                _tileSize / 2,
                _tileSize / 2
            );

            p.drawImage(
                target,
                child
            );
        }
    }

    p.end();

    if (!hasAnyChild)
        return;

    const QString path =
        tilePath(z, x, y, true);

    parent.save(path, "PNG");

    _tileCache.insert(key, parent);

    emit tileUpdated(
        z,
        x,
        y,
        "PYRAMID",
        "#FFFFFF",
        path
    );
}

QString OverlayTileWorker::tileKey(
    int z,
    int x,
    int y)
{
    return
        QString("%1/%2/%3")
            .arg(z)
            .arg(x)
            .arg(y);
}

void OverlayTileWorker::trimCache()
{
    if (_tileCache.size() <= _maxCachedTiles)
        return;

    auto it =
        _tileCache.begin();

    while (it != _tileCache.end() &&
           _tileCache.size() > _maxCachedTiles) {

        if (_dirtyTiles.contains(it.key())) {
            ++it;
            continue;
        }

        it = _tileCache.erase(it);
    }
}

double OverlayTileWorker::lonToGlobalPixelX(
    double lon,
    int z)
{
    const double n =
        qPow(2.0, z) * 256.0;

    return
        (lon + 180.0) / 360.0 * n;
}

double OverlayTileWorker::latToGlobalPixelY(
    double lat,
    int z)
{
    lat =
        qBound(
            -85.05112878,
            lat,
            85.05112878
        );

    const double sinLat =
        std::sin(qDegreesToRadians(lat));

    const double n =
        qPow(2.0, z) * 256.0;

    return
        (
            0.5 -
            std::log(
                (1.0 + sinLat) /
                (1.0 - sinLat)
            ) /
            (4.0 * PI_VALUE)
        ) * n;
}

double OverlayTileWorker::metersPerPixel(
    double lat,
    int z)
{
    return
        156543.03392 *
        std::cos(qDegreesToRadians(lat)) /
        qPow(2.0, z);
}

QString OverlayTileWorker::colorForSource(
    const QString &source) const
{
    if (source == "CHARGE")
        return "#00FF00";

    if (source == "ENGINE")
        return "#FF0000";

    return "#FFFF00";
}

QString OverlayTileWorker::tilePath(
    int z,
    int x,
    int y,
    bool createDir) const
{
    const QString dir =
        _sessionDir +
        QString("/z%1/%2")
            .arg(z)
            .arg(x);

    if (createDir)
        QDir().mkpath(dir);

    return
        dir +
        QString("/%1.png")
            .arg(y);
}

// ======================= MODEL =======================

OverlayTileModel::OverlayTileModel(QObject *parent)
    : QAbstractListModel(parent)
{
    _worker =
        new OverlayTileWorker();

    _worker->moveToThread(
        &_workerThread
    );

    connect(
        &_workerThread,
        &QThread::finished,
        _worker,
        &QObject::deleteLater
    );

    connect(
        this,
        &OverlayTileModel::workerResetSession,
        _worker,
        &OverlayTileWorker::resetSession,
        Qt::QueuedConnection
    );

    connect(
        this,
        &OverlayTileModel::workerDrawSample,
        _worker,
        &OverlayTileWorker::drawSample,
        Qt::QueuedConnection
    );

    connect(
        this,
        &OverlayTileModel::workerDrawSegment,
        _worker,
        &OverlayTileWorker::drawSegment,
        Qt::QueuedConnection
    );

    connect(
        _worker,
        &OverlayTileWorker::tileUpdated,
        this,
        &OverlayTileModel::onWorkerTileUpdated,
        Qt::QueuedConnection
    );

    _workerThread.start();

    startNewSession();
}

OverlayTileModel::~OverlayTileModel()
{
    if (_worker && _workerThread.isRunning()) {
        QMetaObject::invokeMethod(
            _worker,
            "flush",
            Qt::BlockingQueuedConnection
        );
    }

    _workerThread.quit();
    _workerThread.wait();
}

int OverlayTileModel::rowCount(
    const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return _visibleKeys.size();
}

QVariant OverlayTileModel::data(
    const QModelIndex &index,
    int role) const
{
    if (!index.isValid())
        return {};

    const int row =
        index.row();

    if (row < 0 || row >= _visibleKeys.size())
        return {};

    const QString key =
        _visibleKeys.at(row);

    if (!_tiles.contains(key))
        return {};

    const OverlayTile &t =
        _tiles[key];

    switch (role) {
    case ZRole: return t.z;
    case XRole: return t.x;
    case YRole: return t.y;
    case NorthRole: return t.north;
    case SouthRole: return t.south;
    case WestRole: return t.west;
    case EastRole: return t.east;
    case SourceRole: return t.source;
    case ColorRole: return t.color;
    case TimeRole: return t.time;
    case TileUrlRole: return t.tileUrl;
    case RevisionRole: return t.revision;
    default: return {};
    }
}

QHash<int, QByteArray> OverlayTileModel::roleNames() const
{
    return {
        {ZRole, "z"},
        {XRole, "x"},
        {YRole, "y"},
        {NorthRole, "north"},
        {SouthRole, "south"},
        {WestRole, "west"},
        {EastRole, "east"},
        {SourceRole, "source"},
        {ColorRole, "tileColor"},
        {TimeRole, "time"},
        {TileUrlRole, "tileUrl"},
        {RevisionRole, "revision"}
    };
}

void OverlayTileModel::setDisplayZoom(int zoom)
{
    const int clamped =
        qBound(
            _minZoom,
            zoom,
            _nativeZoom
        );

    if (_displayZoom == clamped)
        return;

    beginResetModel();

    _displayZoom = clamped;
    rebuildVisible();

    endResetModel();

    emit displayZoomChanged();
}

void OverlayTileModel::rebuildVisible()
{
    _visibleKeys.clear();
    _visibleRowByKey.clear();

    for (auto it = _tiles.constBegin();
         it != _tiles.constEnd();
         ++it) {

        if (it.value().z != _displayZoom)
            continue;

        const int row =
            _visibleKeys.size();

        _visibleKeys.push_back(it.key());
        _visibleRowByKey.insert(it.key(), row);
    }
}

void OverlayTileModel::clear()
{
    beginResetModel();

    _tiles.clear();
    _visibleKeys.clear();
    _visibleRowByKey.clear();

    endResetModel();
}

void OverlayTileModel::startNewSession()
{
    beginResetModel();

    _tiles.clear();
    _visibleKeys.clear();
    _visibleRowByKey.clear();

    _sessionId =
        QDateTime::currentDateTimeUtc()
            .toString("yyyyMMdd_HHmmss");

    _displayZoom = _nativeZoom;

    QDir().mkpath(sessionDir());

    writeManifest();

    endResetModel();

    emit displayZoomChanged();

    emit workerResetSession(
        sessionDir(),
        _nativeZoom,
        _minZoom,
        _tileSize
    );
}

void OverlayTileModel::flush()
{
    if (!_worker)
        return;

    QMetaObject::invokeMethod(
        _worker,
        "flush",
        Qt::QueuedConnection
    );
}

void OverlayTileModel::addSample(
    double lat,
    double lon,
    double altitude,
    const QString &source)
{
    emit workerDrawSample(
        lat,
        lon,
        altitude,
        source
    );
}

void OverlayTileModel::addSegment(
    double lat1,
    double lon1,
    double lat2,
    double lon2,
    double altitude,
    const QString &source)
{
    emit workerDrawSegment(
        lat1,
        lon1,
        lat2,
        lon2,
        altitude,
        source
    );
}

void OverlayTileModel::onWorkerTileUpdated(
    int z,
    int x,
    int y,
    const QString &source,
    const QString &color,
    const QString &path)
{
    if (!path.startsWith(sessionDir()))
        return;

    const QString key =
        tileKey(z, x, y);

    OverlayTile t;

    const bool existed =
        _tiles.contains(key);

    if (existed)
        t = _tiles.value(key);

    t.z = z;
    t.x = x;
    t.y = y;

    t.north = tileYToLat(y, z);
    t.south = tileYToLat(y + 1, z);
    t.west = tileXToLon(x, z);
    t.east = tileXToLon(x + 1, z);

    t.source = source;
    t.color = color;
    t.time =
        QDateTime::currentDateTimeUtc()
            .toString(Qt::ISODate);

    t.tileUrl =
        QUrl::fromLocalFile(path).toString();

    if (existed)
        t.revision++;
    else
        t.revision = 0;

    _tiles.insert(key, t);

    if (z != _displayZoom)
        return;

    if (_visibleRowByKey.contains(key)) {
        const int row =
            _visibleRowByKey.value(key);

        emit dataChanged(
            index(row, 0),
            index(row, 0),
            {
                SourceRole,
                ColorRole,
                TimeRole,
                TileUrlRole,
                RevisionRole
            }
        );

        return;
    }

    const int row =
        _visibleKeys.size();

    beginInsertRows(QModelIndex(), row, row);

    _visibleKeys.push_back(key);
    _visibleRowByKey.insert(key, row);

    endInsertRows();
}

QString OverlayTileModel::tileKey(
    int z,
    int x,
    int y)
{
    return
        QString("%1/%2/%3")
            .arg(z)
            .arg(x)
            .arg(y);
}

double OverlayTileModel::tileXToLon(
    int x,
    int z)
{
    const double n =
        qPow(2.0, z);

    return
        x / n * 360.0 - 180.0;
}

double OverlayTileModel::tileYToLat(
    int y,
    int z)
{
    const double n =
        qPow(2.0, z);

    const double rad =
        std::atan(
            std::sinh(
                PI_VALUE *
                (1.0 - 2.0 * y / n)
            )
        );

    return qRadiansToDegrees(rad);
}

QString OverlayTileModel::storageRoot() const
{
    const QString docs =
        QStandardPaths::writableLocation(
            QStandardPaths::DocumentsLocation
        );

    return docs + "/UAVOS/Storage/overlays";
}

QString OverlayTileModel::sessionDir() const
{
    return
        storageRoot() +
        "/overlay/" +
        _sessionId;
}

void OverlayTileModel::writeManifest() const
{
    QDir().mkpath(sessionDir());

    QJsonObject root;

    root["type"] = "overlay";
    root["session"] = _sessionId;

    root["createdUtc"] =
        QDateTime::currentDateTimeUtc()
            .toString(Qt::ISODate);

    root["algorithm"] =
        "est.pos.vspeed - est.air.vse";

    root["nativeZoom"] = _nativeZoom;
    root["minZoom"] = _minZoom;
    root["tileSize"] = _tileSize;
    root["format"] = "png";
    root["layout"] = "z/x/y.png";
    root["pyramid"] = true;
    root["threaded"] = true;
    root["opacityInQml"] = 0.4;
    root["footprintMeters"] = 20.0;
    root["sampleStepMeters"] = 8.0;

    QFile f(
        sessionDir() + "/manifest.json"
    );

    if (!f.open(
            QIODevice::WriteOnly |
            QIODevice::Truncate))
        return;

    f.write(
        QJsonDocument(root)
            .toJson(QJsonDocument::Indented)
    );

    f.close();
}
