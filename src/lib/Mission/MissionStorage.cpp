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
#include "MissionStorage.h"
#include "Vehicle.h"
#include "VehicleSelect.h"
#include "VehicleMission.h"

#include <AppDirs.h>
#include <QFileDialog>
#include "MissionXml.h"
//=============================================================================
MissionStorage::MissionStorage(VehicleMission *mission,Fact *parent)
  : Fact(parent,"storage",tr("Storage"),tr("Missions database and sharing"),mission==parent?GroupItem:SectionItem,NoData),
    mission(mission)
{
  setIconSource("database");

  f_export=new Fact(this,"export",tr("Save"),tr("Export mission"),FactItem,NoData);
  f_export->setIconSource("content-save");
  connect(f_export,&Fact::triggered,this,&Fact::actionTriggered); //to close popups
  connect(f_export,&Fact::triggered,this,&MissionStorage::saveToFile);

  f_import=new Fact(this,"import",tr("Load"),tr("Import mission"),FactItem,NoData);
  f_import->setIconSource("folder-open");
  connect(f_import,&Fact::triggered,this,&Fact::actionTriggered);
  connect(f_import,&Fact::triggered,this,&MissionStorage::loadFromFile);

  f_copy=new VehicleSelect(this,"copy",tr("Copy"),tr("Copy to vehicle"));
  f_copy->setIconSource("content-copy");


  if(treeItemType()==SectionItem){
    for (int i=0;i<size();++i) {
      childFact(i)->setSection(title());
    }
  }
  connect(mission,&VehicleMission::missionSizeChanged,this,&MissionStorage::updateActions);
  connect(this,&MissionStorage::misssionRead,mission,&VehicleMission::missionReceived);
}
//=============================================================================
void MissionStorage::updateActions()
{
  bool bEmpty=mission->missionSize()<=0;
  f_export->setEnabled(!bEmpty);
}
//=============================================================================
void MissionStorage::saveToFile() const
{
  if(!AppDirs::missions().exists()) AppDirs::missions().mkpath(".");
  QFileDialog dlg(NULL,f_export->descr(),AppDirs::missions().canonicalPath());
  dlg.setAcceptMode(QFileDialog::AcceptSave);
  dlg.setOption(QFileDialog::DontConfirmOverwrite,false);
  QStringList filters;
  filters << tr("Mission files")+" (*.mission)"
          << tr("Any files")+" (*)";
  dlg.setNameFilters(filters);
  dlg.setDefaultSuffix("mission");
  QString fname=mission->f_missionTitle->text().replace(' ','-');
  if(!fname.isEmpty())fname.append("-");
  fname.append(mission->vehicle->f_callsign->text());
  dlg.selectFile(AppDirs::missions().filePath(fname));
  if(!dlg.exec() || dlg.selectedFiles().size()!=1)return;

  fname=dlg.selectedFiles().first();
  QFile file(fname);
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    qWarning("%s",QString(tr("Cannot write file")+" %1:\n%2.").arg(fname).arg(file.errorString()).toUtf8().data());
    return;
  }
  QTextStream stream(&file);
  //vehicle->f_nodes->xml->write().save(stream,2);
  file.close();
}
//=============================================================================
void MissionStorage::loadFromFile()
{
  if(!AppDirs::missions().exists()) AppDirs::missions().mkpath(".");
  QFileDialog dlg(NULL,f_import->descr(),AppDirs::missions().canonicalPath());
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  dlg.setFileMode(QFileDialog::ExistingFile);
  QStringList filters;
  filters << tr("Mission files")+" (*.xml *.mission)"
          << tr("Any files")+" (*)";
  dlg.setNameFilters(filters);
  QString fname=mission->f_missionTitle->text().replace(' ','-');
  if(!fname.isEmpty())fname.append("-");
  fname.append(mission->vehicle->f_callsign->text());
  dlg.selectFile(AppDirs::missions().filePath(fname));
  if(!dlg.exec() || dlg.selectedFiles().size()!=1)return;

  fname=dlg.selectedFiles().first();
  QFile file(fname);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    qWarning("%s",QString(tr("Cannot read file")+" %1:\n%2.").arg(fname).arg(file.errorString()).toUtf8().data());
    return;
  }
  QDomDocument doc;
  doc.setContent(&file);
  file.close();
  mission->clearMission();
  MissionXml xml(mission);
  if(!xml.read(doc.documentElement()))mission->clearMission();
  mission->backup();
  mission->setModified(false,true);
  if(mission->missionSize()>0)emit misssionRead();
}
//=============================================================================
//=============================================================================
