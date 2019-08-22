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
#include "AppShortcuts.h"
#include "AppShortcut.h"
#include <App/AppSettings.h>
#include <ApxApp.h>
#include <ApxDirs.h>
#include <QKeySequence>
#include <QQmlEngine>
//=============================================================================
AppShortcuts::AppShortcuts(Fact *parent)
    : Fact(parent, "shortcuts", tr("Shortcuts"), tr("Keyboard hotkeys"), Group)
{
    model()->setFlat(true);

    //setSection("");

    // QML types register
    qmlRegisterUncreatableType<AppShortcuts>("APX.AppShortcuts",
                                             1,
                                             0,
                                             "AppShortcuts",
                                             "Reference only");

    f_add = new AppShortcut(this, this, nullptr, false);
    f_blocked = new Fact(this,
                         "blocked",
                         tr("Block all"),
                         tr("Temporally block all shortcuts"),
                         Bool);
    //f_blocked->setVisible(false);

    QString sect;
    sect = tr("User");

    f_allonUsr = new Fact(this, "allonUsr", tr("Enable all"), tr("Turn on all shortcuts"));
    f_allonUsr->setSection(sect);
    f_alloffUsr = new Fact(this, "alloffUsr", tr("Disable all"), tr("Turn off all shortcuts"));
    f_alloffUsr->setSection(sect);
    f_usr = new Fact(this, "user", sect, tr("User defined shortcuts"), Section);
    f_usr->setSection(sect);

    sect = tr("System");
    f_allonSys = new Fact(this, "allonSys", tr("Enable all"), tr("Turn on all shortcuts"));
    f_allonSys->setSection(sect);
    f_alloffSys = new Fact(this, "alloffSys", tr("Disable all"), tr("Turn off all shortcuts"));
    f_alloffSys->setSection(sect);
    f_sys = new Fact(this, "system", sect, tr("System default shortcuts"), Section);
    f_sys->setSection(sect);

    load();

    saveTimer.setSingleShot(true);
    saveTimer.setInterval(500);
    connect(&saveTimer, &QTimer::timeout, this, &AppShortcuts::saveDo);

    connect(this, &Fact::sizeChanged, this, &AppShortcuts::updateStats);
    //connect(this,&Fact::childValueChanged,this,&AppShortcuts::updateStats);
    updateStats();

    ApxApp::jsync(this);
}
//=============================================================================
void AppShortcuts::updateStats()
{
    bool bSz = f_sys->size();
    f_allonSys->setEnabled(bSz);
    f_alloffSys->setEnabled(bSz);
    bSz = f_usr->size();
    f_allonUsr->setEnabled(bSz);
    f_alloffUsr->setEnabled(bSz);
    //disable f_sys shortcuts found in user
    for (int i = 0; i < f_usr->size(); ++i) {
        AppShortcut *item = f_usr->child<AppShortcut>(i);
        if (item->_enabled->value().toBool() == false)
            continue;
        for (int j = 0; j < f_sys->size(); ++j) {
            AppShortcut *fsys = f_sys->child<AppShortcut>(j);
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
void AppShortcuts::addTriggered()
{
    addUserShortcut();
    saveDo();
    f_add->defaults();
}
//=============================================================================
void AppShortcuts::addUserShortcut()
{
    new AppShortcut(f_usr, this, f_add, true);
}
//=============================================================================
void AppShortcuts::load()
{
    QMap<QString, QJsonObject> msys, musr;
    QStringList lsys, lusr;
    QFile fsys(ApxDirs::res().filePath("templates/shortcuts.json"));
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
    QFile fusr(ApxDirs::prefs().filePath("shortcuts.json"));
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
        new AppShortcut(f_sys, this, f_add, false);
    }

    foreach (QString key, lusr) {
        f_add->defaults();
        f_add->valuesFromJson(musr.value(key));
        addUserShortcut();
    }
    f_add->defaults();
}
void AppShortcuts::save()
{
    saveTimer.start();
}
void AppShortcuts::saveDo()
{
    QJsonArray asys;
    for (int i = 0; i < f_sys->size(); ++i) {
        asys.append(f_sys->child<AppShortcut>(i)->valuesToJson());
    }
    QJsonArray ausr;
    for (int i = 0; i < f_usr->size(); ++i) {
        ausr.append(f_usr->child<AppShortcut>(i)->valuesToJson());
    }
    //save json file
    QFile fusr(ApxDirs::prefs().filePath("shortcuts.json"));
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
QString AppShortcuts::keyToPortableString(int key, int modifier) const
{
    //qDebug()<<key<<modifier;
    if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Meta)
        key = 0;
    QString s = QKeySequence(key | modifier).toString();
    return s;
}
//=============================================================================
