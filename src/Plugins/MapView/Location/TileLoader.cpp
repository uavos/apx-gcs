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
#include "TileLoader.h"
#include <App/AppLog.h>
#include <App/AppSettings.h>
#include <Database/Database.h>
#include <Fact/Fact.h>
#include <QImage>
#include <QtMath>

#define TILE_DOWNLOAD_MAX 20000    // max tiles per area download request
#define TILE_DOWNLOAD_PARALLEL 4   // concurrent downloads
#define TILE_DOWNLOAD_TIMEOUT 15000 // [ms] transfer timeout for a tile request

#define Random(low, high) ((int) (low + qrand() % (high - low)))

TileLoader *TileLoader::_instance = nullptr;
TileLoader::TileLoader(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Tile loader"),
           tr("Map tiles loader service"),
           Group,
           "earth")
{
    _instance = this;

    f_offline = new Fact(this,
                         "offline",
                         tr("Offline mode"),
                         tr("Disable tiles download"),
                         Bool | PersistentValue,
                         "wifi-off");

    f_area = new Fact(this,
                      "area",
                      tr("Download visible area"),
                      tr("Cache map tiles of the visible area"),
                      Group,
                      "map-outline");

    f_zoom = new Fact(f_area,
                      "zoom",
                      tr("Max zoom"),
                      tr("Max zoom level to download"),
                      Int | PersistentValue,
                      "magnify-plus-outline");
    f_zoom->setMin(2);
    f_zoom->setMax(20);
    f_zoom->setDefaultValue(18);
    connect(f_zoom, &Fact::valueChanged, this, &TileLoader::updateAreaInfo);

    f_cache = new Fact(f_area,
                       "cache",
                       tr("Cache size"),
                       tr("Cached map tiles on disk"),
                       NoFlags,
                       "database");
    connect(f_area, &Fact::triggered, this, &TileLoader::updateCacheInfo);

    f_download = new Fact(f_area,
                          "download",
                          tr("Download"),
                          tr("Download tiles of the visible area"),
                          Action | Apply,
                          "download");
    f_download->setOption(ShowDisabled); // keep the status text visible when disabled
    connect(f_download, &Fact::triggered, this, &TileLoader::downloadArea);

    f_stop = new Fact(f_area, "stop", tr("Stop"), tr("Stop area download"), Action | Stop, "stop");
    f_stop->setVisible(false);
    connect(f_stop, &Fact::triggered, this, &TileLoader::areaStop);

    // no downloads in offline mode
    connect(f_offline, &Fact::valueChanged, this, &TileLoader::updateAreaInfo);
    f_download->setEnabled(!f_offline->value().toBool());

    net = new QNetworkAccessManager(this);

    userAgent = QString("Mozilla/5.0 (Windows NT 6.1; WOW64; rv:%1.0) Gecko/%2%3%4 Firefox/%5.0.%6")
                    .arg(QString::number(QRandomGenerator::global()->bounded(3, 14)),
                         QString::number(
                             QRandomGenerator::global()->bounded(QDate().currentDate().year() - 4,
                                                                 QDate().currentDate().year())),
                         QString::number(QRandomGenerator::global()->bounded(11, 12)),
                         QString::number(QRandomGenerator::global()->bounded(10, 30)),
                         QString::number(QRandomGenerator::global()->bounded(3, 14)),
                         QString::number(QRandomGenerator::global()->bounded(1, 10)))
                    .toUtf8();
    /*QStringList langs = QLocale::system().uiLanguages();
  if (langs.length()>0){
    language=langs[0];
    qDebug()<<language;
  }*/
    language = "en-US";

    connect(this, &TileLoader::requestCountChanged, this, &TileLoader::updateStatus);
}
TileLoader::~TileLoader()
{
    _instance = nullptr;
    abort();
}

