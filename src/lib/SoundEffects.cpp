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
#include "SoundEffects.h"
#include "QMandala.h"
#include "AppSettings.h"
//=============================================================================
SoundEffects::SoundEffects(QObject *parent)
    :QObject(parent)
{
  mandala=qApp->property("Mandala").value<QMandala*>();

  //load SoundEffects files
  QSettings st;
  QString lang=st.value("lang","default").toString();
  QString voice=st.value("voice","default").toString();
  if(voice=="default"){
    if(lang=="ru") voice="ru-milena";
    else voice="vicki";
  }
  QDir fdir(QMandala::Global::res().filePath("audio/speech/"+voice),"*.ogg *.wav");
  //qDebug()<<fdir.absolutePath();
  QStringList files=fdir.entryList();
  foreach(QString file,files){
    QSoundEffect *e=new QSoundEffect(this);
    e->setSource(QUrl::fromLocalFile(fdir.absoluteFilePath(file)));
    connect(e,&QSoundEffect::playingChanged,this,&SoundEffects::effectPlayingChanged);
    speech.insert(file.left(file.indexOf('.')),e);
    //qDebug()<<file;
  }
  //add alarms
  QMap<QString,QString> alias;
  alias["error"]="alarm2";
  alias["warning"]="alarm";
  alias["disconnected"]="alternator_off";
  alias["connected"]="radar_lock";
  //load files and create effects
  //qDebug() << QSoundEffect::supportedMimeTypes();
  fdir=QDir(QMandala::Global::res().filePath("audio/alerts"),"*.ogg *.wav");
  foreach(QFileInfo fi,fdir.entryInfoList()){
    if(!alias.values().contains(fi.baseName()))continue;
    QSoundEffect *e=new QSoundEffect(this);
    //qDebug()<<fi.absoluteFilePath();
    e->setSource(QUrl::fromLocalFile(fi.absoluteFilePath()));
    connect(e,&QSoundEffect::playingChanged,this,&SoundEffects::effectPlayingChanged);
    effects.insert(alias.key(fi.baseName()),e);
    //qDebug()<<alias.key(fi.baseName())<<fi.baseName();
  }
  //last_file timeout timer
  timeoutTimer.setSingleShot(true);
  timeoutTimer.setInterval(4000);
  connect(&timeoutTimer,&QTimer::timeout,this,&SoundEffects::timeout);
}
//=============================================================================
void SoundEffects::play(QString text)
{
  if(text.contains('\n')){
    QStringList st(text.split('\n',QString::SkipEmptyParts));
    while(st.size())play(st.takeFirst());
    return;
  }
  if(!AppSettings::value("sounds").toBool())return;
  //qDebug()<<"play"<<text;
  text.remove(':');
  QStringList st;
  QHash<QString,QSoundEffect*> *elist=NULL;
  foreach(QString key,speech.keys()){
    if(text.contains(key,Qt::CaseInsensitive))
      st.append(key);
  }
  if(st.size()){
    elist=&speech;
  }else{
    //try errors & warnings
    foreach(QString key,effects.keys()){
      //qDebug()<<text<<found_file<<key;
      if(text.contains(key,Qt::CaseInsensitive))
        st.append(key);
    }
    if(st.size()){
      elist=&effects;
    }
  }
  if(st.isEmpty())return;
  st.sort();
  //qDebug()<<st;
  QSoundEffect *e=elist->value(st.last());
  if(!e)return;
  if(lastEffect==e)return;
  lastEffect=e;
  if(!timeoutTimer.isActive())timeoutTimer.start();
  if(effectsQueue.contains(e))return;
  effectsQueue.append(e);
  if(effectsQueue.size()==1) e->play();
}
//=============================================================================
void SoundEffects::timeout()
{
  lastEffect=NULL;
}
//=============================================================================
void SoundEffects::effectPlayingChanged()
{
  QSoundEffect *e=static_cast<QSoundEffect*>(sender());
  if((!e) || e->isPlaying())return;
  effectsQueue.takeFirst();
  if(effectsQueue.isEmpty())return;
  effectsQueue.first()->play();
}
//=============================================================================
