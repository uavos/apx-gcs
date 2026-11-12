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
#include "AppPlugins.h"
#include "AppSettings.h"
#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppLog.h>

AppPlugins::AppPlugins(Fact *f_enabled, QObject *parent)
    : QObject(parent)
    , QList<AppPlugin *>()
    , f_enabled(f_enabled)
    , check_tool(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("gcs_plugin_test"))
{
    if (App::dryRun() || App::segfault()) {
        QSettings sx;
        sx.beginGroup("plugins_test");
        for (auto k : sx.allKeys())
            sx.remove(k);
    }

    if (!check_tool.exists()) {
        qWarning() << "missing tool" << check_tool.absoluteFilePath();
    }
}
AppPlugins::~AppPlugins()
{
    unload();
}

void AppPlugins::load(const QStringList &names)
{
    //collect all available plugins filenames
    QStringList allFiles;
    QStringList filters(QStringList() << "*.gcs"
                                      << "*.so"
                                      << "*.dylib"
                                      << "*.bundle"
                                      << "*.qml");

    QDir userp = AppDirs::userPlugins();
    userp.setSorting(QDir::Name | QDir::IgnoreCase);
    userp.refresh();
    if (!userp.exists())
        userp.mkpath(".");
    QStringList stRep, stRepQml;
    for (const auto fileName : userp.entryList(filters, QDir::Files | QDir::Dirs)) {
        if (fileName.startsWith('-'))
            continue;
        QString pname = fileName;
        if (pname.startsWith("lib"))
            pname.remove(0, 3);
        if (!pname.endsWith(".qml")) {
            pname.truncate(pname.lastIndexOf('.'));
            stRep.append(pname);
        } else {
            stRepQml.append(pname.left(pname.lastIndexOf('.')));
        }
        allFiles.append(userp.absoluteFilePath(fileName));
    }
    if (!stRep.isEmpty())
        apxConsole() << QObject::tr("User plugins").append(": ").append(stRep.join(','));
    if (!stRepQml.isEmpty())
        apxConsole() << QObject::tr("User QML plugins").append(": ").append(stRepQml.join(','));

    // third party plugins installed from GitHub: github/<repo>/<Name>.so
    QDir githubDir = AppDirs::githubPlugins();
    QStringList stRepGithub;
    if (githubDir.exists()) {
        for (const auto &repoName :
             githubDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase)) {
            if (repoName.startsWith('-') || repoName.startsWith('.'))
                continue;
            const QString repoPath = githubDir.absoluteFilePath(repoName);
            const auto files = findLibPlugins(QDir(repoPath));
            if (files.isEmpty()) {
                apxConsoleW() << QObject::tr("No plugins found").append(": ") << repoName;
                continue;
            }
            if (!checkGithubRepo(repoPath, files))
                continue;
            for (const auto &f : files) {
                stRepGithub.append(pluginName(f));
                allFiles.append(f);
            }
        }
    }
    if (!stRepGithub.isEmpty())
        apxConsole() << QObject::tr("GitHub plugins").append(": ").append(stRepGithub.join(','));

    QDir pluginsDir = AppDirs::plugins();
    pluginsDir.setSorting(QDir::Name | QDir::IgnoreCase);
    pluginsDir.refresh();
    //apxDebug()<<pluginsDir;
    for (const auto fileName : pluginsDir.entryList(filters, QDir::Files | QDir::Dirs)) {
        //apxDebug()<<fileName;
        allFiles.append(pluginsDir.absoluteFilePath(fileName));
    }

    //parse command line arguments (plugins to load)
    if (!names.isEmpty()) {
        QStringList st;
        for (const auto pname : names) {
            for (const auto fname : allFiles) {
                if (pluginName(fname) != pname)
                    continue;
                st.append(fname);
            }
        }
        allFiles.swap(st);
    }

    apxConsole() << QObject::tr("Loading plugins").append("...");

    //load and initialize
    QStringList libFiles, qmlFiles;
    for (const auto s : allFiles) {
        if (s.endsWith(".qml", Qt::CaseInsensitive)) {
            qmlFiles.append(s);
        } else {
            libFiles.append(s);
        }
    }
    //sort by name
    std::sort(libFiles.begin(), libFiles.end(), [](QString a, QString b) {
        return QFileInfo(a).baseName().localeAwareCompare(QFileInfo(b).baseName()) < 0;
    });

    std::sort(qmlFiles.begin(), qmlFiles.end(), [](QString a, QString b) {
        return QFileInfo(a).baseName().localeAwareCompare(QFileInfo(b).baseName()) < 0;
    });

    //remove duplicates (system but keep user)
    fixDuplicates(libFiles, userp.absolutePath());
    fixDuplicates(qmlFiles, userp.absolutePath());

    loadFiles(libFiles);
    loadFiles(qmlFiles);

    for (auto p : *this) {
        if (p->f_enabled->value().toBool()) {
            p->load();
        } else {
            apxConsole() << tr("Loading skipped").append(":") << p->name;
        }
    }

    emit loaded();
    updateStatus();
}