void TileLoader::updateStatus()
{
    if (_areaTotal > 0) {
        // area download progress: on the loader page, on the area page and on the stop button
        const int percent = qMax(1, _areaDone * 100 / _areaTotal);
        const auto status = QString("%1/%2").arg(_areaDone).arg(_areaTotal);
        setValue(status);
        setProgress(percent);
        f_area->setValue(status);
        f_area->setProgress(percent);
        f_stop->setTitle(QString("%1 %2 (%3%)").arg(tr("Stop")).arg(status).arg(percent));
        return;
    }
    f_area->setValue(QVariant());
    f_area->setProgress(-1);
    f_stop->setTitle(tr("Stop"));

    int cnt = requestCount();
    setValue(cnt > 0 ? QString::number(cnt) : "");
    setProgress(cnt > 0 ? 0 : -1);
}

void TileLoader::loadTile(quint64 uid)
{
    //qDebug()<<"Load: "<<uid;
    if (downloads.contains(uid))
        return;

    QByteArray data;
    if (readTile(uid, &data)) {
        emit tileLoaded(uid, data);
        return;
    }
    // file not exists - download
    emit download(uid);
}
bool TileLoader::readTile(quint64 uid, QByteArray *data)
{
    auto path = basePath(uid);
    QMap<int, QString> versions;
    for (auto i : QDir(path).entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        versions[i.toInt()] = i;
    if (versions.isEmpty())
        return false;

    auto level = QString::number(TileLoader::level(uid));
    auto fileName = TileLoader::basename(uid) + ".jpg";
    for (auto version : versions) {
        auto filePath = QStringList({path, version, level, fileName}).join('/');
        if (!QFile::exists(filePath))
            continue;
        if (!data)
            return true;

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
            continue;
        *data = file.readAll();
        file.close();
        return true;
    }
    return false;
}
void TileLoader::saveTile(quint64 uid, uint version, const QByteArray &data)
{
    // create filename
    QStringList pathList;
    pathList << basePath(uid);
    pathList << QString::number(version);
    pathList << QString::number(level(uid));
    auto dir = QDir(pathList.join('/'));
    if (!dir.exists())
        dir.mkpath(".");

    auto fileName = dir.absoluteFilePath(basename(uid) + ".jpg");
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
    }

    // qDebug() << "Save tile" << fileName;
}

void TileLoader::loadCancel(quint64 uid)
{
    Q_UNUSED(uid)
    //qDebug()<<"Cancel: "<<uid;
}

bool TileLoader::checkImage(const QByteArray &data)
{
    QImage image;
    image.loadFromData(data);
    if (image.isNull() || image.size() != QSize(256, 256)) {
        return false;
    }
    return true;
}

void TileLoader::abort()
{
    for (auto reply : reqMap.keys()) {
        reply->abort();
        emit tileError(reqMap.value(reply), reply->errorString());
    }
    reqMap.clear();
    downloads.clear();
}

