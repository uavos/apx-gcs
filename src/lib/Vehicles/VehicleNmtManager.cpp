#include "VehicleNmtManager.h"
#include <node.h>
#include <Mandala.h>
//=============================================================================
VehicleNmt::VehicleNmt(uint cmd, const QByteArray sn, const QByteArray data, uint timeout_ms)
 : QObject(),
   cmd(cmd),sn(sn),data(data),timeout_ms(timeout_ms),
   retry(timeout_ms>0?4:0),active(false)
{
  connect(&timer,SIGNAL(timeout()),this,SLOT(timeout()));
  //qDebug()<<"nmt req"<<cmd;
}
//=============================================================================
bool VehicleNmt::equals(uint cmd,const QByteArray &sn,const QByteArray &data)
{
  return this->cmd==cmd && this->sn==sn && this->data==data && this->data.size()==data.size();
}
//=============================================================================
void VehicleNmt::trigger()
{
  active=true;
  emit sendUplink(QByteArray().append((unsigned char)idx_service).append(sn.isEmpty()?QByteArray(sizeof(_node_sn),(char)0):sn).append(cmd).append(data));
  if(timeout_ms){
    timer.start(timeout_ms);
  }else{
    active=false;
    emit finished(this);
  }
}
//=============================================================================
void VehicleNmt::nmtResponse(const QByteArray &packet)
{
  if(packet.size()<(int)bus_packet_size_hdr_srv)return;
  _bus_packet &bus_packet=*(_bus_packet*)packet.data();
  if(bus_packet.id!=idx_service)return;
  QByteArray rx_sn((const char*)bus_packet.srv.sn,sizeof(_node_sn));
  if((!sn.isEmpty()) && sn!=rx_sn)return;
  if(cmd!=bus_packet.srv.cmd)return;
  QByteArray rx_data((const char*)bus_packet.srv.data,packet.size()-bus_packet_size_hdr_srv);
  //qDebug()<<"nmtResponse"<<packet.toHex();

  switch(cmd){
    case apc_ack:
      if(rx_data.size()==1 && bus_packet.srv.data[0]==cmd)break;
      return;
    case apc_conf_dsc:
    case apc_conf_read:
      //response with value of field
      if(rx_data.size()>1 && data.size()==1 && rx_data.at(0)==data.at(0)) break;
      return;
    case apc_conf_write:
      //write field confirm
      if(rx_data.size()==1 && data.size()>1 && rx_data.at(0)==data.at(0)) break;
      //write eeprom confirm
      if(rx_data.size()==1 && data.size()==1 && rx_data.at(0)==data.at(0) && (unsigned char)rx_data.at(0)==0xFF) break;
      return;
    case apc_script_file:
      //response with script file header
      if(rx_data.size()==(int)sizeof(_flash_file) && data.size()==0)break;
      //confirmation of script write request
      if(rx_data.size()==0 && data.size()==sizeof(_flash_file))break;
      return;
    case apc_script_read:
      //response with script data
      if(data.size()==(int)sizeof(_flash_data_hdr) && rx_data.size()>(int)sizeof(_flash_data_hdr))break;
      return;
    case apc_script_write:
      //write script confirm
      if(data.size()>(int)sizeof(_flash_data_hdr) && rx_data.size()==(int)sizeof(_flash_data_hdr))break;
      return;
    case apc_conf_cmds:
      break;
    default:
      //response to cmd request
      if(rx_data.size()>1 && data.size()==0)break;
      return;
  }
  //request done
  timer.stop();
  active=false;
  emit finished(this);
}
//=============================================================================
void VehicleNmt::timeout()
{
  timer.stop();
  //request timeout
  if(retry){
    retry--;
    trigger();
    return;
  }
  active=false;
  //forceNext=true;
  qWarning("%s (%u/%u)",tr("Timeout").toUtf8().data(),cmd,data.size());
  //if(data.size())qWarning()<<data.toHex().toUpper();
  emit finished(this);
}
//=============================================================================
//=============================================================================
VehicleNmtManager::VehicleNmtManager(QObject *parent)
 : QObject(parent)
{
  m_enabled=true;
  m_busy=false;
  //emitDone=false;
  //disableOnDone=false;
  activeCount=0;
  timer.setSingleShot(true);
  timer.setInterval(100);
  connect(&timer,SIGNAL(timeout()),this,SLOT(next()));

  //disableTimer.setSingleShot(true);
  //disableTimer.setInterval(10000);
  //connect(&disableTimer,SIGNAL(timeout()),this,SLOT(disableTimerTimeout()));
  //doneTimer.setSingleShot(true);
  //doneTimer.setInterval(500);
  //connect(&doneTimer,SIGNAL(timeout()),this,SLOT(allDone()));
}
//=============================================================================
void VehicleNmtManager::request(uint cmd, const QByteArray &sn, const QByteArray &data, uint timeout_ms, bool highprio)
{
  //qDebug()<<"VehicleNmt"<<cmd;
  VehicleNmt *request=NULL;
  foreach(VehicleNmt *i,pool){
    if(!i->equals(cmd,sn,data))continue;
    request=i;
    i->timeout_ms=timeout_ms;
    break;
  }
  if(!request){
    request=new VehicleNmt(cmd,sn,data,timeout_ms);
    pool.append(request);
    connect(request,&VehicleNmt::sendUplink,this,&VehicleNmtManager::sendUplink);
    connect(this,&VehicleNmtManager::nmtReceived,request,&VehicleNmt::nmtResponse);
    connect(request,&VehicleNmt::finished,this,&VehicleNmtManager::requestFinished);
  }
  if(highprio){
    pool.removeAll(request);
    pool.insert(0,request);
  }

  if(m_busy==false){
    m_busy=true;
    emit busyChanged(m_busy);
  }
  if(activeCount==0 || (!timer.isActive())) next();
}
//=============================================================================
void VehicleNmtManager::stop()
{
  timer.stop();
  qDeleteAll(pool);
  pool.clear();
  activeCount=0;
  if(m_busy==true){
    m_busy=false;
    emit busyChanged(m_busy);
  }
}
//=============================================================================
void VehicleNmtManager::next()
{
  timer.stop();
  if(pool.isEmpty()){
    stop();
    return;
  }
  if(activeCount>=3){
    //qDebug()<<activeCount;
    timer.start();
    return;
  }
  //find and trigger next request in pool
  foreach(VehicleNmt *i,pool){
    if(i->active)continue;
    activeCount++;
    i->trigger();
    timer.start(); //to continue trigger
    return;
  }
}
//=============================================================================
void VehicleNmtManager::requestFinished(VehicleNmt *request)
{
  //qDebug()<<"requestFinished"<<request->cmd;
  pool.removeAll(request);
  request->deleteLater();
  if(activeCount)activeCount--;
  next();
}
//=============================================================================
bool VehicleNmtManager::enabled()
{
  return m_enabled;
}
void VehicleNmtManager::setEnabled(bool v)
{
  if(m_enabled==v)return;
  m_enabled=v;
  emit enabledChanged(v);
  if(!m_enabled){
    stop();
    //qDebug()<<"RequestManager Disabled";
  }
}
bool VehicleNmtManager::busy()
{
  return m_busy;
}
//=============================================================================

