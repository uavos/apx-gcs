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
#include "AppPlugin.h"
#include "AppPlugins.h"

#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppLog.h>

#include <QCryptographicHash>
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
    Fact *f = new Fact(plugins->f_enabled,
                       name.toLower(),
                       name,
                       "",
                       Fact::Bool | Fact::PersistentValue);
    f->setDefaultValue(true);
    App::jsync(plugins->f_enabled);
    f_enabled = f;
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
    QCoreApplication::processEvents();

    if (!checkLib(fname))
        return;

    QObject *instance = nullptr;
    PluginInterface *p = nullptr;
    QLibrary lib(fname);
    try {
        if (!lib.load()) {
            apxMsgW() << "lib-load:" << lib.errorString() << "(" + fname + ")";
            return;
        }
        loader = new QPluginLoader(fname);
        instance = loader->instance();
    } catch (...) {
        apxMsgW() << "Plugin load error" << name << "(" + fname + ")";
        instance = nullptr;
    }
    if (!instance) {
        //try to load pure qml qrc plugin
        if (!lib.isLoaded()) {
            return;
        }
        QString qrcFileName = QString(":/%1/%1Plugin.qml").arg(name);
        if (QFile::exists(qrcFileName)) {
            fileName = "qrc" + qrcFileName;
            loadQml();
            return;
        }
        if (loader) {
            apxMsgW() << "loader:" << loader->errorString() << "(" + fname + ")";
        }
        return;
    }
    p = reinterpret_cast<PluginInterface *>(instance);
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
    switch (p->flags() & PluginInterface::PluginSectionMask) {
    default:
        section = tr("Other");
        break;
    case PluginInterface::System:
        section = tr("System features");
        break;
    case PluginInterface::Tool:
        section = tr("Tools");
        break;
    case PluginInterface::Map:
        section = tr("Mission");
        break;
    }

    //initialize lib
    p->init();
    QString title = p->title();
    QString descr = p->descr();
    switch (p->flags() & PluginInterface::PluginTypeMask) {
    default:
        break;
    case PluginInterface::Feature: {
        control = p->createControl();
        Fact *f = qobject_cast<Fact *>(control);
        if (f) {
            if (title.isEmpty())
                title = f->title();
            if (descr.isEmpty())
                descr = f->descr();
        } else {
        }
        f_enabled->setSection(section);
        if (!descr.isEmpty())
            descr.prepend(tr("Tool").append(": "));
        plugins->loadedTool(this);
    } break;
    case PluginInterface::Widget: {
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
    if (QFileInfo(fileName).completeBaseName().contains("Plugin")) {
        QVariantMap opts;
        opts.insert("name", name.toLower());
        App::instance()->engine()->loadQml(fileName, opts);
    } else {
        Fact *f = new Fact(nullptr, name.toLower(), name, "", Fact::Group);
        f->setQmlPage(fileName);
        control = f;
    }
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
QString AppPlugin::errorString()
{
    return m_errorString;
}
//=============================================================================
bool AppPlugin::checkLib(const QString &fname)
{
    QFileInfo tool(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("gcs_plugin_test"));
    if (!tool.exists()) {
        //qWarning() << "missing tool" << tool.absoluteFilePath();
        return true;
    }

    QSettings spt;
    spt.beginGroup("plugins_test");
    QCryptographicHash hash(QCryptographicHash::Sha1);
    QFileInfo fi(fname);
    hash.addData(fname.toUtf8());
    hash.addData(fi.filePath().toUtf8());
    hash.addData(fi.lastModified().toString().toUtf8());
    hash.addData(tool.filePath().toUtf8());
    hash.addData(tool.lastModified().toString().toUtf8());
    QString sptKey = hash.result().toHex().toUpper();
    if (spt.value(sptKey).toString() == name) {
        //qDebug() << "already checked" << name << sptKey;
        return true;
    }

    QCoreApplication::processEvents();
    QCoreApplication::processEvents();
    QProcess proc;
    proc.start(tool.absoluteFilePath(), QStringList() << fname);
    if (!proc.waitForStarted())
        return false;
    if (!proc.waitForFinished())
        return false;
    if (proc.exitCode() != 0) {
        m_errorString = proc.readAllStandardError();
        apxMsgW() << "Error loading plugin:" << name;
        apxMsgW() << m_errorString;
        f_enabled->setStatus(tr("error").toUpper());
        return false;
    }
    spt.setValue(sptKey, name);
    // qDebug() << "checked" << name << sptKey;
    return true;
}
//=============================================================================
