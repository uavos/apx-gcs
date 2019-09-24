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
#include <ApxApp.h>
#include <ApxDirs.h>
#include <ApxLog.h>
//=============================================================================
AppPlugins::AppPlugins(Fact *f_enabled, QObject *parent)
    : QObject(parent)
    , QList<AppPlugin *>()
    , f_enabled(f_enabled)
{}
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

    QDir userp = ApxDirs::userPlugins();
    userp.setSorting(QDir::Name | QDir::IgnoreCase);
    userp.refresh();
    if (!userp.exists())
        userp.mkpath(".");
    QStringList stRep, stRepQml;
    foreach (QString fileName, userp.entryList(filters, QDir::Files)) {
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

    QDir pluginsDir = ApxDirs::plugins();
    pluginsDir.setSorting(QDir::Name | QDir::IgnoreCase);
    pluginsDir.refresh();
    //apxDebug()<<pluginsDir;
    foreach (QString fileName, pluginsDir.entryList(filters, QDir::Files | QDir::Dirs)) {
        //apxDebug()<<fileName;
        allFiles.append(pluginsDir.absoluteFilePath(fileName));
    }

    allFiles.append("qrc:///app/EFIS.qml");
    (void) QT_TRANSLATE_NOOP("Plugins", "EFIS");

    allFiles.append("qrc:///app/PFD.qml");
    (void) QT_TRANSLATE_NOOP("Plugins", "PFD");

    allFiles.append("qrc:///app/HDG.qml");
    (void) QT_TRANSLATE_NOOP("Plugins", "HDG");

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
    libFiles.sort();
    qmlFiles.sort();
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
void AppPlugins::loadFiles(const QStringList &fileNames)
{
    QSettings st;
    st.beginGroup("plugins");
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
//=============================================================================
//=============================================================================
AppPlugin::AppPlugin(AppPlugins *plugins, QString name, QString fileName)
    : QObject(plugins)
    , plugins(plugins)
    , name(name)
    , fileName(fileName)
    , f_enabled(nullptr)
    , interface(nullptr)
    , control(nullptr)
    , loader(nullptr)
{
    AppSettingFact *f = new AppSettingFact(AppSettings::settings(),
                                           plugins->f_enabled,
                                           name.toLower(),
                                           name,
                                           "",
                                           Fact::Bool,
                                           true);
    ApxApp::jsync(plugins->f_enabled);
    f_enabled = f;
    f->load();
    connect(f, &Fact::valueChanged, this, &AppPlugin::enabledChanged);
}
AppPlugin::~AppPlugin()
{
    unload();
}
//=============================================================================
void AppPlugin::loadLib()
{
    QString fname = fileName;
    if (fname.endsWith(".bundle")) {
        fname += "/Contents/MacOS/" + QFileInfo(fname).baseName();
    }
    //load lib
    apxConsole() << tr("Loading").append(":") << name;
    loader = new QPluginLoader(fname);
    QObject *instance = nullptr;
    try {
        instance = loader->instance();
    } catch (...) {
        apxMsgW() << "Plugin load error" << name << "(" + fname + ")";
    }
    if (!instance) {
        apxMsgW() << "QPluginLoader" << loader->errorString() << "(" + fname + ")";
        return;
    }
    ApxPluginInterface *p = reinterpret_cast<ApxPluginInterface *>(instance);
    interface = p;
    if (!f_enabled->value().toBool())
        return;

    //check depends
    depends = p->depends();
    if (!depends.isEmpty()) {
        bool ok = false;
        //apxConsole() << tr("Deps").append(":") << depends;
        for (auto d : *plugins) {
            if (!depends.contains(d->name))
                continue;
            ok = true;
            apxConsole() << tr("Depends").append(":") << d->name;
            d->load();
        }
        if (!ok) {
            apxConsoleW() << tr("Dependency not found").append(":") << depends;
        }
        apxConsole() << tr("Initializing").append(":") << name;
    }

    //define section
    switch (p->flags() & ApxPluginInterface::PluginSectionMask) {
    default:
        section = tr("Other");
        break;
    case ApxPluginInterface::System:
        section = tr("System features");
        break;
    case ApxPluginInterface::Tool:
        section = tr("Tools");
        break;
    case ApxPluginInterface::Map:
        section = tr("Map view features");
        break;
    }

    //initialize lib
    p->init();
    QString title = p->title();
    QString descr = p->descr();
    switch (p->flags() & ApxPluginInterface::PluginTypeMask) {
    default:
        break;
    case ApxPluginInterface::Feature: {
        control = p->createControl();
        Fact *f = qobject_cast<Fact *>(control);
        if (!f)
            break;
        if (title.isEmpty())
            title = f->title();
        if (descr.isEmpty())
            descr = f->descr();
        f_enabled->setSection(section);
        descr.prepend(tr("Tool").append(": "));
        plugins->loadedTool(this);
    } break;
    case ApxPluginInterface::Widget: {
        f_enabled->setSection(tr("Windows"));
        descr.prepend(tr("Window").append(": "));
        plugins->loadedWindow(this);
    } break;
    }
    f_enabled->setTitle(title);
    f_enabled->setDescr(descr);
}
void AppPlugin::loadQml()
{
    f_enabled->setSection(tr("Controls"));
    Fact *f = new Fact(nullptr, name.toLower(), name, "", Fact::Group);
    f->setQmlPage(fileName);
    control = f;
    plugins->loadedControl(this);
}
//=============================================================================
void AppPlugin::load()
{
    if (loader || control)
        return;
    if (fileName.endsWith(".so") || fileName.endsWith(".dylib") || fileName.endsWith(".bundle")) {
        loadLib();
    } else if (fileName.endsWith(".qml")) {
        loadQml();
    }
}
void AppPlugin::unload()
{
    /*Fact *f = qobject_cast<Fact *>(control);
    if (f && f->parentFact())
        f->setParentFact(nullptr);
    if (control) {
        delete control;
    }
    if (loader) {
        loader->unload();
        delete loader;
    }
    loader = nullptr;
    control = nullptr;*/
}
//=============================================================================
void AppPlugin::enabledChanged()
{
    Fact *f = qobject_cast<Fact *>(sender());
    if (!f)
        return;
    if (f->value().toBool()) {
        load();
    } else {
        //unload();
    }
}
//=============================================================================
