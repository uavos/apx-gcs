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
#include <mandala/backport/MandalaBackport.h>

Mandala::Mandala(Fact *parent)
    : Fact(parent,
           "mandala",
           "Mandala",
           tr("Vehicle data tree"),
           Group | FilterModel | ModifiedGroup,
           "hexagon-multiple")
    , m_timestamp(0)
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
        //find aliases
        for (auto i : mandala::backport::items) {
            if (d.uid != i.meta.uid)
                continue;
            if (i.meta.uid == mandala::backport::meta_void.uid)
                continue;
            f->addAlias(i.alias);
            break;
        }
    }
}

quint64 Mandala::timestamp() const
{
    return m_timestamp;
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
    } else {
        for (auto i : uid_map) {
            if (i->alias() != mpath)
                continue;
            f = i;
            qDebug() << "Fact by alias:" << mpath;
            break;
        }
    }
    if (!f && !silent) {
        apxMsgW() << "Mandala fact not found:" << mpath;
    }
    return f;
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
    m_timestamp = timestamp_ms;
    if (m_timestamp == 0)
        _timestamp_time.start();

    for (auto const &uid : values.keys()) {
        MandalaFact *f = fact(uid);
        if (!f)
            continue;
        f->setValueFromStream(values.value(uid));
    }
}

void Mandala::valuesData(PBase::Values values)
{
    if (!_timestamp_time.isValid()) {
        m_timestamp = 0;
        _timestamp_time.start();
    } else {
        qint64 elapsed = _timestamp_time.elapsed();
        quint64 uelapsed = elapsed > 0 ? elapsed : 0;
        if (m_timestamp < uelapsed)
            m_timestamp = uelapsed;
    }

    //qDebug() << values.size();
    for (auto const &uid : values.keys()) {
        MandalaFact *f = fact(uid);
        if (!f)
            continue;
        f->setValueFromStream(values.value(uid));
    }
}
