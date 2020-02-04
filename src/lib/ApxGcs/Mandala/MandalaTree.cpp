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
#include "MandalaTree.h"
#include "MandalaTreeFact.h"
#include <App/App.h>
#include <App/AppRoot.h>
#include <Mandala/MandalaValue.h>
#include <Mandala/backport/MandalaBackport.h>

MandalaTree::MandalaTree(Fact *parent)
    : Fact(parent,
           "mandala",
           "Mandala tree",
           tr("Vehicle data tree"),
           Group | FilterModel,
           "hexagon-multiple")
{
    //qDebug() << mandala::sns::nav::ins::gyro::title;

    qmlRegisterUncreatableType<MandalaTreeFact>("APX.Facts",
                                                1,
                                                0,
                                                "MandalaTreeFact",
                                                "Reference only");

    /*mandala::data m;
    qDebug() << m.sns.title;

    m.sns.nav.ins.mag.y.setValue(0.123f);
    qDebug() << m.sns.nav.ins.mag.y;
    //m.sns.nav.ins.mag.z = 0.123f;
    qDebug() << m.sns.nav.ins.mag.z;
    qDebug() << m.sns.nav.ins.mag.z.uid;*/

    /*static mandala::Text<mandala::sns::nav::ins::acc::y> txt;
    qDebug() << txt.title();*/

    /*struct mandala::sns::nav::ins::mag::y hy;
    qDebug() << hy.meta.title << hy;
    hy.set(0.551f);
    qDebug() << hy.meta.title << hy;

    {
        mandala::Value<mandala::sns::nav::ins::mag::y> v_hy;
        qDebug() << "Value:" << v_hy.meta.title << v_hy;
        v_hy.set(0.101f);
        qDebug() << "Value:" << v_hy.meta.title << v_hy;
        mandala::Value<mandala::sns::nav::ins::mag::y> v_hy2;
        qDebug() << "Value:" << v_hy2.meta.title << v_hy2;
    }

    {
        mandala::StaticValue<mandala::sns::nav::ins::mag::y> s_hy;
        qDebug() << "StaticValue:" << s_hy.data().meta.title << s_hy.data();
        s_hy.data().set(0.101f);
        qDebug() << "StaticValue:" << s_hy.data().meta.title << s_hy.data();
        mandala::StaticValue<mandala::sns::nav::ins::mag::y> s_hy2;
        qDebug() << "StaticValue:" << s_hy2.data().meta.title << s_hy2.data();
    }

    {
        mandala::Value<mandala::sns::nav::ins::gyro::temp> v;
        v.set(-5.101f);
        qDebug() << "Pack:" << v.meta.title << v;
        QByteArray ba(100, '\0');
        size_t sz = v.pack(ba.data());
        ba.resize(sz);
        qDebug() << "Pack:" << sz << ba.toHex().toUpper();
        sz = v.unpack(ba.data());
        qDebug() << "Unpack:" << sz << v.meta.title << v;
        ba.resize(100);
        sz = v.copy_to(ba.data());
        ba.resize(sz);
        qDebug() << "Copy:" << sz << ba.toHex().toUpper();
        sz = v.copy_from(ba.data());
        qDebug() << "Copy:" << sz << v.meta.title << v;
    }*/

    //size_t sz = sizeof(mandala::meta);
    //size_t sz1 = sizeof(*mandala::meta);
    //qDebug() << sz1 << sz << "bytes" << sz / sz1 << "items";

    Fact *group = this;
    uint8_t level = 0;
    QString sect;
    for (size_t i = 0; i < (sizeof(mandala::meta) / sizeof(*mandala::meta)); ++i) {
        const mandala::meta_t &d = mandala::meta[i];
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
            group = new MandalaTreeFact(this, group, d);
            if (d.level == 2)
                group->setSection(sect);
            continue;
        }
        if (group->child(d.name)) {
            apxMsgW() << "dup fact:" << group->child(d.name)->path(2);
        }
        MandalaTreeFact *f = new MandalaTreeFact(this, group, d);
        uid_map.insert(f->uid(), f);
        //find aliases
        for (auto i : mandala::backport::items) {
            if (d.uid != i.meta.uid)
                continue;
            if (i.meta.uid == mandala::backport::meta_void.uid)
                continue;
            f->addAlias(i.alias);
            f->setDescr(QString("{%1} %2").arg(i.alias).arg(f->descr()));
            break;
        }
    }

    //fact tests
    mandala::backport::MandalaBackport backport;

    //apxMsg() << findChild("sns.tcas.vel")->title();
    //apxMsg() << fact(mandala::sns::nav::air::aoa::meta.uid)->title();
}

MandalaTreeFact *MandalaTree::fact(mandala::uid_t uid) const
{
    MandalaTreeFact *f = uid_map.value(uid);
    if (f)
        return f;
    apxMsgW() << "Mandala uid not found:" << uid;
    return nullptr;
}

MandalaTreeFact *MandalaTree::fact(const QString &mpath) const
{
    MandalaTreeFact *f = nullptr;
    if (mpath.contains('.')) {
        f = static_cast<MandalaTreeFact *>(findChild(mpath));
    } else {
        for (auto i : uid_map) {
            if (i->alias() != mpath)
                continue;
            f = i;
            qDebug() << "Fact by alias:" << mpath;
            break;
        }
    }
    if (!f) {
        apxMsgW() << "Mandala fact not found:" << mpath;
    }
    return f;
}

QString MandalaTree::mandalaToString(quint16 uid) const
{
    MandalaTreeFact *f = uid_map.value(uid);
    return f ? f->mpath() : QString();
}
quint16 MandalaTree::stringToMandala(const QString &s) const
{
    if (s.isEmpty() || s == "0")
        return 0;
    //try int
    bool ok = false;
    uint i = s.toUInt(&ok);
    if (ok && i < 0xFFFF) {
        quint16 uid = static_cast<quint16>(i);
        MandalaTreeFact *f = fact(uid);
        if (f)
            return f->uid();
    }
    //try text
    MandalaTreeFact *f = fact(s);
    return f ? f->uid() : 0;
}
