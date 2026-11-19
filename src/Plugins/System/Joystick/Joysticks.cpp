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
#include "Joysticks.h"
#include <App/App.h>
#include <App/AppDirs.h>

#include "Joystick.h"
#include <QFileDialog>

Joysticks::Joysticks(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Joystick"),
           tr("Hardware input devices"),
           Group | FlatModel)
{
    f_enabled = new Fact(this,
                         "enable",
                         tr("Enable"),
                         tr("Joysticks enable"),
                         Bool | PersistentValue);
    f_enabled->setDefaultValue(true);

    f_export = new Fact(this,
                        "export",
                        tr("Export"),
                        tr("Export configs to file"),
                        Action,
                        "export");
    connect(f_export, &Fact::triggered, this, &Joysticks::exportConfigs);

    f_import = new Fact(this,
                        "import",
                        tr("Import"),
                        tr("Import configs from file"),
                        Action,
                        "import");
    connect(f_import, &Fact::triggered, this, &Joysticks::importConfigs);

    f_list = new Fact(this, "list", tr("Controllers"), "", Section);
    connect(f_list, &Fact::sizeChanged, this, &Joysticks::updateStatus);

    saveEvent.setInterval(1000);
    connect(&saveEvent, &DelayedEvent::triggered, this, &Joysticks::saveConfigs);

    loadConfigs();

    // the backend lives in the main thread and reports through the Qt event loop
    backend = JoystickBackend::create(this);
    if (backend) {
        connect(backend, &JoystickBackend::deviceAdded, this, &Joysticks::deviceAdded);
        connect(backend, &JoystickBackend::deviceRemoved, this, &Joysticks::deviceRemoved);
        connect(backend, &JoystickBackend::axisChanged, this, &Joysticks::axisChanged);
        connect(backend, &JoystickBackend::buttonChanged, this, &Joysticks::buttonChanged);
        connect(backend, &JoystickBackend::hatChanged, this, &Joysticks::hatChanged);
    } else {
        qWarning() << "Joysticks are not supported on this platform";
    }

    connect(f_enabled, &Fact::valueChanged, this, &Joysticks::updateEnabled);
    updateEnabled();
}

void Joysticks::updateStatus()
{
    int acnt = 0, cnt = 0;
    for (int i = 0; i < f_list->size(); ++i) {
        Joystick *j = static_cast<Joystick *>(f_list->child(i));
        if (j->active())
            acnt++;
        if (j->value().toBool())
            cnt++;
    }
    setValue(QString("%1/%2").arg(acnt).arg(cnt));
}
void Joysticks::updateEnabled()
{
    if (!backend)
        return;

    if (f_enabled->value().toBool()) {
        // connected devices are reported through deviceAdded
        if (!backend->start()) {
            qWarning() << "Cannot initialize joysticks";
            return;
        }
        qDebug() << "Joysticks enabled";
    } else {
        backend->stop();
        map.clear();
        for (auto i : f_list->facts()) {
            i->deleteFact();
            delete i;
        }
        //f_list->deleteChildren();
        qDebug() << "Joysticks disabled";
    }
}

void Joysticks::deviceAdded(JoystickDeviceInfo info)
{
    // Find the controller which was connected before, to keep its settings.
    // Controllers of the same model are distinguished by their number.
    Joystick *j = nullptr;
    QSet<int> indexes;
    for (int i = 0; i < f_list->size(); ++i) {
        Joystick *c = static_cast<Joystick *>(f_list->child(i));
        if (c->uid != info.uid || c->devName != info.name)
            continue;
        indexes.insert(c->device_index);
        if (!j && !c->connected() && c->matches(info))
            j = c;
    }

    if (j) {
        j->setConnected(info.id);
    } else {
        int index = 0;
        while (indexes.contains(index))
            index++;
        j = addJoystick(info, index);
    }
    map.insert(info.id, j);
    updateStatus();

    apxMsg() << QString("%1 (js%2)").arg(j->devName).arg(j->device_index);
}

void Joysticks::deviceRemoved(int id)
{
    Joystick *j = map.take(id);
    if (!j)
        return;
    apxMsgW() << QString("%1 (js%2)").arg(j->devName).arg(j->device_index);
    j->setDisconnected();
}

void Joysticks::axisChanged(int id, int axis, qreal value)
{
    Joystick *j = map.value(id);
    if (j && j->active())
        j->updateAxis(axis, value);
}
void Joysticks::buttonChanged(int id, int button, bool pressed)
{
    Joystick *j = map.value(id);
    if (j && j->active())
        j->updateButton(button, pressed);
}
void Joysticks::hatChanged(int id, int hat, quint8 mask)
{
    Joystick *j = map.value(id);
    if (j && j->active())
        j->updateHat(hat, mask);
}

