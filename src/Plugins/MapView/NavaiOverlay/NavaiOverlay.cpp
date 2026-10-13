#include "NavaiOverlay.h"
#include "NavaiHeatmap.h"

#include <App/AppNotify.h>
#include <Fleet/Fleet.h>
#include <Fleet/Unit.h>

#include <QCoreApplication>
#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QQmlEngine>
#include <QStandardPaths>

#include <cmath>
#include <limits>

namespace {

QString cleanId(QString id)
{
    id = id.trimmed();

    while (id.contains(QStringLiteral(".."))) {
        id.replace(QStringLiteral(".."), QStringLiteral("."));
    }

    if (id.startsWith(QLatin1Char('.')))
        id.remove(0, 1);

    if (id.endsWith(QLatin1Char('.')))
        id.chop(1);

    return id;
}

void insertFact(
    QHash<QString, Fact *> &facts,
    const QString &id,
    Fact *fact)
{
    const QString key =
        cleanId(id);

    if (!fact || key.isEmpty())
        return;

    facts.insert(key, fact);
}

void collectFactTree(
    QHash<QString, Fact *> &facts,
    Fact *fact,
    const QString &prefix = QString(),
    int depth = 0)
{
    if (!fact || depth > 64)
        return;

    const QString name =
        fact->name().trimmed();

    const QString id =
        name.isEmpty()
            ? cleanId(prefix)
            : cleanId(prefix.isEmpty()
                          ? name
                          : prefix + QLatin1Char('.') + name);

    const auto children =
        fact->facts();

    if (children.isEmpty()) {
        QString key =
            id;

        if (key.isEmpty())
            key = cleanId(fact->titlePath(QLatin1Char('.')));

        insertFact(
            facts,
            key,
            fact
        );

        return;
    }

    for (Fact *child : children) {
        collectFactTree(
            facts,
            child,
            id,
            depth + 1
        );
    }
}

Fact *factByAnyId(
    const QHash<QString, Fact *> &facts,
    const QStringList &ids,
    QString *matchedId = nullptr)
{
    for (const QString &id : ids) {
        const QString key =
            cleanId(id);

        if (facts.contains(key)) {
            if (matchedId)
                *matchedId = key;

            return facts.value(key);
        }
    }

    for (const QString &id : ids) {
        const QString key =
            cleanId(id);

        const QString suffix =
            QStringLiteral(".") + key;

        for (auto it = facts.constBegin(); it != facts.constEnd(); ++it) {
            if (it.key().endsWith(suffix)) {
                if (matchedId)
                    *matchedId = it.key();

                return it.value();
            }
        }
    }

    return nullptr;
}

} // namespace

// ======================= MODEL =======================

NavaiResultModel::NavaiResultModel(QObject *parent)
    : QAbstractListModel(parent)
{
    _fadeTimer.setInterval(50);
    _fadeTimer.setTimerType(Qt::CoarseTimer);

    connect(
        &_fadeTimer,
        &QTimer::timeout,
        this,
        &NavaiResultModel::updateFade
    );
}

int NavaiResultModel::rowCount(
    const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return _items.size();
}

QVariant NavaiResultModel::data(
    const QModelIndex &index,
    int role) const
{
    if (!index.isValid())
        return {};

    const int row =
        index.row();

    if (row < 0 || row >= _items.size())
        return {};

    const Result &item =
        _items.at(row);

    switch (role) {
    case LatitudeRole:
        return item.lat;

    case LongitudeRole:
        return item.lon;

    case PercentRole:
        return item.percent;

    case LabelRole:
        return item.label;

    case ItemOpacityRole:
        return item.opacity;

    case TrajectoryCoordinatesRole:
        return item.trajectoryCoordinates;

    case LatestRole:
        return row == _items.size() - 1;

    default:
        return {};
    }
}

QHash<int, QByteArray> NavaiResultModel::roleNames() const
{
    return {
        {LatitudeRole, "latitude"},
        {LongitudeRole, "longitude"},
        {PercentRole, "percent"},
        {LabelRole, "label"},
        {ItemOpacityRole, "itemOpacity"},
        {TrajectoryCoordinatesRole, "trajectoryCoordinates"},
        {LatestRole, "latest"}
    };
}