void TileLoader::download(quint64 uid)
{
    //qDebug()<<"download"<<uid;
    if (downloads.contains(uid))
        return;

    if (f_offline->value().toBool())
        return;

    //download
    QNetworkRequest request;
    MapID type = (MapID) TileLoader::type(uid);
    int x = TileLoader::x(uid);
    int y = TileLoader::y(uid);
    int zoom = TileLoader::level(uid);
    bool bVersionRequest = false;
    //qDebug()<<spec<<type;
    switch (type) {
    default:
        break;
    case GoogleSatellite: {
        // http://mt1.google.com/vt/lyrs=s
        if (!checkGoogleVersion(&request)) {
            // tile is downloaded when version is known
            if (!pendingDownloads.contains(uid))
                pendingDownloads.append(uid);
            if (request.url().isEmpty())
                return; // version request is already in progress
            bVersionRequest = true;
        } else {
            QString server = "khm";
            QString req = "kh";
            QString sec1 = ""; // after &x=...
            QString sec2 = ""; // after &zoom=...
            getSecGoogleWords(x, y, sec1, sec2);
            request.setUrl(
                QUrl(QString("http://%1%2.google.com/%3/v=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10")
                         .arg(server)
                         .arg(getServerNum(x, y, 4))
                         .arg(req, versionGoogleMaps, language)
                         .arg(x)
                         .arg(sec1)
                         .arg(y)
                         .arg(zoom)
                         .arg(sec2)));
            request.setRawHeader("Referrer", "https://www.google.com/maps/preview");
        }
    } break;
    case GoogleHybrid: {
        // version is not needed for the request
        request.setUrl(QUrl(QString("https://khm.google.com/vt/lbw/lyrs=y&x=%1&y=%2&z=%3")
                                .arg(x)
                                .arg(y)
                                .arg(zoom)));
        request.setRawHeader("Referrer", "http://maps.google.com/");
    } break;
    }
    //begin download
    QNetworkReply *reply = downloadRequest(&request);
    //qDebug()<<request.url();
    connect(reply,
            static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(
                &QNetworkReply::errorOccurred),
            this,
            &TileLoader::networkReplyError);
    if (bVersionRequest) {
        connect(reply, &QNetworkReply::finished, this, &TileLoader::versionReplyFinished);
    } else {
        connect(reply, &QNetworkReply::finished, this, &TileLoader::networkReplyFinished);
        reqMap.insert(reply, uid);
        downloads.append(uid);
        emit requestCountChanged();
    }
}

QNetworkReply *TileLoader::downloadRequest(QNetworkRequest *request)
{
#if !defined(__mobile__)
    QNetworkProxy proxy = net->proxy();
    QNetworkProxy tProxy;
    tProxy.setType(QNetworkProxy::DefaultProxy);
    net->setProxy(tProxy);
#endif

    QSslConfiguration ssl = request->sslConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
    request->setSslConfiguration(ssl);

    request->setRawHeader("Accept", "*/*");
    request->setRawHeader("User-Agent", userAgent);

    // don't let stalled requests hang forever, spread requests over connections
    request->setTransferTimeout(TILE_DOWNLOAD_TIMEOUT);
    request->setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    QNetworkReply *reply = net->get(*request);

#if !defined(__mobile__)
    net->setProxy(proxy);
#endif
    return reply;
}

bool TileLoader::checkGoogleVersion(QNetworkRequest *request)
{
    if (versionGoogleMaps == "?")
        return false; // request in progress
    if (!versionGoogleMaps.isEmpty())
        return true;
    versionGoogleMaps = "?";
    request->setUrl(QUrl("http://maps.google.com/maps/api/js?v=3.2&sensor=false"));
    request->setRawHeader("Referrer", "https://www.google.com/maps/preview");
    return false;
}

void TileLoader::getSecGoogleWords(int x, int y, QString &sec1, QString &sec2)
{
    sec1 = ""; // after &x=...
    sec2 = ""; // after &zoom=...
    int seclen = ((x * 3) + y) % 8;
    sec2 = QLatin1String("Galileo").left(seclen);
    if (y >= 10000 && y < 100000) {
        sec1 = "&s=";
    }
}

int TileLoader::getServerNum(int x, int y, int max)
{
    return (x + 2 * y) % max;
}

void TileLoader::networkReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    quint64 uid = reqMap.value(reply);
    reqMap.remove(reply);
    downloads.removeOne(uid);
    emit requestCountChanged();

    if (reply->error() != QNetworkReply::NoError) {
        areaError(uid);
        return;
    }
    QByteArray data = reply->readAll();
    emit tileLoaded(uid, data);

    if (!checkImage(data)) {
        apxConsoleW() << "Error downloading map (not an image)";
        areaError(uid);
        return;
    }

    saveTile(uid, versionGoogleMaps.toUInt(), data);
    areaFinished(uid);
}

