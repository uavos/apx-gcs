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
#ifndef NodeItemData_H
#define NodeItemData_H
//=============================================================================
#include "NodeItemBase.h"
class Nodes;
//=============================================================================
class NodeItemData : public NodeItemBase
{
    Q_OBJECT

    Q_PROPERTY(QString sn READ sn CONSTANT)

    //flags
    Q_PROPERTY(bool reconf READ reconf WRITE setReconf NOTIFY reconfChanged)
    Q_PROPERTY(bool fwSupport READ fwSupport WRITE setFwSupport NOTIFY fwSupportChanged)
    Q_PROPERTY(bool fwUpdating READ fwUpdating WRITE setFwUpdating NOTIFY fwUpdatingChanged)
    Q_PROPERTY(bool addressing READ addressing WRITE setAddressing NOTIFY addressingChanged)
    Q_PROPERTY(bool rebooting READ rebooting WRITE setRebooting NOTIFY rebootingChanged)
    Q_PROPERTY(bool busy READ reconf WRITE setBusy NOTIFY busyChanged)
    Q_PROPERTY(bool failure READ failure WRITE setFailure NOTIFY failureChanged)

    //nstat
    Q_PROPERTY(qreal vbat READ vbat WRITE setVbat NOTIFY vbatChanged)
    Q_PROPERTY(qreal ibat READ ibat WRITE setIbat NOTIFY ibatChanged)
    Q_PROPERTY(quint8 errCnt READ errCnt WRITE setErrCnt NOTIFY errCntChanged)
    Q_PROPERTY(quint8 canRxc READ canRxc WRITE setCanRxc NOTIFY canRxcChanged)
    Q_PROPERTY(quint8 canAdr READ canAdr WRITE setCanAdr NOTIFY canAdrChanged)
    Q_PROPERTY(quint8 canErr READ canErr WRITE setCanErr NOTIFY canErrChanged)
    Q_PROPERTY(quint8 cpuLoad READ cpuLoad WRITE setCpuLoad NOTIFY cpuLoadChanged)

public:
    explicit NodeItemData(Fact *parent, QString sn);
    //---------------------------------------
    // PROPERTIES
public:
    QString sn() const;

    bool reconf() const;
    void setReconf(const bool &v);
    bool fwSupport() const;
    void setFwSupport(const bool &v);
    bool fwUpdating() const;
    void setFwUpdating(const bool &v);
    bool addressing() const;
    void setAddressing(const bool &v);
    bool rebooting() const;
    void setRebooting(const bool &v);
    bool busy() const;
    void setBusy(const bool &v);
    bool failure() const;
    void setFailure(const bool &v);

    qreal vbat() const;
    void setVbat(const qreal &v);
    qreal ibat() const;
    void setIbat(const qreal &v);
    quint8 errCnt() const;
    void setErrCnt(const quint8 &v);
    quint8 canRxc() const;
    void setCanRxc(const quint8 &v);
    quint8 canAdr() const;
    void setCanAdr(const quint8 &v);
    quint8 canErr() const;
    void setCanErr(const quint8 &v);
    quint8 cpuLoad() const;
    void setCpuLoad(const quint8 &v);

protected:
    QString m_sn;

    bool m_reconf;
    bool m_fwSupport;
    bool m_fwUpdating;
    bool m_addressing;
    bool m_rebooting;
    bool m_busy;
    bool m_failure;

    qreal m_vbat;
    qreal m_ibat;
    quint8 m_errCnt;
    quint8 m_canRxc;
    quint8 m_canAdr;
    quint8 m_canErr;
    quint8 m_cpuLoad;

signals:
    void reconfChanged();
    void fwSupportChanged();
    void fwUpdatingChanged();
    void addressingChanged();
    void rebootingChanged();
    void busyChanged();
    void failureChanged();

    void vbatChanged();
    void ibatChanged();
    void errCntChanged();
    void canRxcChanged();
    void canAdrChanged();
    void canErrChanged();
    void cpuLoadChanged();
};
//=============================================================================
#endif