void NavaiResultModel::addResult(
    double lat,
    double lon,
    double percent,
    const QString &label,
    const QVariantList &trajectoryCoordinates)
{
    for (Result &oldItem : _items)
        oldItem.targetOpacity = 0.35;

    Result item;
    item.lat = lat;
    item.lon = lon;
    item.percent = percent;
    item.opacity = 0.0;
    item.targetOpacity = 1.0;
    item.label = label;
    item.trajectoryCoordinates = trajectoryCoordinates;

    const int row =
        _items.size();

    beginInsertRows(QModelIndex(), row, row);
    _items.push_back(item);
    endInsertRows();

    if (row > 0) {
        emit dataChanged(
            index(row - 1, 0),
            index(row - 1, 0),
            {LatestRole}
        );
    }

    startFadeTimer();
}

void NavaiResultModel::clear()
{
    beginResetModel();
    _items.clear();
    endResetModel();

    _fadeTimer.stop();
    _lastFadeMs = 0;
}

void NavaiResultModel::startFadeTimer()
{
    _lastFadeMs =
        QDateTime::currentMSecsSinceEpoch();

    if (!_fadeTimer.isActive())
        _fadeTimer.start();
}

void NavaiResultModel::updateFade()
{
    if (_items.isEmpty()) {
        _fadeTimer.stop();
        return;
    }

    const qint64 now =
        QDateTime::currentMSecsSinceEpoch();

    const qint64 dtMs =
        _lastFadeMs > 0
            ? now - _lastFadeMs
            : _fadeTimer.interval();

    _lastFadeMs = now;

    const double step =
        qBound(
            0.01,
            static_cast<double>(dtMs) /
                static_cast<double>(_fadeDurationMs),
            1.0
        );

    bool hasActiveAnimation = false;

    for (int i = 0; i < _items.size(); ++i) {
        Result &item =
            _items[i];

        if (item.opacity < item.targetOpacity) {
            item.opacity =
                qMin(
                    item.targetOpacity,
                    item.opacity + step
                );

            hasActiveAnimation = true;
        } else if (item.opacity > item.targetOpacity) {
            item.opacity =
                qMax(
                    item.targetOpacity,
                    item.opacity - step
                );

            hasActiveAnimation = true;
        }
    }

    if (!_items.isEmpty()) {
        emit dataChanged(
            index(0, 0),
            index(_items.size() - 1, 0),
            {ItemOpacityRole}
        );
    }

    for (int i = _items.size() - 1; i >= 0; --i) {
        const Result &item =
            _items.at(i);

        if (item.targetOpacity <= 0.0 &&
            item.opacity <= 0.001) {

            beginRemoveRows(QModelIndex(), i, i);
            _items.removeAt(i);
            endRemoveRows();
        }
    }

    if (!hasActiveAnimation)
        _fadeTimer.stop();
}

// ======================= PLUGIN =======================

NavaiOverlay::NavaiOverlay(Fact *parent)
    : Fact(parent,
           "navai",
           tr("Navai"),
           tr("Navai UDP results overlay"),
           Group,
           "crosshairs-gps")
{
    qmlRegisterType<NavaiHeatmap>("Apx.Navai", 1, 0, "NavaiHeatmap");
    f_enabled =
        new Fact(
            this,
            "enable",
            tr("Enable"),
            tr("Enable Navai UDP overlay"),
            Fact::Bool | Fact::PersistentValue,
            "link"
        );

    // The overlay is a passive local UDP receiver and is expected to work as
    // soon as the plugin is loaded.  A fresh GCS profile otherwise leaves the
    // receiver disabled, so simulators successfully send packets to port 5005
    // while nothing is listening there.
    f_enabled->setDefaultValue(true);
    f_enabled->setValue(true);

    connect(
        f_enabled,
        SIGNAL(valueChanged()),
        this,
        SLOT(updateEnabled())
    );

    if (Fleet::instance()) {
        connect(
            Fleet::instance(),
            &Fleet::unitSelected,
            this,
            &NavaiOverlay::bindUnit
        );

        bindUnit(Fleet::instance()->current());
    } else {
        postToGcsConsole("Fleet instance not available");
    }

    const QString mapPluginPath =
        uiDir() + "/NavaiMapPlugin.qml";

    if (QFileInfo::exists(mapPluginPath)) {
        loadQml(mapPluginPath);
    } else {
        qWarning().noquote()
            << "[NAVAI] UI file not found:"
            << mapPluginPath;
    }

    updateEnabled();
}

