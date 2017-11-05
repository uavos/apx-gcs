#include "Player.h"
#include <QtWidgets>
#include <QUdpSocket>
#include <Vehicles>
//==============================================================================
Player::Player(QWidget *parent)
  : QDialog(parent),rec(Vehicles::instance()->f_local->f_recorder)
{
  setupUi(this);
  setWindowFlags(Qt::Tool);
  this->setModal(false);
  QToolBar *toolBar=new QToolBar(this);
  toolBar->setIconSize(QSize(16,16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  toolBar->layout()->setMargin(0);
  toolBarLayout->insertWidget(0,toolBar);
  toolBar->addAction(aPlay);
  toolBar->addAction(aStop);

  connect(this,SIGNAL(replay_progress(uint)),rec,SIGNAL(replay_progress(uint)));
  connect(this,SIGNAL(finished(int)),aStop,SLOT(trigger()));
  connect(rec,SIGNAL(fileLoaded()),this,SLOT(mandalaFileLoaded()),Qt::QueuedConnection);
  connect(&timer,SIGNAL(timeout()),this,SLOT(timerStep()));
  pos_ms=total_ms=0;
  timer.setInterval(10);  //100Hz

  connect(this,&Player::frameUpdated,Vehicles::instance()->f_local->f_mandala,&VehicleMandala::dataReceived);
}
void Player::closeEvent(QCloseEvent *event)
{
  Q_UNUSED(event)
  reject();
}
//==============================================================================
void Player::mandalaFileLoaded()
{
  aStop->trigger();
  if(rec->file.time.size()>2){
    pos_ms=rec->file.time.first();
    total_ms=rec->file.time.last();
  }else total_ms=pos_ms=0;
  updateStats();
}
//=============================================================================
void Player::updateStats()
{
  setEnabled(total_ms>0);
  lbTime->setText(QTime(0, 0).addSecs(pos_ms/1000.0).toString("hh:mm:ss")+"/"+QTime(0, 0).addSecs(total_ms/1000.0).toString("hh:mm:ss"));
  slider->setMaximum(total_ms);
  slider->setSliderPosition(pos_ms);
  emit timeTrack(pos_ms);
}
//=============================================================================
void Player::on_slider_sliderMoved(int v)
{
  if(total_ms<=0)return;
  aPlay->setChecked(false);
  //find pos_ms in data from slider pos_ms
  pos_ms=0;
  foreach(uint time_ms,rec->file.time){
    if((int)time_ms<v)continue;
    pos_ms=time_ms;
    break;
  }
  updateStats();
  sendData();
}
//=============================================================================
void Player::on_aStop_triggered()
{
  aPlay->setChecked(false);
  pos_ms=rec->file.time.size()?rec->file.time.first():0;
  updateStats();
  rec->recDisable=false;
}
//=============================================================================
void Player::on_aPlay_toggled(bool checked)
{
  if(checked){
    if(Vehicles::instance()->current()->f_vclass->value().toInt()!=Vehicle::LOCAL)
      Vehicles::instance()->selectVehicle(Vehicles::instance()->f_local);
    rec->recDisable=true;
    time.start();
    play_ms=pos_ms;
    timer.start();
  }else{
    //pause
    timer.stop();
    rec->recDisable=false;
  }
}
//=============================================================================
void Player::timerStep()
{
  if(!timer.isActive())return;
  int next_i=rec->file.time.indexOf(pos_ms)+1;
  if(next_i==0 || next_i>=rec->file.time.size()){
    aStop->trigger();
    return;
  }
  uint next_ms=rec->file.time.at(next_i);
  if(time.elapsed()<(int)(next_ms-play_ms))return;
  sendData();
  //send other link packets
  foreach(const QString &s,rec->file.msg.value(pos_ms)){
    if(!s.contains(":")){
    }
    const QString &sname=s.left(s.indexOf(':')).trimmed();
    const QString &svalue=s.right(s.length()-s.indexOf(':')-1).trimmed();

    //TODO: service requests replay
    if(sname=="service"){
      //qDebug("service response");
      /*const QByteArray &ba=QByteArray::fromHex(svalue.toUtf8());
      _bus_packet &packet=*(_bus_packet*)ba.data();
      if(packet.srv.cmd!=apc_conf_write) //dont emit acknowledges
        QMandala::instance()->local->dataRead(ba);*/
    }else if(sname==">service"){
      //qDebug("service request");
      Vehicles::instance()->f_local->downlinkReceived(QByteArray::fromHex(svalue.toUtf8()));
    }else if(sname.contains("flightplan")){
      Vehicles::instance()->f_local->downlinkReceived(QByteArray::fromHex(svalue.toUtf8()));
    }else if(sname=="nodes"){
      //Vehicles::instance()->f_local->dataRead(QByteArray::fromHex(svalue.toUtf8()));
    }else if(sname=="msg"){
      FactSystem::instance()->sound(svalue);
      qDebug("<[RE]%s",svalue.toUtf8().data());
    }else{
      if(s.startsWith("gcu_")){
        //ignore
      }else if(s.startsWith("rc_")){
        //ignore
      }else if(s.startsWith("cam")){
        //ignore
      }else if(sname=="data"){
        //ignore
      }else if(sname=="mission"){
        //ignore
      }else if(sname=="ping"){
        //ignore
      }else if(sname=="tagged"){
        //ignore
      }else if(sname=="xpdr"){
        //ignore
      }else{
        qDebug("%s",s.toUtf8().data());
      }
    }
  }
  emit replay_progress(pos_ms);
  pos_ms=next_ms;
  updateStats();
  timerStep();
}
//=============================================================================
void Player::sendData()
{
  int i=rec->file.time.indexOf(pos_ms);
  if(i<0)return;
  const VehicleRecorder::ListDouble &vlist=rec->file.data.at(i);
  i=0;
  Vehicles::instance()->f_local->f_streamType->setValue(Vehicle::REPLAY);
  foreach(VehicleMandalaFact *f,Vehicles::instance()->f_local->f_mandala->allFacts)
    f->setValueLocal(vlist.at(i++));
  emit frameUpdated(idx_downstream);
}
//=============================================================================