Joystick *Joysticks::addJoystick(const JoystickDeviceInfo &info, int device_index)
{
    Joystick *j = new Joystick(f_list, info, device_index);
    connect(j, &Fact::activeChanged, this, &Joysticks::updateStatus);
    connect(j, &Fact::valueChanged, this, &Joysticks::updateStatus);
    updateStatus();
    updateConfEnums();
    //find and load config
    QString juid = j->juid();
    if (configIds.contains(juid)) {
        j->loadConfig(configs.at(configIds.indexOf(juid)));
    }
    //show matching config in the selection box
    QString confTitle = j->f_title->text();
    if (!confTitle.isEmpty()) {
        if (!j->devName.isEmpty())
            confTitle.append(" - ").append(j->devName);
        if (configTitles.contains(confTitle)) {
            _updatingEnums = true;
            j->f_conf->setValue(confTitle);
            _updatingEnums = false;
        }
    }
    connect(j->f_conf, &Fact::valueChanged, this, [this, j]() {
        if (_updatingEnums)
            return;
        QString s = j->f_conf->text();
        if (configTitles.contains(s)) {
            //qDebug()<<s<<confTitles.value(s);
            j->loadConfig(configs.at(configTitles.value(s)));
        }
    });
    connect(j, &Joystick::save, this, [this, j]() {
        QString s = j->juid();
        if (configIds.contains(s)) {
            int i = configIds.indexOf(s);
            configs[i] = j->saveConfig();
        } else {
            configs.append(j->saveConfig());
            configIds.append(s);
        }
        saveEvent.schedule();
    });
    connect(j->f_save, &Fact::triggered, this, [this, j]() {
        QJsonObject config = j->saveConfig();
        config.remove("index");
        config["uid"] = "user";
        const QString title = config.value("title").toString().simplified();
        if (title.isEmpty()) {
            apxMsgW() << tr("Joystick config title is empty");
            return;
        }
        //update existing config with the same title, create new only when title changed
        int i = userConfigIndex(title, j->devName);
        if (i >= 0) {
            configs[i] = config;
            apxMsg() << tr("Joystick config updated") << title;
        } else {
            configs.append(config);
            configIds.append(QString("0:%1:user").arg(j->devName));
            apxMsg() << tr("Joystick config created") << title;
        }
        saveConfigs();
        //show the saved config in the selection box
        QString confTitle = title;
        if (!j->devName.isEmpty())
            confTitle.append(" - ").append(j->devName);
        if (configTitles.contains(confTitle)) {
            _updatingEnums = true;
            j->f_conf->setValue(confTitle);
            _updatingEnums = false;
        }
    });
    connect(j->f_remove, &Fact::triggered, this, [this, j]() {
        const QString title = j->f_title->text().simplified();
        int i = userConfigIndex(title, j->devName);
        if (i < 0)
            i = userConfigIndex(title, QString());
        if (i < 0) {
            apxMsgW() << tr("Joystick config not found") << title;
            return;
        }
        configs.removeAt(i);
        configIds.removeAt(i);
        j->loadConfig(QJsonObject()); //clear fields
        apxMsg() << tr("Joystick config removed") << title;
        saveConfigs();
    });

    return j;
}

int Joysticks::userConfigIndex(const QString &title, const QString &name)
{
    for (int i = 0; i < configs.size(); ++i) {
        const auto &c = configs.at(i);
        if (c.value("uid").toString() != "user")
            continue;
        if (c.value("title").toString() != title)
            continue;
        if (!name.isEmpty() && c.value("name").toString() != name)
            continue;
        return i;
    }
    return -1;
}

void Joysticks::updateConfEnums()
{
    _updatingEnums = true;
    for (int i = 0; i < f_list->size(); ++i) {
        Joystick *j = static_cast<Joystick *>(f_list->child(i));
        const QString cur = j->f_conf->text();
        j->f_conf->setEnumStrings(QStringList() << "" << configTitles.keys());
        //keep current selection if the config still exists
        if (!cur.isEmpty() && configTitles.contains(cur))
            j->f_conf->setValue(cur);
        else
            j->f_conf->setValue(0);
    }
    _updatingEnums = false;
}