QString NavaiOverlay::uiDir() const
{
    const QString appDir =
        QCoreApplication::applicationDirPath();

    const QString docs =
        QStandardPaths::writableLocation(
            QStandardPaths::DocumentsLocation
        );

    const QStringList candidates = {
        appDir + "/../plugins/navai-overlay-ui",
        appDir + "/navai-overlay-ui",
        appDir + "/../share/gcs/plugins/navai-overlay-ui",
        docs + "/UAVOS/Plugins/navai-overlay-ui"
    };

    for (const QString &path : candidates) {
        if (QDir(path).exists())
            return path;
    }

    return candidates.first();
}

bool NavaiOverlay::isOverlayEnabled() const
{
    return f_enabled &&
           f_enabled->value().toBool();
}

void NavaiOverlay::bindUnit(Unit *unit)
{
    if (_unit)
        disconnect(_unit, nullptr, this, nullptr);

    _unit = unit;
    _mandalaFacts.clear();
    f_camLat = nullptr;
    f_camLon = nullptr;

    if (!unit) {
        postToGcsConsole("Unit bind failed: no current unit");
        return;
    }

    if (!unit->f_mandala) {
        postToGcsConsole("Mandala bind failed: no current unit mandala");
        return;
    }

    collectFactTree(
        _mandalaFacts,
        unit->f_mandala,
        QString()
    );

    QString latKey;
    QString lonKey;

    f_camLat =
        factByAnyId(
            _mandalaFacts,
            {
                "est.cam.lat",
                "est.nav.cam.lat",
                "mandala.est.cam.lat",
                "mandala.est.nav.cam.lat",
                "cam.lat"
            },
            &latKey
        );

    f_camLon =
        factByAnyId(
            _mandalaFacts,
            {
                "est.cam.lon",
                "est.nav.cam.lon",
                "mandala.est.cam.lon",
                "mandala.est.nav.cam.lon",
                "cam.lon"
            },
            &lonKey
        );

    postToGcsConsole(
        QString("Mandala facts bound: total=%1 est.cam.lat=%2 key=%3 est.cam.lon=%4 key=%5")
            .arg(_mandalaFacts.size())
            .arg(f_camLat ? "OK" : "FAIL")
            .arg(latKey)
            .arg(f_camLon ? "OK" : "FAIL")
            .arg(lonKey)
    );
}

QVariantList NavaiOverlay::parseTrajectory(const QJsonArray &trajectory) const
{
    QVariantList coordinates;

    for (const QJsonValue &value : trajectory) {
        if (!value.isObject())
            continue;
        const QJsonObject point = value.toObject();
        bool okLat = false;
        bool okLon = false;
        const double lat = jsonNumber(point, {"lat", "latitude"}, &okLat);
        const double lon = jsonNumber(point, {"lon", "longitude"}, &okLon);
        const QGeoCoordinate coordinate(lat, lon);
        if (!okLat || !okLon || !coordinate.isValid())
            continue;
        coordinates.push_back(
            QVariant::fromValue(coordinate)
        );
    }

    return coordinates;
}

void NavaiOverlay::writeCameraFacts(
    double lat,
    double lon)
{
    if (f_camLat)
        f_camLat->setValue(lat);

    if (f_camLon)
        f_camLon->setValue(lon);

    postToGcsConsole(
        QString("MANDALA CAM WRITE est.cam.lat=%1 est.cam.lon=%2 bind_lat=%3 bind_lon=%4")
            .arg(lat, 0, 'f', 7)
            .arg(lon, 0, 'f', 7)
            .arg(f_camLat ? "OK" : "FAIL")
            .arg(f_camLon ? "OK" : "FAIL")
    );
}

