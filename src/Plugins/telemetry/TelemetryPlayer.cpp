#include "TelemetryPlayer.h"
#include "TelemetryPlot.h"
#include <TelemetryDB.h>
#include <VehicleMandalaFact.h>
#include <Vehicles.h>
#include <Vehicle.h>
//==============================================================================
TelemetryPlayer::TelemetryPlayer(QObject *parent)
  : QObject(parent),
    _db(new TelemetryDB(this,QLatin1String("GCSTelemetryPlayerSession"),NULL,true)),
    qDownlink(*_db),
    setTime0(0),
    m_telemetryID(0),
    m_time(0),
    m_playing(false),
    m_speed(1.0)
{
  qDownlink.setForwardOnly(true);
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
  Vehicles::instance()->f_local->f_select->trigger();
  Vehicles::instance()->f_local->setReplay(true);
  if(!_db->readDownlink(m_telemetryID,m_time))return;
  m_playing=true;
  playTime0=m_time;
  playTime.start();
  bool ok=true;
  while(ok){
    qDownlink.prepare("SELECT * FROM t_TelemetryData WHERE time>=?");
    qDownlink.addBindValue(m_time);
    ok=qDownlink.exec();
    if(!ok)break;
    if(qDownlink.next()){
      tNext=qDownlink.value(0).toULongLong();
      next();
    }else{
      qDebug()<<"No data";
    }

    break;
  }
  _db->checkResult(qDownlink);
  if(!ok)pause();
}
void TelemetryPlayer::pause()
{
  if(!m_playing)return;
  m_playing=false;
  qDownlink.finish();
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
    if(!qDownlink.value(1).isNull()){
      quint64 fieldID=qDownlink.value(1).toULongLong();
      Fact *f=_db->recFacts.key(fieldID);
      if(f){
        Vehicles::instance()->f_local->setReplay(true);
        f->setValue(qDownlink.value(2));
        if(!qDownlink.value(3).isNull()){
          QString s=f->title();
          if(!s.startsWith("rc_")){
            qDebug("%s",QString("[replay]%1: %2 = %3").arg(tr("uplink")).arg(s).arg(f->text()).toUtf8().data());
          }
        }
      }
    }else if(!qDownlink.value(4).isNull()){
      //event
      QString evt=qDownlink.value(4).toString();
      QString sv=qDownlink.value(2).toString();
      if(evt=="msg"){
        qDebug("%s",QString("<[replay]%1").arg(sv).toUtf8().data());
        FactSystem::instance()->sound(sv);
      }else{
        qDebug("%s",QString("[replay]%1: %2 (%3)").arg(qDownlink.value(3).isNull()?tr("downlink"):tr("uplink")).arg(evt).arg(sv).toUtf8().data());
      }
    }
    if(!qDownlink.next())pause();
    else{
      tNext=qDownlink.value(0).toULongLong();
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

