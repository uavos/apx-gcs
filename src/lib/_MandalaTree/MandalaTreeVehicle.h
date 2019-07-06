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
#ifndef MandalaTreeVehicle_H
#define MandalaTreeVehicle_H
//=============================================================================
#include <QtCore>
#include "MandalaTree.h"
#include "node.h"
//=============================================================================
class MandalaTreeVehicle : public MandalaTree
{
    Q_OBJECT
public:
    explicit MandalaTreeVehicle(MandalaTree *parent, IDENT::_ident *ident);

    Q_INVOKABLE void buildDefault();

    void bind(MandalaTree *f);

    enum StreamType { Offline, Replay, FieldValue, XPDR, DLINK };
    Q_ENUM(StreamType)

public slots:
    void downlinkReceived(const QByteArray &ba);

    //PROPERTIES
public:
    Q_PROPERTY(bool online READ online NOTIFY onlineChanged)
    bool online();
    void updateOnline(bool v);
    Q_PROPERTY(StreamType streamType READ streamType WRITE setStreamType NOTIFY streamTypeChanged)
    StreamType streamType();

private:
    QTimer onlineTimer;
    StreamType m_streamType;
    void setStreamType(StreamType v);
signals:
    void onlineChanged(bool);
    void streamTypeChanged(StreamType);
};
//Q_DECLARE_METATYPE(MandalaTreeVehicle*)
//=============================================================================
#endif