void NavaiOverlay::updateEnabled()
{
    const bool nextActive =
        isOverlayEnabled();

    if (_active != nextActive) {
        _active = nextActive;
        emit activeChanged();
    }

    if (_active) {
        setupUdp();

        postToGcsConsole(
            QString("Navai overlay enabled, UDP port %1")
                .arg(_udpPort)
        );
    } else {
        stopUdp();

        postToGcsConsole("Navai overlay disabled");
    }
}

void NavaiOverlay::setupUdp()
{
    if (_udpSocket)
        return;

    _udpSocket =
        new QUdpSocket(this);

    _udpReady =
        _udpSocket->bind(
            QHostAddress::AnyIPv4,
            _udpPort,
            QUdpSocket::ShareAddress |
                QUdpSocket::ReuseAddressHint
        );

    if (_udpReady) {
        connect(
            _udpSocket,
            &QUdpSocket::readyRead,
            this,
            &NavaiOverlay::readUdpDatagrams
        );

        postToGcsConsole(
            QString("Navai UDP receiver started on port %1")
                .arg(_udpPort)
        );
    } else {
        _udpSocket->deleteLater();
        _udpSocket = nullptr;

        postToGcsConsole(
            QString("Navai UDP receiver failed on port %1")
                .arg(_udpPort)
        );
    }

    emit udpReadyChanged();
}

void NavaiOverlay::stopUdp()
{
    if (_udpSocket) {
        disconnect(
            _udpSocket,
            nullptr,
            this,
            nullptr
        );

        _udpSocket->close();
        _udpSocket->deleteLater();
        _udpSocket = nullptr;
    }

    const bool wasReady =
        _udpReady;

    _udpReady =
        false;

    if (wasReady)
        emit udpReadyChanged();

    _resultsModel.clear();
    _opticalSegments.clear();
    _opticalPoints = 0;
    emit opticalTrackChanged();
}

void NavaiOverlay::readUdpDatagrams()
{
    if (!_active)
        return;

    while (_udpSocket &&
           _udpSocket->hasPendingDatagrams()) {

        QByteArray data;
        data.resize(
            static_cast<int>(
                _udpSocket->pendingDatagramSize()
            )
        );

        QHostAddress sender;
        quint16 senderPort = 0;

        _udpSocket->readDatagram(
            data.data(),
            data.size(),
            &sender,
            &senderPort
        );

        handleDatagram(
            data,
            sender,
            senderPort
        );
    }
}

double NavaiOverlay::jsonNumber(
    const QJsonObject &obj,
    const QStringList &keys,
    bool *ok) const
{
    if (ok)
        *ok = false;

    for (const QString &key : keys) {
        if (!obj.contains(key))
            continue;

        const QJsonValue value =
            obj.value(key);

        if (value.isDouble()) {
            if (ok)
                *ok = true;

            return value.toDouble();
        }

        if (value.isString()) {
            bool localOk = false;

            const double number =
                value.toString().toDouble(&localOk);

            if (localOk) {
                if (ok)
                    *ok = true;

                return number;
            }
        }
    }

    return 0.0;
}

