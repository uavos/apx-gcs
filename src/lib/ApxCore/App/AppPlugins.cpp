/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "AppPlugins.h"
#include "AppSettings.h"
#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppLog.h>
//=============================================================================
AppPlugins::AppPlugins(Fact *f_enabled, QObject *parent)
    : QObject(parent)
    , QList<AppPlugin *>()
    , f_enabled(f_enabled)
{
    if (App::dryRun() || App::segfault()) {
        QSettings spt;
        spt.beginGroup("plugins_test");
        for (auto k : spt.allKeys())
            spt.remove(k);
    }
}
AppPlugins::~AppPlugins()
{
    unload();
}
//=============================================================================
void AppPlugins::load(const QStringList &names)
{
    //collect all available plugins filenames
    QStringList allFiles;
    QStringList filters(QStringList() << "*.so"
                                      << "*.dylib"
                                      << "*.bundle"
                                      << "*.qml");

    QDir userp = AppDirs::userPlugins();
    userp.setSorting(QDir::Name | QDir::IgnoreCase);
    userp.refresh();
    if (!userp.exists())
        userp.mkpath(".");
    QStringList stRep, stRepQml;
    foreach (QString fileName, userp.entryList(filters, QDir::Files | QDir::Dirs)) {
        if (fileName.startsWith('-'))
            continue;
        QString pname = QString(fileName);
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
    foreach (QString fileName, pluginsDir.entryList(filters, QDir::Files | QDir::Dirs)) {
        //apxDebug()<<fileName;
        allFiles.append(pluginsDir.absoluteFilePath(fileName));
    }

    /*allFiles.append("qrc:///app/EFIS.qml");
    (void) QT_TRANSLATE_NOOP("Plugins", "EFIS");

    allFiles.append("qrc:///app/PFD.qml");
    (void) QT_TRANSLATE_NOOP("Plugins", "PFD");

    allFiles.append("qrc:///app/HDG.qml");
    (void) QT_TRANSLATE_NOOP("Plugins", "HDG");*/

    //allFiles.append("qrc:///Apx/Controls/video/Video.qml");
    //(void)QT_TRANSLATE_NOOP("Plugins","Video");

    //allFiles.append("qrc:///controls/menu/MenuSys.qml");
    //(void)QT_TRANSLATE_NOOP("Plugins","Facts");

    //allFiles.append("qrc:///instruments/engine/Rotax914.qml");

    //allFiles.append("qrc:///Apx/Controls/signals/Signals.qml");
    //allFiles.append("qrc:///Apx/Controls/state/State.qml");
    //allFiles.append("qrc:///Apx/Controls/terminal/Terminal.qml");

    //allFiles.append("qrc:///app/MenuSys.qml");

    //allFiles.append("qrc:///app/GroundControl.qml");

    //allFiles.append("qrc:///app/Map.qml");
    //allFiles.append("qrc:///Apx/Map/ApxMap.qml");

    //parse command line arguments (plugins to load)
    if (!names.isEmpty()) {
        QStringList st;
        foreach (QString pname, names) {
            foreach (QString fname, allFiles) {
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
    foreach (QString s, allFiles) {
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
}
//=============================================================================
void AppPlugins::fixDuplicates(QStringList &list, const QString &userPluginsPath) const
{
    foreach (QString p, list) {
        if (!p.startsWith(userPluginsPath))
            continue;
        //p is user plugin
        QString pn = QFileInfo(p).baseName();
        foreach (QString ps, list) {
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
//=============================================================================
void AppPlugins::loadFiles(const QStringList &fileNames)
{
    QStringList loadedNames;
    foreach (QString fileName, fileNames) {
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
//=============================================================================
void AppPlugins::unload()
{
    qDeleteAll(*this);
    clear();
}
//=============================================================================
