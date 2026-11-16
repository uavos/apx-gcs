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

#include <App/PluginInterface.h>
#include <Fact/Fact.h>
#include <QtCore>

#include "AppPlugin.h"

class AppPlugins : public QObject, public QList<AppPlugin *>
{
    Q_OBJECT
public:
    explicit AppPlugins(Fact *f_enabled, QObject *parent = nullptr);
    ~AppPlugins();

    void load(const QStringList &names = QStringList());
    void unload();

    AppPlugin *plugin(QString name);

    // third party plugins installed from GitHub into AppDirs::githubPlugins()/<repo>/<Name>.so
    // find plugin libraries (*.so, *.dylib, *.gcs, *.bundle) in dir
    static QStringList findLibPlugins(const QDir &dir);
    // plugin name from file name (Emip.so, libEmip.so, Emip.qml -> Emip)
    static QString pluginName(const QString &fileName);
    // create plugin entry, nullptr when a plugin with the same name exists
    AppPlugin *addPlugin(const QString &fileName);
    // add and load plugin libraries found in the installed repository folder
    void loadGithubRepo(const QString &path);
    // remove plugin entry (not loaded plugins only)
    void removePlugin(AppPlugin *plugin);

    // GitHub plugin releases are tagged with the application version plus the plugin build number
    // ([v]11.2.32.1 for GCS 11.2.32); a plugin is loaded only when the versions match
    static QVersionNumber pluginVersion(const QString &tag);
    static bool isCompatible(const QVersionNumber &version);
    // release info of the installed repository folder (<repo>/<repo>.json)
    static QJsonObject githubRepoInfo(const QString &repoPath);

    Fact *f_enabled;

    const QFileInfo check_tool;

private:
    void loadFiles(const QStringList &fileNames);
    // the installed release matches the application version, reports disabled plugins otherwise
    static bool checkGithubRepo(const QString &repoPath, const QStringList &files);

    void fixDuplicates(QStringList &list, const QString &userPluginsPath) const;

public slots:
    void updateStatus();

signals:
    void loaded();

    void loadedTool(AppPlugin *plugin);
    void loadedWindow(AppPlugin *plugin);
    void loadedControl(AppPlugin *plugin);
};
