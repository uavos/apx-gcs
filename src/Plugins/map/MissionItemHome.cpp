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
#include "MissionModel.h"
#include "MissionItemHome.h"
#include "MissionItemField.h"
#include "QMandala.h"
//=============================================================================
MissionItemHome::MissionItemHome(MissionModel *model, QString descr)
 : MissionItem(model->rootItem,tr("Home"),descr),model(model)
{
  connect(this,SIGNAL(changed()),model,SLOT(emit_layoutChanged()));
  //MissionItemField *f;
  new MissionItemField(this,"latitude",MissionItemField::dt_lat);
  //connect(f,SIGNAL(edited()),var->field(f->bind),SLOT(send()));
  new MissionItemField(this,"longitude",MissionItemField::dt_lon);
  //connect(f,SIGNAL(edited()),var->field(f->bind),SLOT(send()));
  new MissionItemField(this,"HMSL",MissionItemField::dt_distance);
  //connect(f,SIGNAL(edited()),var->field(f->bind),SLOT(send()));
}
//=============================================================================
QVariant MissionItemHome::value(void) const
{
  return QVariant();
}
//=============================================================================
