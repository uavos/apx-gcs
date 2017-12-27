#include "TelemetryPlayer.h"
//==============================================================================
TelemetryPlayer::TelemetryPlayer(QObject *parent)
  : QObject(parent),
    m_telemetryID(0),
    m_time(0),
    m_playing(false)
{
}
//==============================================================================
void TelemetryPlayer::setTelemetryID(quint64 v)
{
  if(m_telemetryID==v)return;
  m_telemetryID=v;
}
//==============================================================================
void TelemetryPlayer::setTime(quint64 v)
{
  if(m_time==v)return;
  m_time=v;
  emit timeChanged();
}
quint64 TelemetryPlayer::time()
{
  return m_time;
}
bool TelemetryPlayer::playing()
{
  return m_playing;
}
//==============================================================================
//==============================================================================
//==============================================================================
void TelemetryPlayer::play()
{
  if(m_playing)return;
}
void TelemetryPlayer::pause()
{
  if(!m_playing)return;
}
void TelemetryPlayer::rewind()
{
  pause();
  setTime(0);
}
//==============================================================================

