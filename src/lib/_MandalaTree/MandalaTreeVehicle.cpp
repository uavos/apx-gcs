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
#include "MandalaTreeVehicle.h"
#include "MandalaTreeField.h"
//=============================================================================
MandalaTreeVehicle::MandalaTreeVehicle(MandalaTree *parent, IDENT::_ident *ident)
    : MandalaTree(parent,
                  ident ? ident->squawk : 0,
                  ident ? QString(ident->callsign[0] ? ident->callsign : "Vehicle").toUpper()
                        : "current",
                  "")
    , m_streamType(Offline)
{
    m_name.replace(' ', '-');
    m_name.replace('_', '-');
    parent->setProperty(m_name.toUtf8().data(), qVariantFromValue(this));
    buildDefault();
    if (ident && ident->squawk) {
        qDebug("IDENT: %s (%.4X)", ident->callsign, ident->squawk);
    }

    onlineTimer.setSingleShot(true);
    onlineTimer.setInterval(7000);
    connect(&onlineTimer, &QTimer::timeout, this, [=]() { updateOnline(false); });
    connect(this, &MandalaTree::dlcntChanged, this, [=]() { updateOnline(true); });
}
//=============================================================================
void MandalaTreeVehicle::buildDefault()
{
    _mandala *mandala = new _mandala(); //temporary
    for (_mandala::_group *g1 = mandala->next_group(NULL); g1; g1 = mandala->next_group(g1)) {
        MandalaTree *item_g1 = new MandalaTree(this, g1->index(), g1->name(), g1->descr());
        for (_mandala::_group *g2 = g1->next_group(NULL); g2; g2 = g1->next_group(g2)) {
            MandalaTree *item_g2 = new MandalaTree(item_g1, g2->index(), g2->name(), g2->descr());
            for (_mandala_field *f = g2->next_field(NULL); f; f = g2->next_field(f)) {
                QStringList opts, optsDescr;
                if (f->type() == _mandala_field::mf_enum) {
                    for (uint n = 0;; n++) {
                        QString s = f->opt(n);
                        if (s.isEmpty())
                            break;
                        opts.append(s);
                        optsDescr.append(qApp->translate("MandalaTree", f->opt(n | 0x80)));
                    }
                }
                new MandalaTreeField(item_g2,
                                     f->index(),
                                     f->name(),
                                     f->descr(),
                                     f->type(),
                                     opts,
                                     optsDescr,
                                     f->alias());
            }
        }
    }
    delete mandala;
    emit structChanged(this);
}
//=============================================================================
void MandalaTreeVehicle::bind(MandalaTree *v)
{
    if (bindItem == v)
        return;
    disconnect(this, &MandalaTree::dlcntChanged, this, 0);

    MandalaTree::bind(v);

    MandalaTreeVehicle *bv = static_cast<MandalaTreeVehicle *>(v);

    connect(bv, &MandalaTreeVehicle::onlineChanged, this, &MandalaTreeVehicle::onlineChanged);
    emit onlineChanged(online());
}
//=============================================================================
// PROPERTIES
//=============================================================================
bool MandalaTreeVehicle::online()
{
    //qDebug()<<this<<onlineTimer.isActive();
    if (bindItem)
        return static_cast<MandalaTreeVehicle *>(bindItem)->online();
    return onlineTimer.isActive();
}
void MandalaTreeVehicle::updateOnline(bool v)
{
    if (bindItem) {
        static_cast<MandalaTreeVehicle *>(bindItem)->updateOnline(v);
        return;
    }
    bool bV = online();
    if (v)
        onlineTimer.start();
    else
        onlineTimer.stop();
    //qDebug()<<this<<bV<<v;
    if ((!v) || bV != v) {
        emit onlineChanged(v);
    }
}
MandalaTreeVehicle::StreamType MandalaTreeVehicle::streamType()
{
    return m_streamType;
}
void MandalaTreeVehicle::setStreamType(MandalaTreeVehicle::StreamType v)
{
    if (v == m_streamType)
        return;
    m_streamType = v;
    emit streamTypeChanged(v);
}
//=============================================================================
//=============================================================================
void MandalaTreeVehicle::downlinkReceived(const QByteArray &ba)
{
    MandalaTree::downlinkReceived(ba);
    setStreamType(XPDR);
    //qDebug("rx: %s",ba.toHex().toUpper().data());
}
//=============================================================================
