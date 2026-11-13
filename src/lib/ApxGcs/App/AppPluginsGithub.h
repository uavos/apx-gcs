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
#include <QtCore>

#include "AppPluginsGithubRepo.h"

/*
 * Third party plugins installed from GitHub repositories.
 * A release of the repository provides the plugin library built for every platform
 * as an asset named <Name>-<os>-<arch>.zip (linux-x86_64, macos-universal, macos-arm64).
 * Releases are tagged with the application version plus the plugin build number ([v]11.2.32.1),
 * the archive of the latest release for the current application version is unpacked into
 * AppDirs::githubPlugins()/<repo>/, the release info is kept in <repo>/<repo>.json next to it.
 * The plugins are loaded by AppPlugins like the other user plugins and are switched on and off
 * in the plugins settings. Updates are checked on startup and on request, like the app updates.
 */
class AppPluginsGithub : public Fact
{
    Q_OBJECT
public:
    explicit AppPluginsGithub(Fact *parent);

    // https://github.com/<owner>/<repo>[.git], github.com/<owner>/<repo>, <owner>/<repo>
    static bool parseRepoUrl(const QString &url, QString &owner, QString &repo);

    Fact *f_install; // page
    Fact *f_url;
    Fact *f_download;

    Fact *f_auto;
    Fact *f_check;
    Fact *f_updates;

    QList<AppPluginsGithubRepo *> repos() const { return _repos; }
    AppPluginsGithubRepo *repo(const QString &name) const;

private:
    QList<AppPluginsGithubRepo *> _repos;
    AppPluginsGithubRepo *_installing{};
    QSet<AppPluginsGithubRepo *> _checking; // release checks started by checkForUpdates and not finished yet

    static QDir dir();

    void loadRepos();
    AppPluginsGithubRepo *addRepo(const QJsonObject &info);
    void connectRepo(AppPluginsGithubRepo *r);
    void removeRepo(AppPluginsGithubRepo *r);
    void deleteRepo(AppPluginsGithubRepo *r);

    void updateStatus();

private slots:
    void install();
    void checkForUpdates();

    void repoChecked();
    void repoInstalled(QString libPath);
    void repoFailed();
};