void TileLoader::networkReplyError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error)
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    apxConsoleW() << "Network error:" << reply->errorString();
    if (!reqMap.contains(reply))
        return; // version request, handled by versionReplyFinished
    quint64 uid = reqMap.value(reply);
    reqMap.remove(reply);
    downloads.removeOne(uid);
    emit requestCountChanged();

    emit tileError(uid, reply->errorString());
    areaError(uid);
}

void TileLoader::versionReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    reply->deleteLater();
    //emit requestCountChanged();
    //decRequestCount();
    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() != QNetworkReply::HostNotFoundError) {
            apxMsgW() << "Error collecting Google maps version info";
        }
        // tiles can't be downloaded now, don't keep requests pending
        versionGoogleMaps.clear(); // retry version request later
        const auto pending = pendingDownloads;
        pendingDownloads.clear();
        for (auto uid : pending) {
            emit tileError(uid, reply->errorString());
            areaError(uid);
        }
        return;
    }
    QString html = QString(reply->readAll());

#if defined(DEBUG_GOOGLE_MAPS)
    QString filename = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    filename += "/google.output";
    QFile file(filename);
    if (file.open(QIODevice::ReadWrite)) {
        QTextStream stream(&file);
        stream << html << endl;
    }
#endif

    bool ok = false;
    static QRegularExpression re("\"*https?://mt\\D?\\d..*/vt\\?lyrs=m@(\\d*)",
                                 QRegularExpression::CaseInsensitiveOption);
    {
        auto match = re.match(html);
        if (match.hasMatch()) {
            // QStringList gc = match.capturedTexts();
            //versions[GoogleMap] = QString("m@%1").arg(gc[1]);
        }
    }
    {
        static QRegularExpression re("\"*https?://khm\\D?\\d.googleapis.com/kh\\?v=(\\d*)",
                                     QRegularExpression::CaseInsensitiveOption);
        auto match = re.match(html);
        if (match.hasMatch()) {
            QStringList gc = match.capturedTexts();
            versionGoogleMaps = gc[1];
            ok = true;
        }
    }
    {
        static QRegularExpression re("\"*https?://mt\\D?\\d..*/vt\\?lyrs=t@(\\d*),r@(\\d*)",
                                     QRegularExpression::CaseInsensitiveOption);
        auto match = re.match(html);
        if (match.hasMatch()) {
            // QStringList gc = match.capturedTexts();
            //_versionGoogleTerrain = QString("t@%1,r@%2").arg(gc[1]).arg(gc[2]);
        }
    }

    if (!ok) {
        // page format changed - use the latest version known from cache
        uint v = 0;
        for (auto i : QDir(basePath(uid(GoogleSatellite, 0, 0, 0))).entryList(QDir::Dirs))
            v = qMax(v, i.toUInt());
        versionGoogleMaps = QString::number(v > 0 ? v : 1015);
        apxMsgW() << "Google maps version unknown, using" << versionGoogleMaps;
    }
    if (versionGoogleMaps.isEmpty())
        versionGoogleMaps = "748";
    //qDebug()<<versionGoogleMaps;
    for (auto uid : pendingDownloads) {
        download(uid);
    }
    pendingDownloads.clear();
}

// area download

int TileLoader::tileX(double lon, int zoom)
{
    const double n = 1 << zoom;
    int x = static_cast<int>(std::floor((lon + 180.0) / 360.0 * n));
    return std::clamp(x, 0, static_cast<int>(n) - 1);
}
int TileLoader::tileY(double lat, int zoom)
{
    const double n = 1 << zoom;
    lat = std::clamp(lat, -85.0511, 85.0511);
    const double latRad = qDegreesToRadians(lat);
    int y = static_cast<int>(
        std::floor((1.0 - std::log(std::tan(latRad) + 1.0 / std::cos(latRad)) / M_PI) / 2.0 * n));
    return std::clamp(y, 0, static_cast<int>(n) - 1);
}