void AppPlugins::fixDuplicates(QStringList &list, const QString &userPluginsPath) const
{
    for (const auto p : list) {
        if (!p.startsWith(userPluginsPath))
            continue;
        //p is user plugin
        QString pn = QFileInfo(p).baseName();
        for (const auto ps : QStringList(list)) {
            if (ps.startsWith(userPluginsPath))
                continue;
            //ps is system plugin
            QString psn = QFileInfo(ps).baseName();
            if (psn == pn) {
                list.removeAll(ps);
                apxConsole() << tr("User plugin override").append(":") << psn;
            }
        }
    }
}

void AppPlugins::loadFiles(const QStringList &fileNames)
{
    for (const auto &fileName : fileNames)
        addPlugin(fileName);
}

AppPlugin *AppPlugins::addPlugin(const QString &fileName)
{
    QString pname = pluginName(fileName);
    if (plugin(pname)) {
        apxMsgW() << tr("Duplicate plugin").append(":") << pname;
        return nullptr;
    }
    AppPlugin *p = new AppPlugin(this, pname, fileName);
    append(p);
    return p;
}

QString AppPlugins::pluginName(const QString &fileName)
{
    QString pname = QFileInfo(fileName).baseName();
    if (pname.startsWith("lib"))
        pname.remove(0, 3);
    return pname;
}

QStringList AppPlugins::findLibPlugins(const QDir &dir)
{
    QStringList files;
    for (const auto &fi : dir.entryInfoList({"*.gcs", "*.so", "*.dylib", "*.bundle"},
                                            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                                            QDir::Name | QDir::IgnoreCase)) {
        if (fi.fileName().startsWith('-'))
            continue;
        files.append(fi.absoluteFilePath());
    }
    return files;
}

QVersionNumber AppPlugins::pluginVersion(const QString &tag)
{
    // release-11.2.32.1, v11.2.32.1, 11.2.32.1
    QString s = tag.trimmed();
    s = s.mid(s.lastIndexOf('-') + 1);
    if (s.startsWith('v', Qt::CaseInsensitive))
        s.remove(0, 1);
    qsizetype idx = 0;
    const auto v = QVersionNumber::fromString(s, &idx);
    if (idx != s.size() || v.segmentCount() < 3)
        return {};
    return v;
}

bool AppPlugins::isCompatible(const QVersionNumber &version)
{
    if (version.segmentCount() < 3)
        return false;
    const auto app = QVersionNumber::fromString(App::version());
    if (app.segmentCount() < 3)
        return false;
    return QVersionNumber(version.majorVersion(), version.minorVersion(), version.microVersion())
           == QVersionNumber(app.majorVersion(), app.minorVersion(), app.microVersion());
}

QJsonObject AppPlugins::githubRepoInfo(const QString &repoPath)
{
    const QDir d(repoPath);
    QFile f(d.absoluteFilePath(d.dirName() + ".json"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QJsonDocument::fromJson(f.readAll()).object();
}

bool AppPlugins::checkGithubRepo(const QString &repoPath, const QStringList &files)
{
    const QString tag = githubRepoInfo(repoPath).value("tag").toString();
    if (isCompatible(pluginVersion(tag)))
        return true;
    for (const auto &f : files) {
        apxMsgW() << tr("Plugin disabled").append(":") << pluginName(f) << tag
                  << tr("(requires release %1.x)").arg(App::version());
    }
    return false;
}

void AppPlugins::loadGithubRepo(const QString &path)
{
    const auto files = findLibPlugins(QDir(path));
    if (!checkGithubRepo(path, files))
        return;
    for (const auto &f : files) {
        AppPlugin *p = addPlugin(f);
        if (!p)
            continue;
        if (p->f_enabled->value().toBool()) {
            p->load();
        } else {
            apxConsole() << tr("Loading skipped").append(":") << p->name;
        }
    }
    updateStatus();
}

void AppPlugins::removePlugin(AppPlugin *plugin)
{
    if (!plugin || plugin->isLoaded())
        return;
    removeAll(plugin);
    if (plugin->f_enabled) {
        plugin->f_enabled->deleteFact();
        plugin->f_enabled = nullptr;
    }
    plugin->deleteLater();
    updateStatus();
}

void AppPlugins::unload()
{
    qDeleteAll(*this);
    clear();
}

AppPlugin *AppPlugins::plugin(QString name)
{
    for (auto p : *this) {
        if (p->name == name)
            return p;
    }
    return nullptr;
}

void AppPlugins::updateStatus()
{
    if (!f_enabled)
        return;
    size_t cnt = 0, ecnt = 0;
    for (auto f : *this) {
        cnt++;
        if (f->f_enabled->value().toBool())
            ecnt++;
    }
    f_enabled->setValue(QString("%1/%2").arg(ecnt).arg(cnt));
}
