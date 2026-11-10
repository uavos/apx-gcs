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

    // third party QML plugins (sources) installed from GitHub into AppDirs::githubPlugins()/<repo>
    // find <Name>Plugin.qml files in dir and its subfolders (depth levels)
    static QStringList findQmlPlugins(const QDir &dir, int depth = 2);
    // plugin name from file name (Emip.so, Emip.qml, github/<repo>/Emip/EmipPlugin.qml -> Emip)
    static QString pluginName(const QString &fileName);
    // create plugin entry, nullptr when a plugin with the same name exists
    AppPlugin *addPlugin(const QString &fileName);
    // add and load plugins found in the installed repository folder
    void loadGithubRepo(const QString &path);
    // remove plugin entry (not loaded plugins only)
    void removePlugin(AppPlugin *plugin);

    Fact *f_enabled;

    const QFileInfo check_tool;

private:
    void loadFiles(const QStringList &fileNames);

    void fixDuplicates(QStringList &list, const QString &userPluginsPath) const;

public slots:
    void updateStatus();

signals:
    void loaded();

    void loadedTool(AppPlugin *plugin);
    void loadedWindow(AppPlugin *plugin);
    void loadedControl(AppPlugin *plugin);
};
