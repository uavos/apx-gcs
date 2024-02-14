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
#include "AppPlugin.h"
#include "AppPlugins.h"

#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppLog.h>

#include <QCryptographicHash>

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
    connect(f, &Fact::valueChanged, plugins, &AppPlugins::updateStatus);
}
AppPlugin::~AppPlugin()
{
    unload();
}

void AppPlugin::loadLib()
{
    QString fname = fileName;
    if (fname.endsWith(".bundle")) {
        fname += "/Contents/MacOS/" + QFileInfo(fname).baseName();
    }
    //load lib
    apxConsole() << tr("Loading").append(":") << name;
    QCoreApplication::processEvents();

    QSettings sx_blacklist;
    sx_blacklist.beginGroup("plugins_blacklist");

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
    if (sx_blacklist.value(name).toString() == _hash) {
        apxMsgW() << tr("Plugin blacklisted").append(':') << name << "(" + fname + ")";
        return;
    }

    sx_blacklist.setValue(name, _hash);
    sx_blacklist.sync();

    try {
        p = qobject_cast<PluginInterface *>(instance);
        interface = p;

        if (!interface)
            return;

        if (!f_enabled->value().toBool())
            return;

        //check depends
        depends = p->depends();
        if (!depends.isEmpty()) {
            bool ok = false;
            for (auto d : *plugins) {
                if (!depends.contains(d->name))
                    continue;
                ok = true;
                apxConsole() << tr("Depends").append(":") << d->name;
                d->load();
            }
            if (!ok) {
                apxMsgW() << tr("Dependency not found").append(":") << depends;
            }
            apxConsole() << tr("Initializing").append(":") << name;
        }

        //define section
        switch (p->flags() & PluginInterface::PluginSectionMask) {
        case PluginInterface::System:
            section = tr("System features");
            break;
        case PluginInterface::Tool:
            section = tr("Tools");
            break;
        case PluginInterface::Map:
            section = tr("Mission");
            break;
        default:
            section = tr("Other");
            break;
        }

        //initialize lib
        p->init();
        QString title = p->title();
        QString descr = p->descr();
        switch (p->flags() & PluginInterface::PluginTypeMask) {
        case PluginInterface::Feature: {
            control = p->createControl();
            Fact *f = qobject_cast<Fact *>(control);
            if (f) {
                if (title.isEmpty())
                    title = f->title();
                if (descr.isEmpty())
                    descr = f->descr();
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
        default:
            break;
        }

        f_enabled->setTitle(title);
        f_enabled->setDescr(descr);

        sx_blacklist.remove(name);
        sx_blacklist.sync();
    } catch (const std::exception &e) {
        apxMsgW() << "Error loading plugin " << name << ": " << e.what();
    }
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
        f->setOpt("page", fileName);
        control = f;
    }
    plugins->loadedControl(this);
}

void AppPlugin::load()
{
    if (loader || control)
        return;
    if (fileName.endsWith(".gcs") || fileName.endsWith(".so") || fileName.endsWith(".dylib")
        || fileName.endsWith(".bundle")) {
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

void AppPlugin::enabledChanged()
{
    Fact *f = qobject_cast<Fact *>(sender());
    if (!f)
        return;
    if (f->value().toBool()) {
        load();
    } else {
        //unload();

        // remove from blacklist
        QSettings sx_blacklist;
        sx_blacklist.beginGroup("plugins_blacklist");
        if (sx_blacklist.value(name).toString() == _hash) {
            apxMsgW() << tr("Plugin blacklist removed").append(':') << name;
        }
        sx_blacklist.remove(name);
        sx_blacklist.sync();
    }
}

bool AppPlugin::checkLib(const QString &fname)
{
    QFileInfo tool(plugins->check_tool);
    if (!tool.exists()) {
        qWarning() << "no tool:" << tool.absoluteFilePath();
        return true;
    }

    QSettings sx;
    sx.beginGroup("plugins_test");
    QCryptographicHash h(QCryptographicHash::Sha1);
    QFileInfo fi(fname);
    h.addData(fname.toUtf8());
    h.addData(fi.filePath().toUtf8());
    h.addData(fi.lastModified().toString().toUtf8());
    h.addData(tool.filePath().toUtf8());
    h.addData(tool.lastModified().toString().toUtf8());
    _hash = h.result().toHex().toUpper();
    if (sx.value(name).toString() == _hash) {
        // qDebug() << "already checked" << name << hash;
        return true;
    }

    sx.remove(name);

    // qDebug() << "checking:" << tool.absoluteFilePath();

    QCoreApplication::processEvents();
    QCoreApplication::processEvents();
    QProcess proc;
    proc.start(tool.absoluteFilePath(), QStringList() << fname);
    if (!proc.waitForStarted())
        return false;
    if (!proc.waitForFinished())
        return false;
    if (proc.exitCode() != 0) {
        _errorString = proc.readAllStandardError();
        apxMsgW() << "Error loading plugin:" << name;
        apxMsgW() << _errorString;
        f_enabled->setTitle(QString("%1 (%2)").arg(f_enabled->title()).arg(tr("error").toUpper()));
        return false;
    }
    sx.setValue(name, _hash);
    // qDebug() << "checked" << name << hash;
    return true;
}
