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
#include "NodeData.h"
#include "Nodes.h"
//=============================================================================
NodeData::NodeData(Nodes *parent, const QByteArray &sn)
  : Fact(parent->f_list,"node#","","",GroupItem,NoData),
    sn(sn),
    m_reconf(false),
    m_fwSupport(false),
    m_fwUpdating(false),
    m_addressing(false),
    m_rebooting(false),
    m_busy(false),
    m_failure(false),

    m_vbat(0),
    m_ibat(0),
    m_errCnt(0),
    m_canRxc(0),
    m_canAdr(0),
    m_canErr(0),
    m_cpuLoad(0)
{
}
//=============================================================================
bool NodeData::reconf() const
{
  return m_reconf;
}
void NodeData::setReconf(const bool &v)
{
  if(m_reconf==v)return;
  m_reconf=v;
  emit reconfChanged();
}
bool NodeData::fwSupport() const
{
  return m_fwSupport;
}
void NodeData::setFwSupport(const bool &v)
{
  if(m_fwSupport==v)return;
  m_fwSupport=v;
  emit fwSupportChanged();
}
bool NodeData::fwUpdating() const
{
  return m_fwUpdating;
}
void NodeData::setFwUpdating(const bool &v)
{
  if(m_fwUpdating==v)return;
  m_fwUpdating=v;
  emit fwUpdatingChanged();
}
bool NodeData::addressing() const
{
  return m_addressing;
}
void NodeData::setAddressing(const bool &v)
{
  if(m_addressing==v)return;
  m_addressing=v;
  emit addressingChanged();
}
bool NodeData::rebooting() const
{
  return m_rebooting;
}
void NodeData::setRebooting(const bool &v)
{
  if(m_rebooting==v)return;
  m_rebooting=v;
  emit rebootingChanged();
}
bool NodeData::busy() const
{
  return m_busy;
}
void NodeData::setBusy(const bool &v)
{
  if(m_busy==v)return;
  m_busy=v;
  emit busyChanged();
}
bool NodeData::failure() const
{
  return m_failure;
}
void NodeData::setFailure(const bool &v)
{
  if(m_failure==v)return;
  m_failure=v;
  emit failureChanged();
}

qreal NodeData::vbat() const
{
  return m_vbat;
}
void NodeData::setVbat(const qreal &v)
{
  if(m_vbat==v)return;
  m_vbat=v;
  emit vbatChanged();
}
qreal NodeData::ibat() const
{
  return m_ibat;
}
void NodeData::setIbat(const qreal &v)
{
  if(m_ibat==v)return;
  m_ibat=v;
  emit ibatChanged();
}
quint8 NodeData::errCnt() const
{
  return m_errCnt;
}
void NodeData::setErrCnt(const quint8 &v)
{
  if(m_errCnt==v)return;
  m_errCnt=v;
  emit errCntChanged();
}
quint8 NodeData::canRxc() const
{
  return m_canRxc;
}
void NodeData::setCanRxc(const quint8 &v)
{
  if(m_canRxc==v)return;
  m_canRxc=v;
  emit canRxcChanged();
}
quint8 NodeData::canAdr() const
{
  return m_canAdr;
}
void NodeData::setCanAdr(const quint8 &v)
{
  if(m_canAdr==v)return;
  m_canAdr=v;
  emit canAdrChanged();
}
quint8 NodeData::canErr() const
{
  return m_canErr;
}
void NodeData::setCanErr(const quint8 &v)
{
  if(m_canErr==v)return;
  m_canErr=v;
  emit canErrChanged();
}
quint8 NodeData::cpuLoad() const
{
  return m_cpuLoad;
}
void NodeData::setCpuLoad(const quint8 &v)
{
  if(m_cpuLoad==v)return;
  m_cpuLoad=v;
  emit cpuLoadChanged();
}
//=============================================================================
//=============================================================================
