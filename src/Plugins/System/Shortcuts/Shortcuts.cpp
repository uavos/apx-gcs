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
#include "Shortcuts.h"
#include "Shortcut.h"
#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppSettings.h>
#include <QKeySequence>
#include <QQmlEngine>
//=============================================================================
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
    f_usr = new Fact(this, "user", sect, tr("User defined shortcuts"), Section);
    f_usr->setSection(sect);

    sect = tr("System");
    f_allonSys = new Fact(this, "allon_sys", tr("Enable all"), tr("Turn on all shortcuts"));
    f_allonSys->setSection(sect);
    f_alloffSys = new Fact(this, "alloff_sys", tr("Disable all"), tr("Turn off all shortcuts"));
    f_alloffSys->setSection(sect);
    f_sys = new Fact(this, "system", sect, tr("System default shortcuts"), Section);
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
//=============================================================================
void Shortcuts::updateStats()
{
    bool bSz = f_sys->size();
    f_allonSys->setEnabled(bSz);
    f_alloffSys->setEnabled(bSz);
    bSz = f_usr->size();
    f_allonUsr->setEnabled(bSz);
    f_alloffUsr->setEnabled(bSz);
    //disable f_sys shortcuts found in user
    for (int i = 0; i < f_usr->size(); ++i) {
        Shortcut *item = f_usr->child<Shortcut>(i);
        if (item->_enabled->value().toBool() == false)
            continue;
        for (int j = 0; j < f_sys->size(); ++j) {
            Shortcut *fsys = f_sys->child<Shortcut>(j);
            if (fsys->_enabled->value().toBool() == false)
                continue;
            if (fsys->_key->text() == item->_key->text()) {
                apxMsgW() << tr("Duplicate shortcut").append(":") << fsys->_key->text();
                item->_enabled->setValue(false);
            }
        }
    }
}
//=============================================================================
void Shortcuts::addTriggered()
{
    addUserShortcut();
    saveDo();
    f_add->defaults();
}
//=============================================================================
void Shortcuts::addUserShortcut()
{
    new Shortcut(f_usr, this, f_add, true);
}
//=============================================================================
void Shortcuts::load()
{
    QMap<QString, QJsonObject> msys, musr;
    QStringList lsys, lusr;
    QFile fsys(AppDirs::res().filePath("templates/shortcuts.json"));
    if (fsys.open(QFile::ReadOnly | QFile::Text)) {
        QJsonDocument json = QJsonDocument::fromJson(fsys.readAll());
        fsys.close();
        foreach (QJsonValue v, json["system"].toArray()) {
            QString key = v["key"].toString();
            lsys.append(key);
            QJsonObject jso = v.toObject();
            jso["enb"] = true; //default
            msys.insert(key, jso);
        }
    }
    QFile fusr(AppDirs::prefs().filePath("shortcuts.json"));
    if (fusr.exists() && fusr.open(QFile::ReadOnly | QFile::Text)) {
        QJsonDocument json = QJsonDocument::fromJson(fusr.readAll());
        fusr.close();
        foreach (QJsonValue v, json["system"].toArray()) {
            QString key = v["key"].toString();
            if (!msys.contains(key))
                continue;
            QJsonObject jso = msys[key];
            jso["enb"] = v["enb"];
            msys[key] = jso;
        }
        foreach (QJsonValue v, json["user"].toArray()) {
            QString key = v["key"].toString();
            lusr.append(key);
            musr.insert(key, v.toObject());
        }
    }

    foreach (QString key, lsys) {
        f_add->defaults();
        f_add->valuesFromJson(msys.value(key));
        new Shortcut(f_sys, this, f_add, false);
    }

    foreach (QString key, lusr) {
        f_add->defaults();
        f_add->valuesFromJson(musr.value(key));
        addUserShortcut();
    }
    f_add->defaults();
}
void Shortcuts::save()
{
    saveTimer.start();
}
void Shortcuts::saveDo()
{
    QJsonArray asys;
    for (int i = 0; i < f_sys->size(); ++i) {
        asys.append(f_sys->child<Shortcut>(i)->valuesToJson());
    }
    QJsonArray ausr;
    for (int i = 0; i < f_usr->size(); ++i) {
        ausr.append(f_usr->child<Shortcut>(i)->valuesToJson());
    }
    //save json file
    QFile fusr(AppDirs::prefs().filePath("shortcuts.json"));
    if (!fusr.open(QFile::WriteOnly | QFile::Text)) {
        apxMsgW() << fusr.errorString();
        return;
    }
    QJsonObject jso;
    jso.insert("system", asys);
    jso.insert("user", ausr);
    fusr.write(QJsonDocument(jso).toJson());
    fusr.close();
    //qDebug()<<"saved";
}
//=============================================================================
QString Shortcuts::keyToPortableString(int key, int modifier) const
{
    //qDebug()<<key<<modifier;
    if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Meta)
        key = 0;
    QString s = QKeySequence(key | modifier).toString();
    return s;
}
//=============================================================================