void NavaiOverlay::handleDatagram(
    const QByteArray &data,
    const QHostAddress &sender,
    quint16 senderPort)
{
    if (!_active)
        return;

    QJsonParseError error;

    const QJsonDocument doc =
        QJsonDocument::fromJson(
            data.trimmed(),
            &error
        );

    if (error.error != QJsonParseError::NoError ||
        !doc.isObject()) {

        postToGcsConsole(
            QString("Invalid Navai UDP JSON from %1:%2")
                .arg(sender.toString())
                .arg(senderPort)
        );

        return;
    }

    const QJsonObject packet = doc.object();
    if(packet.value("type").toString()=="tile_grid"){handleTileGrid(packet);return;}
    if(packet.value("type").toString()=="dino_ranking"){handleDinoRanking(packet);return;}
    if(packet.value("type").toString()=="oflow"){handleOpticalFlow(packet);return;}
    QJsonObject obj = packet;

    const bool isRecognitionEnvelope =
        packet.contains("recognized") ||
        packet.contains("raster_overlay") ||
        packet.contains("checked");

    if (isRecognitionEnvelope) {
        if (!packet.value("recognized").toBool() ||
            !packet.value("raster_overlay").isObject()) {
            return;
        }

        obj = packet.value("raster_overlay").toObject();

        const QJsonObject match =
            packet.value("result").toObject();

        if (!obj.contains("percent") &&
            match.contains("inlier_percent")) {
            obj.insert(
                "percent",
                match.value("inlier_percent")
            );
        }

        if (!obj.contains("label"))
            obj.insert("label", "Navai");
    }

    bool okLat = false;
    bool okLon = false;
    bool okPercent = false;

    const double lat =
        jsonNumber(
            obj,
            {"lat", "latitude"},
            &okLat
        );

    const double lon =
        jsonNumber(
            obj,
            {"lon", "longitude"},
            &okLon
        );

    double percent =
        jsonNumber(
            obj,
            {"percent", "confidence", "probability"},
            &okPercent
        );

    if (okPercent &&
        percent > 0.0 &&
        percent <= 1.0) {
        percent *= 100.0;
    }

    percent =
        qBound(
            0.0,
            percent,
            100.0
        );

    if (!okLat ||
        !okLon ||
        !std::isfinite(lat) ||
        !std::isfinite(lon) ||
        lat < -90.0 ||
        lat > 90.0 ||
        lon < -180.0 ||
        lon > 180.0) {

        postToGcsConsole("Invalid Navai result payload");
        return;
    }

    writeCameraFacts(
        lat,
        lon
    );

    const QVariantList trajectoryCoordinates =
        parseTrajectory(packet.value("trajectory").toArray());

    const QString payloadLabel =
        obj.value("label").toString().trimmed();

    const QString percentText =
        QString::number(
            percent,
            'f',
            1
        );

    const QString label =
        payloadLabel.isEmpty()
            ? QString("Navai result - %1%").arg(percentText)
            : QString("%1 - %2%").arg(payloadLabel, percentText);

    const QString consoleText =
        QString("%1, lat=%2 lon=%3")
            .arg(label)
            .arg(lat, 0, 'f', 7)
            .arg(lon, 0, 'f', 7);

    postToGcsConsole(consoleText);

    _resultsModel.addResult(
        lat,
        lon,
        percent,
        label,
        trajectoryCoordinates
    );
}

void NavaiOverlay::handleTileGrid(const QJsonObject &packet)
{
    const int chunkCount = packet.value("chunk_count").toInt();
    const int chunkIndex = packet.value("chunk_index").toInt(-1);
    const QString revision = QString::number(packet.value("revision").toInt());
    QString mode = packet.value("mode").toString("replace");
    if (mode != "append")
        mode = "replace";
    if (chunkCount < 1 || chunkIndex < 0 || chunkIndex >= chunkCount)
        return;

    // Chunks of one grid share the revision and mode; a new revision starts over.
    if (revision != _gridRevision || mode != _gridMode) {
        _gridRevision = revision;
        _gridMode = mode;
        _gridChunks.clear();
        _gridChunkCount = chunkCount;
    }
    if (chunkCount != _gridChunkCount)
        return;
    _gridChunks[chunkIndex] = packet.value("tiles").toArray();
    if (_gridChunks.size() != chunkCount)
        return;

    QVector<NavaiTile> tiles;
    for (int chunk = 0; chunk < chunkCount; ++chunk) {
        for (const QJsonValue &value : _gridChunks[chunk]) {
            const QJsonObject item = value.toObject();
            NavaiTile tile;
            tile.id = item.value("id").toString();
            tile.latitude = item.value("latitude").toDouble();
            tile.longitude = item.value("longitude").toDouble();
            tile.sizeM = item.value("size_m").toDouble();
            if (tile.id.isEmpty() || tile.sizeM <= 0.0)
                continue;
            tiles.append(tile);
        }
    }
    _gridChunks.clear();

    if (mode == "append")
        _tileMap.merge(tiles);
    else
        _tileMap.replace(tiles);
    emit heatmapChanged();

    postToGcsConsole(
        QString("Tile grid %1: revision=%2, tiles=%3, total=%4")
            .arg(mode == "append" ? "merged" : "applied")
            .arg(revision)
            .arg(tiles.size())
            .arg(_tileMap.count())
    );
}
void NavaiOverlay::handleDinoRanking(const QJsonObject &packet)
{
    // Chunks of one ranking share the frame id; an older frame is ignored.
    const qint64 frame = packet.value("frame_id").toVariant().toLongLong();
    if (frame && frame < _rankingFrame)
        return;
    _rankingFrame = frame;

    const QJsonArray tiles = packet.value("tiles").toArray();
    QString topId;
    double topScore = -std::numeric_limits<double>::infinity();
    int applied = 0;
    for (const QJsonValue &value : tiles) {
        const QJsonObject tile = value.toObject();
        const QString id = tile.value("id").toString();
        const double score = tile.value("score").toDouble();
        if (_tileMap.setScore(id, static_cast<float>(score)))
            applied++;
        if (score > topScore) {
            topScore = score;
            topId = id;
        }
    }
    emit heatmapChanged();

    postToGcsConsole(
        tiles.isEmpty()
            ? QString("DINO ranking applied: frame=%1, tiles=0").arg(frame)
            : QString("DINO ranking applied: frame=%1, tiles=%2, known=%3, top=%4 - %5 of 1")
                  .arg(frame)
                  .arg(tiles.size())
                  .arg(applied)
                  .arg(topId)
                  .arg(topScore, 0, 'f', 3)
    );
}

