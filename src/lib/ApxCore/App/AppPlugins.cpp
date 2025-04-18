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
                QString s = QFileInfo(fname).baseName();
                if (s.startsWith("lib"))
                    s.remove(0, 3);
                if (s != pname)
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
        for (const auto ps : list) {
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
    QStringList loadedNames;
    for (const auto fileName : fileNames) {
        QString pname = QFileInfo(fileName).baseName();
        if (pname.startsWith("lib"))
            pname.remove(0, 3);
        if (loadedNames.contains(pname)) {
            apxMsgW() << tr("Duplicate plugin").append(":") << pname;
            continue;
        }
        //apxConsole()<<tr("Loading")<<pname;
        loadedNames.append(pname);

        AppPlugin *plugin = new AppPlugin(this, pname, fileName);
        append(plugin);
    }
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
