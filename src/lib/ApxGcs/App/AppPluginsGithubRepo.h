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
#include <QtCore>

/*
 * A third party plugin repository installed from GitHub (or being installed):
 * AppDirs::githubPlugins()/<repo>/<Name>.so with the release info in <repo>/<repo>.json.
 * The release asset <Name>-<os>-<arch>.zip is unpacked into the repository folder.
 * Releases are tagged with the application version plus the plugin build number ([v]11.2.32.1),
 * check() finds the latest release built for the current application version.
 */
class AppPluginsGithubRepo : public Fact
{
    Q_OBJECT
public:
    // installed repository, info from <repo>/<repo>.json
    explicit AppPluginsGithubRepo(Fact *parent, const QJsonObject &info);
    // new repository to install
    AppPluginsGithubRepo(Fact *parent, const QString &owner, const QString &repo);

    auto owner() const { return _owner; }
    auto repo() const { return _repo; }
    auto info() const { return _info; }
    QString url() const;
    QDir dir() const;

    bool isInstalled() const { return !_info.isEmpty(); }
    QString installedTag() const { return _info.value("tag").toString(); }
    QVersionNumber installedVersion() const;
    // the installed release matches the application version
    bool isCompatible() const;

    // check() results
    bool isChecked() const { return _checked; }
    // the latest release for the application version
    auto latestTag() const { return _latestTag; }
    // the plugin name from the asset name (Emip-linux-x86_64.zip -> Emip)
    auto latestPlugin() const { return _latestPlugin; }
    // the latest release is newer than the installed one, or the installed one is not compatible
    bool hasUpdate() const;
    // short text: Up to date, Update available: v.., Not compatible.., Not available..
    QString status() const;

    // release asset suffixes for this platform, most specific first: linux-x86_64, macos-universal
    static QStringList platformTags();
    // the asset for this platform: <Name>-<tag>.zip with the library inside (or a bare <Name>-<tag>.so|dylib)
    static QString pickAsset(const QJsonArray &assets, QString &assetName, QString &pluginName);

    Fact *f_library{};
    Fact *f_update{};
    Fact *f_remove{};

public slots:
    // find the latest release for the application version
    void check();
    // download the latest release library (checks first when needed)
    void install();
    void abort();

signals:
    void checked();
    // the library file is replaced, libPath is AppDirs::githubPlugins()/<repo>/<Name>.so
    void installed(QString libPath);
    void failed(QString msg);

private:
    QString _owner;
    QString _repo;
    QJsonObject _info;

    GithubReleases _gh;

    bool _checked{};
    bool _installAfterCheck{};

    QVersionNumber _latestVersion;
    QString _latestTag;
    QString _latestAsset;
    QString _latestPlugin;
    QString _latestUrl;

    void init();
    void updateStatus();
    QString infoPath() const;
    bool writeInfo() const;
    void fail(const QString &msg);

private slots:
    void releasesInfo(QJsonArray releases);
    void downloadFinished(QString assetName, QFile *data);
};
