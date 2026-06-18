#include "NavaiOverlay.h"

#include <App/AppNotify.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QStandardPaths>
#include <QUrl>

#include <cmath>

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

    case RadiusMetersRole:
        return item.radiusMeters;

    case PercentRole:
        return item.percent;

    case LabelRole:
        return item.label;

    case ItemOpacityRole:
        return item.opacity;

    default:
        return {};
    }
}

QHash<int, QByteArray> NavaiResultModel::roleNames() const
{
    return {
        {LatitudeRole, "latitude"},
        {LongitudeRole, "longitude"},
        {RadiusMetersRole, "radiusMeters"},
        {PercentRole, "percent"},
        {LabelRole, "label"},
        {ItemOpacityRole, "itemOpacity"}
    };
}

void NavaiResultModel::addResult(
    double lat,
    double lon,
    double radiusMeters,
    double percent,
    const QString &label)
{
    for (int i = 0; i < _items.size(); ++i) {
        _items[i].targetOpacity = 0.0;
    }

    if (!_items.isEmpty()) {
        emit dataChanged(
            index(0, 0),
            index(_items.size() - 1, 0),
            {ItemOpacityRole}
        );
    }

    Result item;
    item.lat = lat;
    item.lon = lon;
    item.radiusMeters = radiusMeters;
    item.percent = percent;
    item.opacity = 0.0;
    item.targetOpacity = 1.0;
    item.label = label;

    const int row =
        _items.size();

    beginInsertRows(QModelIndex(), row, row);
    _items.push_back(item);
    endInsertRows();

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
           "location")
{
    const QString mapPluginPath =
        uiDir() + "/NavaiMapPlugin.qml";

    if (QFileInfo::exists(mapPluginPath))
        loadQml(mapPluginPath);

    setupUdp();
}

QString NavaiOverlay::uiDir() const
{
    const QString docs =
        QStandardPaths::writableLocation(
            QStandardPaths::DocumentsLocation
        );

    const QStringList candidates = {
        docs + "/UAVOS/Plugins/navai-overlay-ui",
        QCoreApplication::applicationDirPath()
            + "/../share/gcs/plugins/navai-overlay-ui",
        QCoreApplication::applicationDirPath()
            + "/../plugins/navai-overlay-ui",
        QCoreApplication::applicationDirPath()
            + "/navai-overlay-ui"
    };

    for (const QString &path : candidates) {
        if (QDir(path).exists())
            return path;
    }

    return candidates.first();
}

void NavaiOverlay::setupUdp()
{
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
        postToGcsConsole(
            QString("Navai UDP receiver failed on port %1")
                .arg(_udpPort)
        );
    }

    emit udpReadyChanged();
}

void NavaiOverlay::readUdpDatagrams()
{
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

    const QJsonObject obj =
        doc.object();

    bool okLat = false;
    bool okLon = false;
    bool okPercent = false;
    bool okRadius = false;

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

    const double radiusMeters =
        jsonNumber(
            obj,
            {"radius_m", "spread_m", "radiusMeters", "spreadMeters", "radius"},
            &okRadius
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
        !okRadius ||
        !std::isfinite(lat) ||
        !std::isfinite(lon) ||
        !std::isfinite(radiusMeters) ||
        lat < -90.0 ||
        lat > 90.0 ||
        lon < -180.0 ||
        lon > 180.0 ||
        radiusMeters <= 0.0) {

        postToGcsConsole("Invalid Navai result payload");
        return;
    }

    const QString label =
        QString("Navai result - %1%")
            .arg(
                QString::number(
                    percent,
                    'f',
                    1
                )
            );

    const QString consoleText =
        QString("%1, lat=%2 lon=%3 spread=%4 m")
            .arg(label)
            .arg(lat, 0, 'f', 7)
            .arg(lon, 0, 'f', 7)
            .arg(radiusMeters, 0, 'f', 1);

    postToGcsConsole(consoleText);

    _resultsModel.addResult(
        lat,
        lon,
        radiusMeters,
        percent,
        label
    );
}

void NavaiOverlay::postToGcsConsole(const QString &text)
{
    if (text.trimmed().isEmpty())
        return;

    const QString message =
        QString("[NAVAI] %1").arg(text);

    qInfo().noquote() << message;

    QMetaObject::invokeMethod(
        QCoreApplication::instance(),
        [message]() {
            AppNotify::instance()->notification(
                message,
                "NAVAI",
                AppNotify::Info,
                nullptr
            );
        },
        Qt::QueuedConnection
    );
}
