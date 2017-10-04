#include "NodesRequestManager.h"
#include "NodesModel.h"
//=============================================================================
NodesRequest::NodesRequest(uint cmd, const QByteArray sn, const QByteArray data, uint timeout_ms)
 : QObject(),
   cmd(cmd),sn(sn),data(data),timeout_ms(timeout_ms),
   retry(timeout_ms>0?4:0),active(false),forceNext(false)
{
  if(cmd>=apc_user || cmd<apc_conf_inf){
    retry=0;
  }
  connect(&timer,SIGNAL(timeout()),this,SLOT(timerTimeout()));
}
//=============================================================================
void NodesRequest::trigger()
{
  time.start();
  active=true;
  emit send(cmd,sn,data);
  if(timeout_ms){
    timer.start(timeout_ms);
  }else emit done(QByteArray(),forceNext);
}
//=============================================================================
void NodesRequest::serviceData(const QByteArray &packet_data)
{
  _bus_packet &packet=*(_bus_packet*)packet_data.data();
  QByteArray rx_sn((const char*)packet.srv.sn,sizeof(_node_sn));
  if(sn!=rx_sn)return;
  if(packet.srv.cmd!=apc_ack && packet.srv.cmd!=cmd)return;
  QByteArray rx_data((const char*)packet.srv.data,packet_data.size()-bus_packet_size_hdr_srv);
  switch(packet.srv.cmd){
    case apc_loader:
      break;
    case apc_ack:
      if(rx_data.size()==1 && packet.srv.data[0]==cmd)break;
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
  forceNext=true;
  emit done(packet_data,forceNext);
}
//=============================================================================
void NodesRequest::timerTimeout()
{
  timer.stop();
  //request timeout
  if(retry){
    retry--;
    trigger();
    return;
  }
  active=false;
  forceNext=true;
  qWarning("%s (%u/%u)",tr("Timeout").toUtf8().data(),cmd,data.size());
  //if(data.size())qWarning()<<data.toHex().toUpper();
  emit done(QByteArray(),forceNext);
}
//=============================================================================
//=============================================================================
NodesRequestManager::NodesRequestManager(NodesModel *model)
 : model(model)
{
  m_enabled=true;
  m_busy=false;
  emitDone=false;
  disableOnDone=false;
  activeCount=0;
  timer.setSingleShot(true);
  timer.setInterval(100);
  connect(&timer,SIGNAL(timeout()),this,SLOT(next()));
  disableTimer.setSingleShot(true);
  disableTimer.setInterval(10000);
  connect(&disableTimer,SIGNAL(timeout()),this,SLOT(disableTimerTimeout()));
  doneTimer.setSingleShot(true);
  doneTimer.setInterval(500);
  connect(&doneTimer,SIGNAL(timeout()),this,SLOT(allDone()));
}
//=============================================================================
void NodesRequestManager::makeRequest(uint cmd, const QByteArray &sn, const QByteArray &data, uint timeout_ms)
{
  if(cmd!=apc_loader && (!enabled()))return;

  if(disableOnDone)disableTimer.start();
  doneTimer.stop();

  if(timeout_ms<1000)timeout_ms*=5;
  /*if(timeout_ms>0){
    timeout_ms=10000+timeout_ms*10;
  }*/
  NodesRequest *request=NULL;
  foreach(NodesRequest *i,pool){
    if(i->cmd==cmd && i->sn==sn && i->data==data && i->data.size()==data.size()){
      request=i;
      i->timeout_ms=timeout_ms;
      //qDebug("Request timeout updated");
      break;
    }
  }
  if(!request){
    request=new NodesRequest(cmd,sn,data,timeout_ms);
    if(cmd==apc_loader)pool.insert(0,request);
    else pool.append(request);
    connect(request,SIGNAL(send(uint,QByteArray,QByteArray)),model->mvar,SLOT(send_srv(uint,QByteArray,QByteArray)));
    connect(model->mvar,SIGNAL(serviceRequest(QByteArray)),request,SLOT(serviceData(QByteArray)));
    connect(request,SIGNAL(done(QByteArray,bool)),this,SLOT(requestDone(QByteArray,bool)));
  }
  emitDone|=timeout_ms;
  if(timeout_ms && m_busy==false){
    m_busy=true;
    emit busyChanged(true);
  }
  if(!timer.isActive())next();
}
//=============================================================================
void NodesRequestManager::stop()
{
  timer.stop();
  qDeleteAll(pool);
  pool.clear();
  activeCount=0;
  if(emitDone) doneTimer.start();
  emitDone=false;
}
//=============================================================================
void NodesRequestManager::next()
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
  foreach(NodesRequest *i,pool){
    if(i->active)continue;
    activeCount++;
    i->trigger();
    if(i->cmd!=apc_loader)timer.start();
    return;
  }
}
//=============================================================================
void NodesRequestManager::requestDone(const QByteArray &packet_data, bool forceNext)
{
  Q_UNUSED(packet_data)
  NodesRequest *request=static_cast<NodesRequest*>(sender());
  pool.removeAll(request);
  request->deleteLater();
  if(activeCount)activeCount--;
  //forceNext=false;
  if(forceNext || (!timer.isActive()))next();
}
//=============================================================================
void NodesRequestManager::allDone()
{
  if(!pool.isEmpty())return;
  if(disableOnDone){
    disableOnDone=false;
    disableTimer.stop();
    setEnabled(false);
  }
  m_busy=false;
  emit busyChanged(false);
  emit finished();
}
//=============================================================================
bool NodesRequestManager::enabled()
{
  return m_enabled;
}
void NodesRequestManager::setEnabled(bool v)
{
  if(m_enabled==v)return;
  m_enabled=v;
  emit enabledChanged(v);
  if(!m_enabled){
    stop();
    //qDebug()<<"RequestManager Disabled";
  }
}
bool NodesRequestManager::busy()
{
  return m_busy;
}
//=============================================================================
void NodesRequestManager::enableTemporary()
{
  setEnabled(true);
  disableOnDone=true;
  disableTimer.start();
}
void NodesRequestManager::disableTimerTimeout()
{
  if(!disableOnDone)return;
  if(!pool.isEmpty()){
    disableTimer.start();
    return;
  }
  setEnabled(false);
}
//=============================================================================


