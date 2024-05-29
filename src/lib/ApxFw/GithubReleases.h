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
#pragma once

#include <QtCore>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

class GithubReleases : public QObject
{
    Q_OBJECT

public:
    GithubReleases(QString repo, QObject *parent = nullptr);

    auto repoName() const { return _repo; }
    auto latestVersion() const { return _latest; }

private:
    QString _repo;
    QVersionNumber _latest;

    QNetworkAccessManager _net;
    QNetworkReply *_reply{};
    QNetworkReply *request(QUrl url);
    QNetworkReply *getReply(QObject *sender);

    QTemporaryFile *_file{};

private slots:
    void responseError(QString msg);
    void responseLatestVersionInfo();
    void responseReleaseInfo();
    void responseDownload();
    void responseDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

public slots:
    void requestLatestVersionInfo();
    void requestReleaseInfo(QString tag);

    void requestDownload(QUrl url);
    void requestDownloadAsset(QString assetName, QString tag);

    void abort();

signals:
    void finished();
    void error(QString msg);

    void latestVersionInfo(QVersionNumber version, QString tag);

    void releaseInfo(QJsonDocument json);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(QString assetName, QFile *data);
};
