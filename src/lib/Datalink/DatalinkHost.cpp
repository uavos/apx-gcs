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
#include "AppSettings.h"
#include "DatalinkHost.h"
#include "DatalinkHosts.h"
//=============================================================================
DatalinkHost::DatalinkHost(DatalinkHosts *parent, QString title, QVariant host)
 : Fact(parent,"host#",title,"",FactItem,NoData),
   container(parent)
{
  setValue(host);
  connect(this,&Fact::childValueChanged,this,&DatalinkHost::updateStats);

  updateStatsTimer.setSingleShot(true);
  connect(&updateStatsTimer,&QTimer::timeout,this,&DatalinkHost::updateStats);

  updateStats();
}
//=============================================================================
void DatalinkHost::updateStats()
{
  QString s;
  if(time.isValid()){
    int t=time.elapsed()/1000;
    s=QString("%1 (%2)").arg(t>=60?tr("No service"):tr("Alive")).arg(t==0?tr("now"):t>=60?QString("%1 %2").arg(t/60).arg(tr("min")):QString("%1 %2").arg(t).arg(tr("sec")));
    updateStatsTimer.start(t>60?60000:5000);
  }
  setDescr(s);
}
//=============================================================================
void DatalinkHost::updateTimeout()
{
  time.start();
  updateStats();
}
//=============================================================================
