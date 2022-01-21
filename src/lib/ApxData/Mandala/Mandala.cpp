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
#include "Mandala.h"
#include "MandalaFact.h"
#include <App/App.h>
#include <App/AppRoot.h>
#include <mandala/MandalaMetaBase.h>

// TODO PLUGIN: gource-like real-time visualization of Mandala:
// See: https://www.youtube.com/watch?v=NjUuAuBcoqs&ab_channel=Gource
// See: https://doc.qt.io/qt-5/qtwidgets-graphicsview-elasticnodes-example.html
// Intended to monitor by human mind the current state of the vehicle and its behavior in real-time.
// Inspiration could be taken from shaders workbench here http://regis.toile-libre.org/fractals/MYOS/

Mandala::Mandala(Fact *parent)
    : Fact(parent,
           "mandala",
           "Mandala",
           tr("Vehicle data tree"),
           Group | FilterModel | ModifiedGroup,
           "hexagon-multiple")
{
    setMandala(this);
    qmlRegisterUncreatableType<MandalaFact>("APX.Facts", 1, 0, "MandalaFact", "Reference only");

    Fact *group = this;
    uint8_t level = 0;
    QString sect;
    for (size_t i = 0; i < (sizeof(mandala::meta) / sizeof(*mandala::meta)); ++i) {
        const mandala::meta_s &d = mandala::meta[i];
        if (d.group) {
            //move group to upper level
            for (; level > d.level; --level)
                if (level != 2)
                    group = group->parentFact();
            level = d.level + 1;
            if (d.level == 1) {
                sect = d.title;
                continue;
            }
            if (group->child(d.name)) {
                apxMsgW() << "dup group:" << group->child(d.name)->path(1);
            }
            MandalaFact *f = new MandalaFact(this, group, d);
            uid_map.insert(f->uid(), f);
            group = f;
            if (d.level == 2)
                group->setSection(sect);
            continue;
        }
        if (group->child(d.name)) {
            apxMsgW() << "dup fact:" << group->child(d.name)->path(2);
        }
        MandalaFact *f = new MandalaFact(this, group, d);
        uid_map.insert(f->uid(), f);
    }
}

MandalaFact *Mandala::fact(mandala::uid_t uid) const
{
    if (uid == 0xFFFF || mandala::is_bundle(uid))
        return nullptr;
    MandalaFact *f = uid_map.value(uid);
    if (f)
        return f;
    apxMsgW() << "Mandala uid not found:" << uid;
    return nullptr;
}

MandalaFact *Mandala::fact(const QString &mpath, bool silent) const
{
    MandalaFact *f = nullptr;
    if (mpath.isEmpty())
        return f;
    if (mpath.contains('.')) {
        f = static_cast<MandalaFact *>(findChild(mpath));
    }
    if (!f && !silent) {
        apxMsgW() << "Mandala fact not found:" << mpath;
    }
    return f;
}

mandala::uid_t Mandala::uid(const QString &mpath)
{
    for (auto const &d : mandala::meta) {
        if (d.path == mpath)
            return d.uid;
    }
    return {};
}

QList<MandalaFact *> Mandala::facts() const
{
    return uid_map.values();
}

PBase::Values Mandala::getValuesForStream() const
{
    PBase::Values values;
    for (auto f : facts()) {
        if (f->isSystem())
            continue;
        values.insert(f->uid(), f->getValueForStream());
    }
    return values;
}

QString Mandala::mandalaToString(xbus::pid_raw_t pid_raw) const
{
    xbus::pid_s pid(pid_raw);
    if (!pid.seq)
        return QString();
    MandalaFact *f = uid_map.value(pid.uid);
    return f ? f->mpath() : QString();
}
xbus::pid_raw_t Mandala::stringToMandala(const QString &s) const
{
    if (s.isEmpty())
        return 0;
    MandalaFact *f = fact(s);
    if (!f)
        return 0;
    xbus::pid_s pid(f->uid(), xbus::pri_final, 3);
    return pid.raw();
}

const mandala::meta_s &Mandala::meta(mandala::uid_t uid)
{
    for (auto const &d : mandala::meta) {
        if (d.uid == uid)
            return d;
    }
    return mandala::cmd::env::nmt::meta;
}

void Mandala::telemetryData(PBase::Values values, quint64 timestamp_ms)
{
    for (auto const &uid : values.keys()) {
        MandalaFact *f = fact(uid);
        if (!f)
            continue;
        f->setValueFromStream(values.value(uid));
    }
}

void Mandala::valuesData(PBase::Values values)
{
    //qDebug() << values.size();
    for (auto const &uid : values.keys()) {
        //qDebug() << meta(uid).path;

        MandalaFact *f = fact(uid);
        if (!f)
            continue;
        f->setValueFromStream(values.value(uid));
    }
}
