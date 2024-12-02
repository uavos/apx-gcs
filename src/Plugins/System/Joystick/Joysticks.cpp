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
#include <SDL.h>
#include <QtConcurrent>

static int sdlWaitEvent()
{
    return SDL_WaitEventTimeout(nullptr, 500);
}

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

    f_list = new Fact(this, "list", tr("Controllers"), "", Section);
    connect(f_list, &Fact::sizeChanged, this, &Joysticks::updateStatus);

    saveEvent.setInterval(1000);
    connect(&saveEvent, &DelayedEvent::triggered, this, &Joysticks::saveConfigs);

    loadConfigs();

    timer.setInterval(50);
    //connect(&timer,&QTimer::timeout,this,&Joysticks::update);
    //timer.start();
    connect(&watcher, &QFutureWatcher<int>::finished, this, &Joysticks::watcherFinished);

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
    if (f_enabled->value().toBool()) {
        qDebug() << "SDL init...";
        if (SDL_InitSubSystem(SDL_INIT_JOYSTICK)) {
            qDebug() << "Cannot initialize SDL:" << SDL_GetError();
            return;
        }
        scan();
        waitEvent();
        qDebug() << "SDL initialized";
    } else {
        foreach (auto i, f_list->facts()) {
            i->deleteFact();
            delete i;
        }
        //f_list->deleteChildren();
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
        qDebug() << "Joysticks disabled";
    }
}
void Joysticks::watcherFinished()
{
    if (!f_enabled->value().toBool())
        return;

    //if (watcher.result() == 1) {
    //    update();
    //}

    update();
    waitEvent();
    //qDebug()<<watcher.result();
}

void Joysticks::update()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        processEvent(event);
    }
}
void Joysticks::waitEvent()
{
    QFuture<int> future = QtConcurrent::run(sdlWaitEvent);
    watcher.setFuture(future);
}
void Joysticks::processEvent(const SDL_Event &event)
{
    switch (event.type) {
    case SDL_JOYDEVICEADDED: {
        scan();
        int index = event.jdevice.which;

        SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(index);
        QString uid = QByteArray(reinterpret_cast<const char *>(guid.data), sizeof(guid.data))
                          .toHex()
                          .toUpper();
        for (int i = 0; i < f_list->size(); ++i) {
            Joystick *j = static_cast<Joystick *>(f_list->child(i));
            if (j->uid != uid)
                continue;
            j->updateDevice(true);
            break;
        }
        SDL_JoystickID InstanceID = SDL_JoystickGetDeviceInstanceID(index);
        if (map.contains(InstanceID)) {
            apxMsg() << QString("%1 (js%2)")
                            .arg(map.value(InstanceID)->devName)
                            .arg(map.value(InstanceID)->device_index);
        }

    } break;
    case SDL_JOYDEVICEREMOVED: {
        int InstanceID = event.jdevice.which;
        if (map.contains(InstanceID)) {
            apxMsgW() << QString("%1 (js%2)")
                             .arg(map.value(InstanceID)->devName)
                             .arg(map.value(InstanceID)->device_index);
            map.remove(InstanceID);
        }

        SDL_JoystickGUID guid = SDL_JoystickGetGUID(SDL_JoystickFromInstanceID(InstanceID));
        //SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(ID);
        QString uid = QByteArray(reinterpret_cast<const char *>(guid.data), sizeof(guid.data))
                          .toHex()
                          .toUpper();
        for (int i = 0; i < f_list->size(); ++i) {
            Joystick *j = static_cast<Joystick *>(f_list->child(i));
            if (j->uid != uid)
                continue;
            j->updateDevice(false);
            break;
        }
    } break;
    case SDL_JOYAXISMOTION: {
        Joystick *dev = map.value(event.jaxis.which);
        if (dev)
            dev->updateAxis(event.jaxis.axis, static_cast<qreal>(event.jaxis.value) / 32767);
    } break;
    case SDL_JOYBUTTONUP: {
        Joystick *dev = map.value(event.jbutton.which);
        if (dev)
            dev->updateButton(event.jbutton.button, false);
    } break;
    case SDL_JOYBUTTONDOWN: {
        Joystick *dev = map.value(event.jbutton.which);
        if (dev)
            dev->updateButton(event.jbutton.button, true);
    } break;
    case SDL_JOYHATMOTION: {
        Joystick *dev = map.value(event.jhat.which);
        if (dev)
            dev->updateHat(event.jhat.hat, event.jhat.value);
    } break;
    }
}

void Joysticks::scan()
{
    QMap<QString, Joystick *> uids;
    for (int i = 0; i < f_list->size(); ++i) {
        Joystick *j = static_cast<Joystick *>(f_list->child(i));
        uids.insert(j->uid, j);
    }
    map.clear();
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(i);
        QString uid = QByteArray(reinterpret_cast<const char *>(guid.data), sizeof(guid.data))
                          .toHex()
                          .toUpper();
        Joystick *j = uids.value(uid);
        if (!j) {
            j = addJoystick(i, uid);
        } else {
            j->device_index = i;
        }
        if (j->active())
            map.insert(j->instanceID, j);
    }
}

Joystick *Joysticks::addJoystick(int device_index, QString uid)
{
    Joystick *j = new Joystick(f_list, device_index, uid);
    connect(j, &Fact::activeChanged, this, [this, j]() {
        if (j->active())
            map.insert(j->instanceID, j);
        else
            map.remove(j->instanceID);
    });
    connect(j, &Fact::activeChanged, this, &Joysticks::updateStatus);
    connect(j, &Fact::valueChanged, this, &Joysticks::updateStatus);
    updateStatus();
    updateConfEnums();
    //find and load config
    QString juid = j->juid();
    if (configIds.contains(juid)) {
        j->loadConfig(configs.at(configIds.indexOf(juid)));
    }
    connect(j->f_conf, &Fact::valueChanged, this, [this, j]() {
        QString s = j->f_conf->text();
        j->f_conf->setValue(0);
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
        int i = configIndex(config);
        config.remove("index");
        config["uid"] = "user";
        if (i >= 0) {
            configs[i] = config;
        } else {
            configs.append(config);
            configIds.append(QString("0:%1:").arg(j->devName));
        }
        saveEvent.schedule();
    });

    return j;
}

int Joysticks::configIndex(const QJsonObject &config)
{
    QString conf = QJsonDocument(config["config"].toObject()).toJson();
    for (int i = 0; i < configs.size(); ++i) {
        if (conf != QJsonDocument(configs.at(i)["config"].toObject()).toJson())
            continue;
        return i;
    }
    return -1;
}
void Joysticks::updateConfEnums()
{
    for (int i = 0; i < f_list->size(); ++i) {
        Joystick *j = static_cast<Joystick *>(f_list->child(i));
        j->f_conf->setEnumStrings(QStringList() << "" << configTitles.keys());
        j->f_conf->setValue(0);
    }
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
            foreach (QJsonValue v, json["configs"].toArray()) {
                QString uid = v["uid"].toString();
                QString name = v["name"].toString();
                int index = v["index"].toString().toInt();
                QString jkey = QString("%1:%2:%3").arg(index).arg(name).arg(uid);

                int cidx = configIndex(v.toObject());
                configs.append(v.toObject());
                configIds.append(jkey);
                if (cidx >= 0)
                    continue;

                QString confTitle = v["title"].toString();
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
    foreach (const QJsonObject &config, configs) {
        if (config["uid"].isUndefined())
            continue;
        a.append(config);
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
