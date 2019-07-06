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
#include "NodeItemData.h"
#include "Nodes.h"
//=============================================================================
NodeItemData::NodeItemData(Fact *parent, QString sn)
    : NodeItemBase(parent, "node#", "", Group)
    , m_sn(sn)
    , m_reconf(false)
    , m_fwSupport(false)
    , m_fwUpdating(false)
    , m_addressing(false)
    , m_rebooting(false)
    , m_busy(false)
    , m_failure(false)
    ,

    m_vbat(0)
    , m_ibat(0)
    , m_errCnt(0)
    , m_canRxc(0)
    , m_canAdr(0)
    , m_canErr(0)
    , m_cpuLoad(0)
{}
//=============================================================================
QString NodeItemData::sn() const
{
    return m_sn;
}
bool NodeItemData::reconf() const
{
    return m_reconf;
}
void NodeItemData::setReconf(const bool &v)
{
    if (m_reconf == v)
        return;
    m_reconf = v;
    emit reconfChanged();
}
bool NodeItemData::fwSupport() const
{
    return m_fwSupport;
}
void NodeItemData::setFwSupport(const bool &v)
{
    if (m_fwSupport == v)
        return;
    m_fwSupport = v;
    emit fwSupportChanged();
}
bool NodeItemData::fwUpdating() const
{
    return m_fwUpdating;
}
void NodeItemData::setFwUpdating(const bool &v)
{
    if (m_fwUpdating == v)
        return;
    m_fwUpdating = v;
    emit fwUpdatingChanged();
}
bool NodeItemData::addressing() const
{
    return m_addressing;
}
void NodeItemData::setAddressing(const bool &v)
{
    if (m_addressing == v)
        return;
    m_addressing = v;
    emit addressingChanged();
}
bool NodeItemData::rebooting() const
{
    return m_rebooting;
}
void NodeItemData::setRebooting(const bool &v)
{
    if (m_rebooting == v)
        return;
    m_rebooting = v;
    emit rebootingChanged();
}
bool NodeItemData::busy() const
{
    return m_busy;
}
void NodeItemData::setBusy(const bool &v)
{
    if (m_busy == v)
        return;
    m_busy = v;
    emit busyChanged();
}
bool NodeItemData::failure() const
{
    return m_failure;
}
void NodeItemData::setFailure(const bool &v)
{
    if (m_failure == v)
        return;
    m_failure = v;
    emit failureChanged();
}

qreal NodeItemData::vbat() const
{
    return m_vbat;
}
void NodeItemData::setVbat(const qreal &v)
{
    if (m_vbat == v)
        return;
    m_vbat = v;
    emit vbatChanged();
}
qreal NodeItemData::ibat() const
{
    return m_ibat;
}
void NodeItemData::setIbat(const qreal &v)
{
    if (m_ibat == v)
        return;
    m_ibat = v;
    emit ibatChanged();
}
quint8 NodeItemData::errCnt() const
{
    return m_errCnt;
}
void NodeItemData::setErrCnt(const quint8 &v)
{
    if (m_errCnt == v)
        return;
    m_errCnt = v;
    emit errCntChanged();
}
quint8 NodeItemData::canRxc() const
{
    return m_canRxc;
}
void NodeItemData::setCanRxc(const quint8 &v)
{
    if (m_canRxc == v)
        return;
    m_canRxc = v;
    emit canRxcChanged();
}
quint8 NodeItemData::canAdr() const
{
    return m_canAdr;
}
void NodeItemData::setCanAdr(const quint8 &v)
{
    if (m_canAdr == v)
        return;
    m_canAdr = v;
    emit canAdrChanged();
}
quint8 NodeItemData::canErr() const
{
    return m_canErr;
}
void NodeItemData::setCanErr(const quint8 &v)
{
    if (m_canErr == v)
        return;
    m_canErr = v;
    emit canErrChanged();
}
quint8 NodeItemData::cpuLoad() const
{
    return m_cpuLoad;
}
void NodeItemData::setCpuLoad(const quint8 &v)
{
    if (m_cpuLoad == v)
        return;
    m_cpuLoad = v;
    emit cpuLoadChanged();
}
//=============================================================================
//=============================================================================
