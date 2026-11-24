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
#include "JoystickConfig.h"

#include "Joystick.h"

JoystickConfig::JoystickConfig(Fact *parent)
    : Fact(parent, "c#", "", "", Group | FlatModel)
{
    f_title = new Fact(this, "ctitle", tr("Title"), tr("Configuration title"), Text);
    f_name = new Fact(this, "device", tr("Device"), tr("Device name"), Text);
    connect(f_title, &Fact::valueChanged, this, [this]() { setTitle(f_title->text()); });
    connect(f_name, &Fact::valueChanged, this, [this]() { setDescr(f_name->text()); });

    f_naxes = new Fact(this, "naxes", tr("Axes count"), tr("Number of device axes"), Int);
    f_naxes->setMin(0);
    f_naxes->setMax(32);
    f_nbuttons = new Fact(this, "nbuttons", tr("Buttons count"), tr("Number of device buttons"), Int);
    f_nbuttons->setMin(0);
    f_nbuttons->setMax(128);
    f_nhats = new Fact(this, "nhats", tr("Hats count"), tr("Number of device hats"), Int);
    f_nhats->setMin(0);
    f_nhats->setMax(8);

    f_axes = new Fact(this, "axes", tr("Axes"), "", Section);
    f_buttons = new Fact(this, "buttons", tr("Buttons"), "", Section);
    f_hats = new Fact(this, "hats", tr("Hats"), "", Section);

    f_remove = new Fact(this,
                        "remove",
                        tr("Remove"),
                        tr("Remove configuration"),
                        Action | Remove | CloseOnTrigger,
                        "delete");

    saveEvent.setInterval(1000);
    connect(&saveEvent, &DelayedEvent::triggered, this, &JoystickConfig::changed);
    connect(f_title, &Fact::valueChanged, this, &JoystickConfig::schedule);
    connect(f_name, &Fact::valueChanged, this, &JoystickConfig::schedule);

    connect(f_naxes, &Fact::valueChanged, this, &JoystickConfig::updateControls);
    connect(f_nbuttons, &Fact::valueChanged, this, &JoystickConfig::updateControls);
    connect(f_nhats, &Fact::valueChanged, this, &JoystickConfig::updateControls);
}

int JoystickConfig::used(Fact *controls)
{
    for (int i = controls->size() - 1; i >= 0; --i) {
        if (!controls->child(i)->text().simplified().isEmpty())
            return i + 1;
    }
    return 0;
}

void JoystickConfig::updateControls()
{
    // controls which are set are never removed
    const int na = qMax(f_naxes->value().toInt(), used(f_axes));
    const int nb = qMax(f_nbuttons->value().toInt(), used(f_buttons));
    const int nh = qMax(f_nhats->value().toInt(), (used(f_hats) + 3) / 4);
    if (na != f_naxes->value().toInt() || nb != f_nbuttons->value().toInt()
        || nh != f_nhats->value().toInt()) {
        // comes back here with the corrected numbers
        f_naxes->setValue(na);
        f_nbuttons->setValue(nb);
        f_nhats->setValue(nh);
        return;
    }

    while (f_axes->size() > na)
        f_axes->child(f_axes->size() - 1)->deleteFact();
    while (f_buttons->size() > nb)
        f_buttons->child(f_buttons->size() - 1)->deleteFact();
    while (f_hats->size() > nh * 4)
        f_hats->child(f_hats->size() - 1)->deleteFact();

    Joystick::addControls(f_axes, f_buttons, f_hats, na, nb, nh);

    FactList list{f_axes, f_buttons, f_hats};
    for (auto f : list) {
        for (auto c : f->facts()) {
            connect(c, &Fact::valueChanged, this, &JoystickConfig::schedule, Qt::UniqueConnection);
            for (auto p : c->facts()) {
                connect(p,
                        &Fact::valueChanged,
                        this,
                        &JoystickConfig::schedule,
                        Qt::UniqueConnection);
            }
        }
    }
}

void JoystickConfig::schedule()
{
    if (!_loading)
        saveEvent.schedule();
}

void JoystickConfig::load(const QJsonObject &config)
{
    _loading = true;
    stored = config;
    f_title->setValue(config["title"].toString());
    f_name->setValue(config["name"].toString());

    // enough controls to show the configuration, a common gamepad by default
    const QJsonObject conf = config["config"].toObject();
    int cnt[3] = {6, 16, 4};
    const char *names[3] = {"axes", "buttons", "hats"};
    for (int i = 0; i < 3; ++i) {
        for (const auto jsv : conf[names[i]].toArray())
            cnt[i] = qMax(cnt[i], jsv.toObject().value("id").toInt());
    }
    // never hide controls of the configuration: hidden ones are lost on save
    f_naxes->setMax(qMax(cnt[0], f_naxes->max().toInt()));
    f_nbuttons->setMax(qMax(cnt[1], f_nbuttons->max().toInt()));
    f_nhats->setMax(qMax((cnt[2] + 3) / 4, f_nhats->max().toInt()));
    f_naxes->setValue(qMax(cnt[0], f_naxes->value().toInt()));
    f_nbuttons->setValue(qMax(cnt[1], f_nbuttons->value().toInt()));
    f_nhats->setValue(qMax((cnt[2] + 3) / 4, f_nhats->value().toInt()));

    Joystick::loadControls(conf, f_axes, f_buttons, f_hats);
    _loading = false;
}

QJsonObject JoystickConfig::save() const
{
    QJsonObject config;
    config.insert("title", f_title->text().simplified());
    config.insert("name", f_name->text().simplified());
    config.insert("uid", "user");
    config.insert("config", Joystick::saveControls(f_axes, f_buttons, f_hats));
    return config;
}
