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
#include "AppUpdateChecker.h"

#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppGcs.h>

AppUpdateChecker::AppUpdateChecker(Fact *parent)
    : Fact(parent, "checker", "", tr("Current version: ").append(App::version()), Fact::Group)
{
    setOpt("page", "qrc:/" PLUGIN_NAME "/AppUpdate.qml");

    auto f_upgrade = new Fact(this, "upgrade", tr("Upgrade"), title(), Action | Apply, "download");

    // connect GithubReleases
    connect(&_gh, &GithubReleases::latestVersionInfo, this, &AppUpdateChecker::latestVersionInfo);
    connect(&_gh, &GithubReleases::releaseInfo, this, &AppUpdateChecker::releaseInfo);
    connect(&_gh, &GithubReleases::downloadProgress, this, &AppUpdateChecker::downloadProgress);
    connect(&_gh, &GithubReleases::downloadFinished, this, &AppUpdateChecker::downloadFinished);
    connect(&_gh, &GithubReleases::finished, this, [this]() { setProgress(-1); });
    connect(&_gh, &GithubReleases::error, this, [this](QString msg) {
        apxMsgW() << title().append(':') << msg;
        setProgress(-1);
    });

    connect(this, &AppUpdateChecker::releaseInfoChanged, this, &AppUpdateChecker::updateStatus);
    updateStatus();
}

void AppUpdateChecker::updateStatus()
{
    if (_releaseVersion.isNull()) {
        // setEnabled(false);
        setTitle(tr("Waiting for updates"));
        return;
    }
    // repo version available
    setEnabled(true);
    setProgress(-1);

    // compare versions
    auto current = QVersionNumber::fromString(App::version());
    if (current != _releaseVersion) {
        setTitle(tr("Update available: ").append(_releaseVersion.toString()));
    } else {
        setTitle(tr("No updates available"));
    }
}

void AppUpdateChecker::abort()
{
    _gh.abort();
}

void AppUpdateChecker::checkForUpdates()
{
    setProgress(0);
    _gh.requestLatestVersionInfo();
}

void AppUpdateChecker::installUpdate()
{
    if (_releaseVersion.isNull())
        return;

    apxMsgW() << "Not implemented";
}

void AppUpdateChecker::latestVersionInfo(QVersionNumber version, QString tag)
{
    _releaseVersion = version;
    _releaseName.clear();
    _releaseNotes.clear();
    _assetName.clear();
    _assetUrl.clear();
    emit releaseInfoChanged();

    auto v_current = QVersionNumber::fromString(QCoreApplication::applicationVersion());
    if (v_current >= version) {
        qDebug() << "no updates:" << v_current << ">=" << version;
    } else {
        apxMsg() << tr("Update available").append(':') << version.toString();
    }

    _gh.requestReleaseInfo(tag);
}

void AppUpdateChecker::releaseInfo(QJsonDocument json)
{
    while (json["assets"].toArray().count() > 0) {
        if (json["author"]["login"].toString() != "uavinda")
            break;

        //find asset
        _releaseName = json["name"].toString();
        _releaseNotes = json["body"].toString();

        for (auto asset : json["assets"].toArray()) {
            auto jo = asset.toObject();
            auto name = jo["name"].toString();

            // parse filename components
            // APX_Ground_Control-11.1.15-macos-x86_64.dmg
            auto fi = QFileInfo(name);
            auto parts = fi.completeBaseName().split('-');

            if (parts.size() != 4)
                continue;
            if (parts[0] != "APX_Ground_Control")
                continue;
            if (parts[1] != _releaseVersion.toString())
                continue;

#if defined(Q_OS_MAC)
            if (fi.suffix() != "dmg")
                continue;
            if (parts[2] != "macos")
                continue;
            if (parts[3] != "universal" && parts[3] != QSysInfo::currentCpuArchitecture())
                continue;
#elif defined(Q_OS_LINUX)
            if (fi.suffix() != "AppImage")
                continue;
            if (parts[2] != "linux")
                continue;
            if (parts[3] != QSysInfo::currentCpuArchitecture())
                continue;
#else
#error "Unknown platform"
#endif

            _assetName = name;
            _assetUrl = jo["browser_download_url"].toString();
            break;
        }
        if (_assetName.isEmpty())
            break;

        // downloadable asset found
        qDebug() << "asset:" << _assetName;
        return;
    }
    apxMsgW() << tr("Update not available");
    qWarning() << json;
}

void AppUpdateChecker::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        setProgress((bytesReceived * 100) / bytesTotal);
    }
}

void AppUpdateChecker::downloadFinished(QString assetName, QFile *data)
{
    if (!assetName.endsWith(".zip")) {
        apxMsgW() << title().append(':') << tr("Unknown response") << assetName;
        return;
    }

    QDir dir(AppDirs::firmware().absoluteFilePath("releases"));
    dir.mkpath(".");
    auto newFileName = dir.absoluteFilePath(assetName);

    if (!data->copy(newFileName)) {
        apxMsgW() << tr("Can't copy file:") << newFileName;
        return;
    }

    qDebug() << "downloaded" << newFileName << QFileInfo(newFileName).size();
}
