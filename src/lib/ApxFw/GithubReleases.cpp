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
#include "GithubReleases.h"

GithubReleases::GithubReleases(QString repo, QObject *parent)
    : QObject(parent)
    , _repo(repo)
{
    _net.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
}

void GithubReleases::abort()
{
    if (!_reply)
        return;
    _reply->abort();
    _reply = nullptr;
}

QNetworkReply *GithubReleases::request(QUrl url)
{
    if (_reply)
        abort();

    QNetworkRequest *request = new QNetworkRequest(url);

    QSslConfiguration ssl = request->sslConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
    request->setSslConfiguration(ssl);

    request->setRawHeader("Accept", "*/*");
    request->setRawHeader("User-Agent",
                          QString("%1 (v%2)")
                              .arg(QCoreApplication::applicationName())
                              .arg(QCoreApplication::applicationVersion())
                              .toUtf8());

    // qDebug() << url;

    QNetworkReply *reply = _net.get(*request);
    return reply;
}

QNetworkReply *GithubReleases::getReply(QObject *sender)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender);
    if (!reply) {
        responseError("invalid reply");
        emit finished();
        return nullptr;
    }

    reply->deleteLater();

    auto r = _reply;
    _reply = nullptr;

    if (r != reply) {
        responseError("wrong reply");
        emit finished();
        return nullptr;
    }

    if (reply->error()) {
        responseError(reply->errorString());
    }
    return reply;
}

void GithubReleases::responseError(QString msg)
{
    qWarning() << QString("%1: %2").arg(_repo, msg);

    if (_file) {
        _file->deleteLater();
        _file = nullptr;
    }

    emit error(msg);
}

void GithubReleases::requestLatestVersionInfo()
{
    _reply = request(QUrl(QString("https://github.com/%1/releases/latest").arg(_repo)));
    connect(_reply, &QNetworkReply::finished, this, &GithubReleases::responseLatestVersionInfo);
}

void GithubReleases::responseLatestVersionInfo()
{
    QNetworkReply *reply = getReply(sender());
    if (!reply)
        return;

    // reply URL contains version number
    emit finished();

    QString tag(reply->url().toString());
    tag = tag.mid(tag.lastIndexOf('/') + 1);

    QVersionNumber version = QVersionNumber::fromString(tag.mid(tag.lastIndexOf('-') + 1));
    if (version.isNull()) {
        responseError(tr("Release not found (%1)").arg(_repo));
        qWarning() << "no releases:" << tag;
        return;
    }

    // qDebug() << version << tag;

    emit latestVersionInfo(version, tag);
}

void GithubReleases::requestReleaseInfo(QString tag)
{
    _reply = request(QString("https://api.github.com/repos/%1/releases/tags/%2").arg(_repo, tag));
    connect(_reply, &QNetworkReply::finished, this, &GithubReleases::responseReleaseInfo);
}

void GithubReleases::responseReleaseInfo()
{
    QNetworkReply *reply = getReply(sender());
    if (!reply)
        return;

    emit finished();

    QJsonParseError jserr;
    QJsonDocument json(QJsonDocument::fromJson(reply->readAll(), &jserr));
    if (jserr.error != QJsonParseError::NoError) {
        qWarning() << jserr.errorString();
        return;
    }
    if (json.object().contains("message")) {
        QString msg = json["message"].toString();
        responseError(msg);
        return;
    }

    emit releaseInfo(json);

    /*
    QString name = json["name"].toString();
    QString notes = json["body"].toString();

    QHash<QString, QString> assets;
    for (auto asset : json["assets"].toArray()) {
        QJsonObject jo = asset.toObject();
        QString s = jo["name"].toString();
        if (s.isEmpty())
            continue;
        assets[s] = jo["browser_download_url"].toString();
    }*/
}

void GithubReleases::requestDownload(QUrl url)
{
    _reply = request(url);

    _file = new QTemporaryFile(this);
    if (!_file->open()) {
        _file = nullptr;
        _reply->abort();
        _reply = nullptr;
        responseError("cannot open temp file");
        return;
    }

    connect(_reply, &QNetworkReply::finished, this, &GithubReleases::responseDownload);
    connect(_reply,
            &QNetworkReply::downloadProgress,
            this,
            &GithubReleases::responseDownloadProgress);
}

void GithubReleases::requestDownloadAsset(QString assetName, QString tag)
{
    auto s = QString("https://github.com/%1/releases/download/%2/%3").arg(_repo, tag, assetName);
    requestDownload(QUrl(s));
}

void GithubReleases::responseDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal <= 0)
        return;

    if (!_file)
        return;

    _file->write(_reply->readAll());

    emit downloadProgress(bytesReceived, bytesTotal);
}

void GithubReleases::responseDownload()
{
    QNetworkReply *reply = getReply(sender());
    if (!reply)
        return;

    emit finished();

    if (reply->error())
        return;

    if (!_file)
        return;

    _file->flush();

    if (_file->size() <= 0) {
        responseError("no data");
        return;
    }

    if (reply->bytesAvailable() > 0) {
        responseError("unread data");
        return;
    }

    QString s = reply->header(QNetworkRequest::ContentDispositionHeader).toString();
    s = s.mid(s.lastIndexOf("filename=", Qt::CaseInsensitive));
    s = s.mid(s.indexOf("=") + 1);
    s = s.left(s.indexOf(";"));
    auto assetName = s;

    // call slot with downloaded data
    auto file = _file; // can be re-assigned in slots
    _file = nullptr;

    file->seek(0);
    emit downloadFinished(assetName, file);
    file->deleteLater();
}
