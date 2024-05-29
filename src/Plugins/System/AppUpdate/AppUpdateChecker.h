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

#include <Fact/Fact.h>

#include <GithubReleases.h>

class AppUpdateChecker : public Fact
{
    Q_OBJECT

    Q_PROPERTY(QString releaseName READ releaseName NOTIFY releaseInfoChanged)
    Q_PROPERTY(QString releaseVersion READ releaseVersion NOTIFY releaseInfoChanged)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY releaseInfoChanged)

public:
    AppUpdateChecker(Fact *parent = nullptr);

    auto releaseName() const { return _releaseName; }
    auto releaseVersion() const { return _releaseVersion.toString(); }
    auto releaseNotes() const { return _releaseNotes; }

    auto repoName() const { return _gh.repoName(); }

    Fact *f_upgrade;
    Fact *f_stop;
    Fact *f_www;

private:
    GithubReleases _gh{"uavos/apx-gcs", this};

    QVersionNumber _releaseVersion;

    QString _releaseName;
    QString _releaseNotes;
    QString _assetName;
    QUrl _assetUrl;
    QUrl _releaseUrl;

    void updateStatus();

    void reinstallApplication(QString filePath);
    QProcess *_process{};

private slots:
    // GithubReleases
    void latestVersionInfo(QVersionNumber version, QString tag);
    void releaseInfo(QJsonDocument json);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(QString assetName, QFile *data);

    // process
    void hdutilFinished(int exitCode, QProcess::ExitStatus exitStatus);

public slots:
    void abort();
    void checkForUpdates();
    void installUpdate();

signals:
    void releaseInfoChanged();
};
