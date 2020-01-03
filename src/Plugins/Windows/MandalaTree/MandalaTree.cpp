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
#include <MandalaMeta.h>
//=============================================================================
MandalaTree::MandalaTree(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           "Mandala",
           tr("Vehicle data tree"),
           Group,
           "hexagon-multiple")
{
    qDebug() << mandala::meta::sns::nav::ins::gyro::title;

    /*mandala::data m;
    qDebug() << m.sns.title;

    m.sns.nav.ins.mag.y.setValue(0.123f);
    qDebug() << m.sns.nav.ins.mag.y;
    //m.sns.nav.ins.mag.z = 0.123f;
    qDebug() << m.sns.nav.ins.mag.z;
    qDebug() << m.sns.nav.ins.mag.z.uid;*/

    static mandala::Text<mandala::meta::sns::nav::ins::acc::y> txt;
    qDebug() << txt.title();

    class mandala::meta::sns::nav::ins::mag::y hy;
    qDebug() << hy.title << hy;
    hy.setValue(0.551f);
    qDebug() << hy.title << hy;

    mandala::value<mandala::meta::sns::nav::ins::mag::y> v_hy;
    qDebug() << v_hy.title << v_hy;
    v_hy.setValue(0.101f);
    qDebug() << v_hy.title << v_hy;

    size_t sz = sizeof(mandala::dict);
    size_t sz1 = sizeof(*mandala::dict);
    qDebug() << sz << sz1 << sz / sz1;
}
//============================================================================
