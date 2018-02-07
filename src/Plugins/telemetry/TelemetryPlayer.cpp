#include "TelemetryPlayer.h"
#include "TelemetryPlot.h"
#include <TelemetryDB.h>
#include <VehicleMandalaFact.h>
#include <Vehicles.h>
#include <Vehicle.h>
//==============================================================================
TelemetryPlayer::TelemetryPlayer(QObject *parent)
  : QObject(parent),
    vehicle(Vehicles::instance()->f_replay),
    _db(new TelemetryDB(this,QLatin1String("GCSTelemetryPlayerSession"),vehicle,true)),
    query(*_db),
    setTime0(0),
    m_telemetryID(0),
    m_time(0),
    m_playing(false),
    m_speed(1.0)
{
  query.setForwardOnly(true);
  timer.setSingleShot(true);
  connect(&timer,&QTimer::timeout,this,&TelemetryPlayer::next);
}
//==============================================================================
void TelemetryPlayer::setTelemetryID(quint64 v)
{
  if(m_telemetryID==v)return;
  m_telemetryID=v;
  _db->createTelemetryTable(m_telemetryID);
  rewind();
}
//==============================================================================
void TelemetryPlayer::setTime(quint64 v)
{
  if(m_time==v)return;
  m_time=v;
  emit timeChanged();
  setTime0=v;
  if(m_playing){
    pause();
    play();
  }
}
quint64 TelemetryPlayer::time()
{
  return m_time;
}
bool TelemetryPlayer::playing()
{
  return m_playing;
}
void TelemetryPlayer::setSpeed(double v)
{
  if(m_speed==v)return;
  m_speed=v;
  if(!m_playing)return;
  playTime0=m_time;
  playTime.start();
  next();
}
//==============================================================================
//==============================================================================
//==============================================================================
void TelemetryPlayer::play()
{
  if(m_playing)return;
  if(!m_telemetryID)return;
  vehicle->f_select->trigger();
  vehicle->setReplay(true);
  if(!_db->readDownlink(m_telemetryID,m_time))return;
  m_playing=true;
  playTime0=m_time;
  playTime.start();
  bool ok=true;
  while(ok){
    query.prepare("SELECT * FROM t_TelemetryData WHERE time>=?");
    query.addBindValue(m_time);
    ok=query.exec();
    if(!ok)break;
    if(query.next()){
      tNext=query.value(0).toULongLong();
      next();
    }else{
      qDebug()<<"No data";
    }

    break;
  }
  _db->checkResult(query);
  emit stateChanged();
  if(!ok)pause();
}
void TelemetryPlayer::pause()
{
  if(!m_playing)return;
  m_playing=false;
  query.finish();
  emit stateChanged();
  //qDebug()<<"pause";
}
void TelemetryPlayer::rewind()
{
  bool bPlaying=m_playing;
  pause();
  if(m_time==setTime0) setTime(0);
  else {
    setTime(setTime0);
    if(bPlaying)play();
  }
}
//==============================================================================
//==============================================================================
void TelemetryPlayer::next()
{
  if(!m_playing)return;
  quint64 t=playTime.elapsed();
  if(m_speed!=1.0)t=t*m_speed;
  t+=playTime0;
  if(tNext<=t){
    if(!query.value(1).isNull()){
      quint64 fieldID=query.value(1).toULongLong();
      Fact *f=_db->recFacts.key(fieldID);
      if(f){
        vehicle->setReplay(true);
        f->setValue(query.value(2));
        if(!query.value(3).isNull()){
          QString s=f->title();
          if(!s.startsWith("rc_")){
            qDebug("%s",QString("[replay]%1: %2 = %3").arg(tr("uplink")).arg(s).arg(f->text()).toUtf8().data());
          }
        }
      }
    }else if(!query.value(4).isNull()){
      //event
      QString evt=query.value(4).toString();
      QString sv=query.value(2).toString();
      if(evt=="msg"){
        qDebug("%s",QString("<[replay]%1").arg(sv).toUtf8().data());
        FactSystem::instance()->sound(sv);
      }else{
        qDebug("%s",QString("[replay]%1: %2 (%3)").arg(query.value(3).isNull()?tr("downlink"):tr("uplink")).arg(evt).arg(sv).toUtf8().data());
      }
    }
    //continue next record
    if(!query.next())pause();
    else{
      tNext=query.value(0).toULongLong();
    }
  }
  if(!m_playing)return;
  if(tNext>t){
    if(m_playing){
      timer.start(tNext-t);

    }
    if(m_time!=t){
      m_time=t;
      emit timeChanged();
    }
  }else if(m_playing){
    next();
  }
}
//==============================================================================