void NavaiOverlay::handleOpticalFlow(const QJsonObject &packet)
{
    // "paused" means the flow is not running (low altitude), nothing to draw.
    const QString status = packet.value("status").toString();
    if (status.isEmpty() || status == "paused")
        return;

    bool okLat = false;
    bool okLon = false;
    const double lat = jsonNumber(packet, {"optical_latitude"}, &okLat);
    const double lon = jsonNumber(packet, {"optical_longitude"}, &okLon);
    const QGeoCoordinate coordinate(lat, lon);
    if (!okLat || !okLon || !coordinate.isValid())
        return;

    // "first" is the first frame of a new flow chain: the line starts over.
    if (status == "first" || _opticalSegments.isEmpty())
        _opticalSegments.append(QList<QGeoCoordinate>());

    _opticalSegments.last().append(coordinate);
    _opticalPoints++;

    // Forget the oldest points so a long flight does not grow the line forever.
    while (_opticalPoints > kMaxOpticalPoints && !_opticalSegments.isEmpty()) {
        QList<QGeoCoordinate> &oldest = _opticalSegments.first();
        if (!oldest.isEmpty()) {
            oldest.removeFirst();
            _opticalPoints--;
        }
        if (oldest.isEmpty())
            _opticalSegments.removeFirst();
    }

    emit opticalTrackChanged();
}

QVariantList NavaiOverlay::opticalTrack() const
{
    QVariantList segments;
    for (const QList<QGeoCoordinate> &segment : _opticalSegments) {
        QVariantList points;
        for (const QGeoCoordinate &coordinate : segment)
            points.append(QVariant::fromValue(coordinate));
        segments.append(QVariant(points));
    }
    return segments;
}

void NavaiOverlay::postToGcsConsole(const QString &text)
{
    const QString trimmed =
        text.trimmed();

    if (trimmed.isEmpty())
        return;

    const QString logDir =
        QStandardPaths::writableLocation(
            QStandardPaths::DocumentsLocation
        ) + "/UAVOS/Logs";

    QDir().mkpath(logDir);

    QFile file(logDir + "/navai-overlay.log");

    if (!file.open(QIODevice::WriteOnly |
                   QIODevice::Append |
                   QIODevice::Text)) {
        return;
    }

    const QString line =
        QString("[%1] [NAVAI] %2\n")
            .arg(
                QDateTime::currentDateTime()
                    .toString("yyyy-MM-dd HH:mm:ss.zzz")
            )
            .arg(trimmed);

    file.write(line.toUtf8());
    file.close();
}