void Joysticks::loadConfigs()
{
    configs.clear();
    configIds.clear();
    configTitles.clear();
    for (int pass = 0; pass < 2; ++pass) {
        QFile file(pass == 0 ? AppDirs::res().filePath("templates/joystick.json")
                             : AppDirs::prefs().filePath("joystick.json"));
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QJsonDocument json = QJsonDocument::fromJson(file.readAll());
            file.close();
            for (const auto i : json["configs"].toArray()) {
                const auto jso = i.toObject();
                QString uid = jso.value("uid").toString();
                QString name = jso.value("name").toString();
                int index = jso.value("index").toVariant().toInt();
                QString jkey = QString("%1:%2:%3").arg(index).arg(name).arg(uid);

                //collapse duplicate user configs (same title and name) - keep last saved
                if (uid == "user") {
                    int prev = -1;
                    for (int k = 0; k < configs.size(); ++k) {
                        const auto &c = configs.at(k);
                        if (c.value("uid").toString() != "user")
                            continue;
                        if (c.value("title").toString() != jso.value("title").toString())
                            continue;
                        if (c.value("name").toString() != name)
                            continue;
                        prev = k;
                        break;
                    }
                    if (prev >= 0) {
                        configs[prev] = jso;
                        continue;
                    }
                }

                configs.append(jso);
                configIds.append(jkey);

                //device state configs (uid is hardware GUID) are not selectable
                if (!uid.isEmpty() && uid != "user")
                    continue;

                QString confTitle = jso.value("title").toString();
                if (!name.isEmpty())
                    confTitle.append(" - ").append(name);
                QString suffix;
                int n = 1;
                while (configTitles.contains(confTitle + suffix)) {
                    suffix = QString(" %1").arg(n++);
                }
                confTitle.append(suffix);
                configTitles.insert(confTitle, configs.size() - 1);
            }
        }
    }
    updateConfEnums();
}

void Joysticks::saveConfigs()
{
    QJsonObject json;
    QJsonArray a;
    for (const auto &i : configs) {
        if (i.value("uid").isUndefined())
            continue;
        a.append(i);
    }
    json.insert("configs", a);

    QFile file(AppDirs::prefs().filePath("joystick.json"));
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        apxMsgW() << file.errorString();
        return;
    }
    file.write(QJsonDocument(json).toJson());
    file.close();
    loadConfigs();
}

void Joysticks::exportConfigs()
{
    QJsonArray a;
    for (const auto &i : configs) {
        if (i.value("uid").isUndefined())
            continue;
        a.append(i);
    }
    if (a.isEmpty()) {
        apxMsgW() << tr("No joystick configs to export");
        return;
    }
    QString path = QFileDialog::getSaveFileName(nullptr,
                                                tr("Export joystick configs"),
                                                AppDirs::user().filePath("joystick.json"),
                                                "JSON (*.json)");
    if (path.isEmpty())
        return;
    if (!path.endsWith(".json", Qt::CaseInsensitive))
        path.append(".json");

    QJsonObject json;
    json.insert("configs", a);
    QFile file(path);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        apxMsgW() << file.errorString();
        return;
    }
    file.write(QJsonDocument(json).toJson());
    file.close();
    apxMsg() << tr("Joystick configs exported") << QString("(%1)").arg(a.size());
}

void Joysticks::importConfigs()
{
    QString path = QFileDialog::getOpenFileName(nullptr,
                                                tr("Import joystick configs"),
                                                AppDirs::user().canonicalPath(),
                                                "JSON (*.json)");
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        apxMsgW() << file.errorString();
        return;
    }
    QJsonDocument json = QJsonDocument::fromJson(file.readAll());
    file.close();

    int cnt = 0;
    for (const auto jsv : json["configs"].toArray()) {
        auto jso = jsv.toObject();
        if (jso.value("config").toObject().isEmpty())
            continue;
        if (jso.value("uid").toString().isEmpty())
            jso["uid"] = "user";
        const QString uid = jso.value("uid").toString();
        const QString title = jso.value("title").toString();
        const QString name = jso.value("name").toString();
        //replace existing config with the same identity, append new otherwise
        int idx = -1;
        for (int i = 0; i < configs.size(); ++i) {
            const auto &c = configs.at(i);
            if (c.value("uid").toString() != uid)
                continue;
            if (c.value("title").toString() != title)
                continue;
            if (c.value("name").toString() != name)
                continue;
            idx = i;
            break;
        }
        if (idx >= 0) {
            configs[idx] = jso;
        } else {
            configs.append(jso);
            configIds.append(QString("%1:%2:%3")
                                 .arg(jso.value("index").toVariant().toInt())
                                 .arg(name)
                                 .arg(uid));
        }
        cnt++;
    }
    if (!cnt) {
        apxMsgW() << tr("No joystick configs found in file");
        return;
    }
    apxMsg() << tr("Joystick configs imported") << QString("(%1)").arg(cnt);
    saveConfigs();
}
