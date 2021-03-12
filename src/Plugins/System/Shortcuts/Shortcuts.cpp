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
#include "Shortcuts.h"
#include "Shortcut.h"
#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppSettings.h>
#include <QKeySequence>
#include <QQmlEngine>

Shortcuts::Shortcuts(Fact *parent)
    : Fact(parent, "shortcuts", tr("Shortcuts"), tr("Keyboard hotkeys"), Group | FlatModel)
{
    // QML types register
    qmlRegisterUncreatableType<Shortcuts>("APX.Shortcuts", 1, 0, "Shortcuts", "Reference only");

    f_add = new Shortcut(this, this, nullptr, false);
    f_blocked = new Fact(this,
                         "blocked",
                         tr("Block all"),
                         tr("Temporally block all shortcuts"),
                         Bool);

    QString sect;
    sect = tr("User");

    f_allonUsr = new Fact(this, "allon_usr", tr("Enable all"), tr("Turn on all shortcuts"));
    f_allonUsr->setSection(sect);
    f_alloffUsr = new Fact(this, "alloff_usr", tr("Disable all"), tr("Turn off all shortcuts"));
    f_alloffUsr->setSection(sect);
    f_usr = new Fact(this, "user", sect, tr("User defined shortcuts"), Section | Count);
    f_usr->setSection(sect);

    sect = tr("System");
    f_allonSys = new Fact(this, "allon_sys", tr("Enable all"), tr("Turn on all shortcuts"));
    f_allonSys->setSection(sect);
    f_alloffSys = new Fact(this, "alloff_sys", tr("Disable all"), tr("Turn off all shortcuts"));
    f_alloffSys->setSection(sect);
    f_sys = new Fact(this, "system", sect, tr("System default shortcuts"), Section | Count);
    f_sys->setSection(sect);

    load();

    saveTimer.setSingleShot(true);
    saveTimer.setInterval(500);
    connect(&saveTimer, &QTimer::timeout, this, &Shortcuts::saveDo);

    connect(this, &Fact::sizeChanged, this, &Shortcuts::updateStats);
    updateStats();

    App::jsync(this);

    loadQml("qrc:/" PLUGIN_NAME "/ShortcutsPlugin.qml");
}

void Shortcuts::updateStats()
{
    bool bSz = f_sys->size();
    f_allonSys->setEnabled(bSz);
    f_alloffSys->setEnabled(bSz);
    bSz = f_usr->size();
    f_allonUsr->setEnabled(bSz);
    f_alloffUsr->setEnabled(bSz);
    //disable f_sys shortcuts found in user
    for (auto i : f_usr->facts()) {
        Shortcut *item = static_cast<Shortcut *>(i);
        if (item->_enabled->value().toBool() == false)
            continue;
        for (auto j : f_sys->facts()) {
            Shortcut *fsys = static_cast<Shortcut *>(j);
            if (fsys->_enabled->value().toBool() == false)
                continue;
            if (fsys->_key->text() == item->_key->text()) {
                apxMsgW() << tr("Duplicate shortcut").append(":") << fsys->_key->text();
                item->_enabled->setValue(false);
            }
        }
    }
}

void Shortcuts::addTriggered()
{
    addUserShortcut();
    saveDo();
    f_add->defaults();
}

void Shortcuts::addUserShortcut()
{
    new Shortcut(f_usr, this, f_add, true);
}

void Shortcuts::load()
{
    QJsonValue json;
    QFile fusr(AppDirs::prefs().filePath("shortcuts.json"));
    if (fusr.exists() && fusr.open(QFile::ReadOnly | QFile::Text)) {
        json = QJsonDocument::fromJson(fusr.readAll()).object();
    }
    fromJson(json);
}

void Shortcuts::save()
{
    saveTimer.start();
}
void Shortcuts::saveDo()
{
    QFile fusr(AppDirs::prefs().filePath("shortcuts.json"));
    if (!fusr.open(QFile::WriteOnly | QFile::Text)) {
        apxMsgW() << fusr.errorString();
        return;
    }
    fusr.write(toJsonDocument().toJson());
    fusr.close();
    //qDebug()<<"saved";
}

QString Shortcuts::keyToPortableString(int key, int modifier) const
{
    //qDebug()<<key<<modifier;
    if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Meta)
        key = 0;
    QString s = QKeySequence(key | modifier).toString();
    return s;
}

QJsonValue Shortcuts::toJson() const
{
    QJsonArray asys;
    for (auto i : f_sys->facts()) {
        asys.append(static_cast<Shortcut *>(i)->toJson());
    }
    QJsonArray ausr;
    for (auto i : f_usr->facts()) {
        ausr.append(static_cast<Shortcut *>(i)->toJson());
    }
    QJsonObject json;
    json.insert("system", asys);
    json.insert("user", ausr);
    return json;
}

void Shortcuts::fromJson(const QJsonValue json)
{
    f_sys->deleteChildren();
    f_usr->deleteChildren();
    QMap<QString, QJsonObject> msys, musr;
    QStringList lsys, lusr;
    QFile fsys(AppDirs::res().filePath("templates/shortcuts.json"));
    if (fsys.open(QFile::ReadOnly | QFile::Text)) {
        QJsonDocument jsys = QJsonDocument::fromJson(fsys.readAll());
        fsys.close();
        for (auto v : jsys["system"].toArray()) {
            QString key = v.toObject()["key"].toString();
            lsys.append(key);
            QJsonObject jso = v.toObject();
            jso["enb"] = true; //default
            msys.insert(key, jso);
        }
    }
    for (auto v : json["system"].toArray()) {
        QString key = v.toObject()["key"].toString();
        if (!msys.contains(key))
            continue;
        QJsonObject jso = msys[key];
        jso["enb"] = v.toObject()["enb"];
        msys[key] = jso;
    }
    for (auto v : json["user"].toArray()) {
        QString key = v.toObject()["key"].toString();
        lusr.append(key);
        musr.insert(key, v.toObject());
    }

    for (auto key : lsys) {
        f_add->defaults();
        f_add->fromJson(msys.value(key));
        new Shortcut(f_sys, this, f_add, false);
    }

    for (auto key : lusr) {
        f_add->defaults();
        f_add->fromJson(musr.value(key));
        addUserShortcut();
    }
    f_add->defaults();
}