void TileLoader::setVisibleArea(
    double latN, double lonW, double latS, double lonE, int zoom, QString mapType)
{
    _visLatN = latN;
    _visLonW = lonW;
    _visLatS = latS;
    _visLonE = lonE;
    _visZoom = zoom;
    _visMapType = mapType;
    updateAreaInfo();
}

int TileLoader::areaTilesCount(int zoomFrom, int zoomTo) const
{
    qint64 cnt = 0;
    for (int zoom = zoomFrom; zoom <= zoomTo; ++zoom) {
        int x1 = tileX(_visLonW, zoom), x2 = tileX(_visLonE, zoom);
        int y1 = tileY(_visLatN, zoom), y2 = tileY(_visLatS, zoom);
        cnt += static_cast<qint64>(std::abs(x2 - x1) + 1) * (std::abs(y2 - y1) + 1);
        if (cnt > TILE_DOWNLOAD_MAX * 100)
            break;
    }
    return static_cast<int>(std::min<qint64>(cnt, TILE_DOWNLOAD_MAX * 100));
}

int TileLoader::areaMissingCount(int zoomFrom, int zoomTo, MapID type) const
{
    // tiles of the area not found in the cache
    const auto path = basePath(uid(type, 0, 0, 0));
    const auto versions = QDir(path).entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    int cnt = 0;
    for (int zoom = zoomFrom; zoom <= zoomTo; ++zoom) {
        int x1 = tileX(_visLonW, zoom), x2 = tileX(_visLonE, zoom);
        int y1 = tileY(_visLatN, zoom), y2 = tileY(_visLatS, zoom);
        if (x1 > x2)
            std::swap(x1, x2);
        if (y1 > y2)
            std::swap(y1, y2);
        const auto level = QString::number(zoom);
        for (int x = x1; x <= x2; ++x) {
            for (int y = y1; y <= y2; ++y) {
                const auto fileName = basename(uid(type, zoom, x, y)) + ".jpg";
                bool exists = false;
                for (const auto &version : versions) {
                    if (!QFile::exists(QStringList({path, version, level, fileName}).join('/')))
                        continue;
                    exists = true;
                    break;
                }
                if (!exists)
                    cnt++;
            }
        }
    }
    return cnt;
}

void TileLoader::updateAreaInfo()
{
    if (_visZoom <= 0) {
        f_download->setTitle(tr("Download"));
        return;
    }
    int zoomTo = std::clamp(f_zoom->value().toInt(), 2, 20);
    int zoomFrom = std::clamp(_visZoom, 2, zoomTo);
    const int cnt = areaTilesCount(zoomFrom, zoomTo);
    MapID type = _visMapType.contains("satellite", Qt::CaseInsensitive) ? GoogleSatellite
                                                                         : GoogleHybrid;
    const bool offline = f_offline->value().toBool();

    // tiles count on the button: missing/total and estimated size (~20 KB per tile)
    QString title;
    bool enabled = !offline;
    if (cnt > TILE_DOWNLOAD_MAX) {
        title = QString("%1 %2").arg(tr("Too many tiles")).arg(cnt);
        enabled = false;
    } else {
        const int missing = areaMissingCount(zoomFrom, zoomTo, type);
        if (missing == 0) {
            title = QString("%1 %2").arg(tr("Cached")).arg(cnt);
            enabled = false;
        } else {
            title = QString("%1 %2/%3 (~%4 MB)")
                        .arg(tr("Download"))
                        .arg(missing)
                        .arg(cnt)
                        .arg(missing * 20 / 1024);
        }
    }
    f_download->setTitle(title);
    f_download->setEnabled(enabled);
    f_download->setDescr(tr("Download tiles of the visible area")
                         + QString(" z%1-%2, %3").arg(zoomFrom).arg(zoomTo).arg(_visMapType));
}

