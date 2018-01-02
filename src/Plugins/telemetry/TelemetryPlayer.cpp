#include "TelemetryPlayer.h"
#include "TelemetryPlot.h"
#include <TelemetryDB.h>
#include <VehicleMandalaFact.h>
//==============================================================================
TelemetryPlayer::TelemetryPlayer(QObject *parent)
  : QObject(parent),
    _db(new TelemetryDB(this,QLatin1String("GCSTelemetryPlayerSession"))),
    qDownlink(*_db),
    setTime0(0),
    m_telemetryID(0),
    m_time(0),
    m_playing(false),
    m_speed(1.0)
{
  timer.setSingleShot(true);
  connect(&timer,&QTimer::timeout,this,&TelemetryPlayer::next);
}
//==============================================================================
void TelemetryPlayer::setTelemetryID(quint64 v)
{
  if(m_telemetryID==v)return;
  m_telemetryID=v;
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
  if(!_db->readDownlink(m_telemetryID,m_time))return;
  m_playing=true;
  playTime0=m_time;
  playTime.start();
  bool ok=true;
  while(ok){
    qDownlink.prepare("SELECT * FROM TelemetryDownlink WHERE telemetryID=? AND time>?");
    qDownlink.addBindValue(m_telemetryID);
    qDownlink.addBindValue(m_time);
    ok=qDownlink.exec();
    if(!ok)break;
    if(qDownlink.next())next();

    break;
  }
  if(!ok)pause();
}
void TelemetryPlayer::pause()
{
  if(!m_playing)return;
  m_playing=false;
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
  quint64 tNext=qDownlink.value(2).toULongLong();
  if(tNext<=t){
    quint64 fieldID=qDownlink.value(1).toULongLong();
    VehicleMandalaFact *f=qobject_cast<VehicleMandalaFact*>(_db->recFacts.key(fieldID));
    if(f) f->setValueLocal(qDownlink.value(3));
    if(!qDownlink.next())pause();
    else{
      tNext=qDownlink.value(2).toULongLong();
    }
  }
  if(m_time!=t){
    m_time=t;
    emit timeChanged();
  }
  if(!m_playing)return;
  if(tNext>t) timer.start(tNext-t);
  else next();
}
//==============================================================================

