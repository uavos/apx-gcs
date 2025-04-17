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
#include "Joystick.h"
#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppLog.h>

#include "JoystickAxis.h"

Joystick::Joystick(Fact *parent, int device_index, QString uid)
    : Fact(parent, "j#", "", "", Group | Bool | FlatModel)
    , device_index(device_index)
    , uid(uid)
    , instanceID(-1)
{
    f_conf = new Fact(this, "conf", tr("Configuration"), tr("Configuration title"), Enum);

    f_title = new Fact(this, "ctitle", tr("Title"), tr("Configuration title"), Text);
    connect(f_title, &Fact::valueChanged, this, [this]() {
        setDescr(devName);
        setTitle(f_title->text());
    });

    f_axes = new Fact(this, "axes", tr("Axes"), "", Section);
    f_buttons = new Fact(this, "buttons", tr("Buttons"), "", Section);
    f_hats = new Fact(this, "hats", tr("Hats"), "", Section);

    f_save = new Fact(this,
                      "save",
                      tr("Save"),
                      tr("Save configuration"),
                      Action | Apply,
                      "content-save");

    if (device_index < 0) {
        return;
    }

    devName = SDL_JoystickNameForIndex(device_index);
    setTitle(devName);

    //get info
    updateDevice(true);
    if (!dev)
        return;

    int cnt;
    cnt = SDL_JoystickNumAxes(dev);
    for (int i = 0; i < cnt; ++i) {
        new JoystickAxis(f_axes);
    }
    cnt = SDL_JoystickNumButtons(dev);
    for (int i = 0; i < cnt; ++i) {
        new Fact(f_buttons, QString("button%1").arg(i), QString::number(i + 1), "", Text);
    }
    cnt = SDL_JoystickNumHats(dev);
    for (int i = 0; i < cnt; ++i) {
        new Fact(f_hats, QString("hat%1_u").arg(i), QString::number(i + 1) + "_UP", "", Text);
        new Fact(f_hats, QString("hat%1_r").arg(i), QString::number(i + 1) + "_RIGHT", "", Text);
        new Fact(f_hats, QString("hat%1_d").arg(i), QString::number(i + 1) + "_DOWN", "", Text);
        new Fact(f_hats, QString("hat%1_l").arg(i), QString::number(i + 1) + "_LEFT", "", Text);
    }
    for (auto i : f_buttons->facts()) {
        i->setOption(HighlightActive);
    }
    for (auto i : f_hats->facts()) {
        i->setOption(HighlightActive);
    }
    for (auto i : f_axes->facts()) {
        i->setOption(HighlightActive);
    }

    devName = SDL_JoystickName(dev);
    setTitle(devName);
    setDescr(QString("A%1 B%2 H%3 [%4]")
                 .arg(f_axes->size())
                 .arg(f_buttons->size())
                 .arg(f_hats->size())
                 .arg(uid));

    connect(this, &Fact::valueChanged, this, [this]() { setActive(this->dev && value().toBool()); });
    setValue(true);

    //save config
    saveEvent.setInterval(1000);
    connect(&saveEvent, &DelayedEvent::triggered, this, &Joystick::save);
    connect(f_title, &Fact::valueChanged, &saveEvent, &DelayedEvent::schedule);
    FactList f_list{f_axes, f_buttons, f_hats};
    for (auto f : f_list) {
        for (int i = 0; i < f->size(); ++i) {
            connect(f->child(i), &Fact::valueChanged, &saveEvent, &DelayedEvent::schedule);
            for (int j = 0; j < f->child(i)->size(); ++j) {
                connect(f->child(i)->child(j),
                        &Fact::valueChanged,
                        &saveEvent,
                        &DelayedEvent::schedule);
            }
        }
    }
}
Joystick::~Joystick()
{
    if (dev)
        SDL_JoystickClose(dev);
}

QString Joystick::juid() const
{
    return QString("%1:%2:%3").arg(device_index).arg(devName).arg(uid);
}

void Joystick::updateDevice(bool connected)
{
    if (connected) {
        if (!dev) {
            dev = SDL_JoystickOpen(device_index);
            if (!dev) {
                apxMsgW() << tr("Can't open joystick") << title() << SDL_GetError();
                return;
            }
            instanceID = SDL_JoystickInstanceID(dev);
        }
        if (value().toBool())
            setActive(true);
    } else {
        if (device_index >= 0) {
            SDL_JoystickClose(SDL_JoystickOpen(device_index));
        }
        dev = nullptr;
        device_index = -1;
        setActive(false);
    }
}
void Joystick::updateAxis(int i, qreal v)
{
    JoystickAxis *f = static_cast<JoystickAxis *>(f_axes->child(i));
    if (f)
        f->update(v);
}
void Joystick::updateButton(int i, bool v)
{
    Fact *f = f_buttons->child(i);
    if (!f)
        return;
    f->setActive(v);
    if (!v)
        return;
    QString s = f->text().simplified();
    if (s.isEmpty())
        return;
    App::jsexec(s);
}
void Joystick::updateHat(int i, quint8 v)
{
    quint8 m = 1;
    for (int j = 0; j < 4; ++j) {
        bool b = v & m;
        m <<= 1;
        Fact *f = f_hats->child(i * 4 + j);
        if (!f)
            continue;
        f->setActive(b);
        if (!b)
            continue;
        QString s = f->text().simplified();
        if (s.isEmpty())
            return;
        App::jsexec(s);
    }
}

void Joystick::loadConfig(const QJsonObject &config)
{
    //clear
    for (int i = 0; i < f_axes->size(); ++i)
        f_axes->child(i)->setValue(QVariant());
    for (int i = 0; i < f_buttons->size(); ++i)
        f_buttons->child(i)->setValue(QVariant());
    for (int i = 0; i < f_hats->size(); ++i)
        f_hats->child(i)->setValue(QVariant());

    //load
    f_title->setValue(config["title"]);
    const QJsonObject &conf = config["config"].toObject();
    for (const auto jsv : conf["axes"].toArray()) {
        const auto jso = jsv.toObject();
        int i = jso.value("id").toInt() - 1;
        JoystickAxis *f = static_cast<JoystickAxis *>(f_axes->child(i));
        if (f)
            f->loadConfig(jso);
    }
    for (const auto jsv : conf["buttons"].toArray()) {
        const auto jso = jsv.toObject();
        int i = jso.value("id").toInt() - 1;
        Fact *f = f_buttons->child(i);
        if (!f)
            continue;
        f->setValue(jso.value("scr"));
    }
    for (const auto jsv : conf["hats"].toArray()) {
        const auto jso = jsv.toObject();
        int i = jso.value("id").toInt() - 1;
        Fact *f = f_hats->child(i);
        if (!f)
            continue;
        f->setValue(jso.value("scr"));
    }
}

QJsonObject Joystick::saveConfig()
{
    QJsonObject config;
    config.insert("title", f_title->text());
    config.insert("name", devName);
    config.insert("index", device_index);
    config.insert("uid", uid);
    QJsonObject conf;
    FactList f_list{f_axes, f_buttons, f_hats};
    for (auto f : f_list) {
        QJsonArray a;
        for (int i = 0; i < f->size(); ++i) {
            QString s = f->child(i)->text().simplified();
            if (s.isEmpty())
                continue;
            QJsonObject v;
            v.insert("id", i + 1);
            v.insert("scr", s);
            if (f == f_axes) {
                v.insert("hyst", f->child(i)->child(0)->value().toDouble());
            }
            a.append(v);
        }
        if (!a.empty())
            conf.insert(f->name(), a);
    }
    config.insert("config", conf);
    return config;
}