void TileLoader::downloadArea()
{
    if (f_offline->value().toBool()) {
        apxMsgW() << tr("Offline mode, tiles download disabled");
        return;
    }
    if (!_areaQueue.isEmpty() || !_areaActive.isEmpty()) {
        apxMsgW() << tr("Map download in progress");
        return;
    }
    if (_visZoom <= 0) {
        apxMsgW() << tr("Map area is not visible");
        return;
    }

    const double latN = _visLatN, lonW = _visLonW, latS = _visLatS, lonE = _visLonE;
    MapID type = _visMapType.contains("satellite", Qt::CaseInsensitive) ? GoogleSatellite
                                                                         : GoogleHybrid;

    int zoomTo = std::clamp(f_zoom->value().toInt(), 2, 20);
    int zoomFrom = std::clamp(_visZoom, 2, zoomTo);

    QList<quint64> queue;
    for (int zoom = zoomFrom; zoom <= zoomTo; ++zoom) {
        int x1 = tileX(lonW, zoom), x2 = tileX(lonE, zoom);
        int y1 = tileY(latN, zoom), y2 = tileY(latS, zoom);
        if (x1 > x2)
            std::swap(x1, x2);
        if (y1 > y2)
            std::swap(y1, y2);
        for (int x = x1; x <= x2; ++x) {
            for (int y = y1; y <= y2; ++y) {
                queue.append(uid(type, zoom, x, y));
                if (queue.size() > TILE_DOWNLOAD_MAX) {
                    apxMsgW() << tr("Too many tiles to download, reduce the area or max zoom");
                    return;
                }
            }
        }
    }
    if (queue.isEmpty())
        return;

    _areaQueue = queue;
    _areaRetried.clear();
    _areaTotal = queue.size();
    _areaDone = 0;
    apxMsg() << tr("Downloading map tiles").append(':') << _areaTotal
             << QString("(z%1-%2)").arg(zoomFrom).arg(zoomTo);
    areaNext();
}

void TileLoader::areaNext()
{
    while (_areaActive.size() < TILE_DOWNLOAD_PARALLEL && !_areaQueue.isEmpty()) {
        auto uid = _areaQueue.takeFirst();
        if (readTile(uid, nullptr)) {
            _areaDone++; // already cached
            continue;
        }
        _areaActive.insert(uid);
        download(uid);
    }
    updateAreaStatus();
}

void TileLoader::areaFinished(quint64 uid)
{
    if (!_areaActive.remove(uid))
        return;
    _areaDone++;
    areaNext();
}

void TileLoader::areaError(quint64 uid)
{
    if (!_areaActive.remove(uid))
        return;
    if (_areaRetried.contains(uid)) {
        _areaDone++; // give up
    } else {
        _areaRetried.insert(uid);
        _areaQueue.append(uid); // retry later
    }
    areaNext();
}

void TileLoader::updateAreaStatus()
{
    const bool active = !(_areaQueue.isEmpty() && _areaActive.isEmpty());

    if (!active && _areaTotal > 0) {
        apxMsg() << tr("Map tiles download finished").append(':') << _areaDone;
        _areaTotal = 0;
        updateCacheInfo();
        updateAreaInfo();
    }

    // menu: show stop instead of download, lock settings while downloading
    f_download->setVisible(!active);
    f_stop->setVisible(active);
    f_offline->setEnabled(!active);
    f_zoom->setEnabled(!active);

    updateStatus(); // progress bar and counter
}

void TileLoader::areaStop()
{
    _areaQueue.clear();
    _areaActive.clear();
    _areaRetried.clear();
    _areaTotal = 0;
    updateAreaStatus();
}

// cache on disk

void TileLoader::updateCacheInfo()
{
    quint64 size = 0;
    uint cnt = 0;
    QDirIterator it(cachePath(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        size += it.fileInfo().size();
        cnt++;
    }
    f_cache->setValue(QString("%1 (%2 MB)").arg(cnt).arg(size / (1024 * 1024)));
}
