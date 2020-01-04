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
#include <App/App.h>
#include <App/AppRoot.h>
#include <MandalaDict.h>
//=============================================================================
MandalaTree::MandalaTree(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           "Mandala",
           tr("Vehicle data tree"),
           Group,
           "hexagon-multiple")
{
    qDebug() << mandala::sns::nav::ins::gyro::title;

    /*mandala::data m;
    qDebug() << m.sns.title;

    m.sns.nav.ins.mag.y.setValue(0.123f);
    qDebug() << m.sns.nav.ins.mag.y;
    //m.sns.nav.ins.mag.z = 0.123f;
    qDebug() << m.sns.nav.ins.mag.z;
    qDebug() << m.sns.nav.ins.mag.z.uid;*/

    static mandala::Text<mandala::sns::nav::ins::acc::y> txt;
    qDebug() << txt.title();

    struct mandala::sns::nav::ins::mag::y hy;
    qDebug() << hy.title << hy;
    hy.set(0.551f);
    qDebug() << hy.title << hy;

    {
        mandala::Value<mandala::sns::nav::ins::mag::y> v_hy;
        qDebug() << "Value:" << v_hy.title << v_hy;
        v_hy.set(0.101f);
        qDebug() << "Value:" << v_hy.title << v_hy;
        mandala::Value<mandala::sns::nav::ins::mag::y> v_hy2;
        qDebug() << "Value:" << v_hy2.title << v_hy2;
    }

    {
        mandala::StaticValue<mandala::sns::nav::ins::mag::y> s_hy;
        qDebug() << "StaticValue:" << s_hy.data().title << s_hy.data();
        s_hy.data().set(0.101f);
        qDebug() << "StaticValue:" << s_hy.data().title << s_hy.data();
        mandala::StaticValue<mandala::sns::nav::ins::mag::y> s_hy2;
        qDebug() << "StaticValue:" << s_hy2.data().title << s_hy2.data();
    }

    size_t sz = sizeof(mandala::dict);
    size_t sz1 = sizeof(*mandala::dict);
    qDebug() << sz << sz1 << sz / sz1;

    Fact *group = this;
    uint8_t level = 0;
    for (size_t i = 0; i < (sizeof(mandala::dict) / sizeof(*mandala::dict)); ++i) {
        const mandala::dict_meta_t &d = mandala::dict[i];
        if (d.group) {
            //move group to upper level
            for (; level > d.level; --level)
                group = group->parentFact();
            level = d.level + 1;
            group = new Fact(group, d.name, d.name, d.title);
            continue;
        }
        Fact *f = new Fact(group, d.name, d.name, d.title);
        QString alias(d.alias);
        if (!alias.isEmpty())
            f->setValue(alias);
    }
}
//============================================================================
