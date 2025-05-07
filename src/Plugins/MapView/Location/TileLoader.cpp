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
    int cnt = requestCount();
    setValue(cnt > 0 ? QString::number(cnt) : "");
    setProgress(cnt > 0 ? 0 : -1);
}

void TileLoader::loadTile(quint64 uid)
{
    //qDebug()<<"Load: "<<uid;
    if (downloads.contains(uid))
        return;

    do {
        auto path = basePath(uid);
        QMap<int, QString> versions;
        for (auto i : QDir(path).entryList(QDir::Dirs))
            versions[i.toInt()] = i;
        if (versions.isEmpty())
            break;

        auto level = QString::number(TileLoader::level(uid));
        auto fileName = TileLoader::basename(uid) + ".jpg";
        for (auto version : versions) {
            auto filePath = QStringList({path, version, level, fileName}).join('/');
            if (!QFile::exists(filePath))
                continue;

            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly))
                continue;

            auto data = file.readAll();
            file.close();
            // qDebug() << "Load tile" << filePath;
            emit tileLoaded(uid, data);
            return;
        }

    } while (0);
    // file not exists - download
    emit download(uid);
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
            if (request.url().isEmpty())
                return;
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
        if (!checkGoogleVersion(&request)) {
            if (request.url().isEmpty())
                return;
            bVersionRequest = true;
        } else {
            request.setUrl(QUrl(QString("https://khm.google.com/vt/lbw/lyrs=y&x=%1&y=%2&z=%3")
                                    .arg(x)
                                    .arg(y)
                                    .arg(zoom)));
            request.setRawHeader("Referrer", "http://maps.google.com/");
        }
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
        pendingDownloads.append(uid);
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

    QNetworkReply *reply = net->get(*request);

#if !defined(__mobile__)
    net->setProxy(proxy);
#endif
    return reply;
}

bool TileLoader::checkGoogleVersion(QNetworkRequest *request)
{
    if (!versionGoogleMaps.isEmpty())
        return true;
    if (versionGoogleMaps == "?")
        return false;
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
        return;
    }
    QByteArray data = reply->readAll();
    emit tileLoaded(uid, data);

    if (!checkImage(data)) {
        apxConsoleW() << "Error downloading map (not an image)";
    } else {
        // qDebug() << versionGoogleMaps << data.size()
        //          << reply->header(QNetworkRequest::ContentTypeHeader).toString();

        saveTile(uid, versionGoogleMaps.toUInt(), data);
    }

    //qDebug()<<"downloaded"<<uid;
}

void TileLoader::networkReplyError(QNetworkReply::NetworkError error)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    quint64 uid = reqMap.value(reply);
    reqMap.remove(reply);
    downloads.removeOne(uid);
    emit requestCountChanged();

    apxConsoleW() << "Network error:" << reply->errorString();
    if (error != QNetworkReply::OperationCanceledError) {
        emit tileError(uid, reply->errorString());
    } else {
        emit tileError(uid, reply->errorString());
    }
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

    if (!ok)
        return;
    if (versionGoogleMaps.isEmpty())
        versionGoogleMaps = "748";
    //qDebug()<<versionGoogleMaps;
    for (auto uid : pendingDownloads) {
        download(uid);
    }
    pendingDownloads.clear();
}
