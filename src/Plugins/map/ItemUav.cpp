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
#include "ItemUav.h"
#include "QMandala.h"
#include <QAction>
#include "MapView.h"
#include "Datalink.h"
//=============================================================================
ItemUav::ItemUav(MapView *view,QMandalaItem *mvar)
  :ItemBase(view,mvar,QPixmap(getIconPixmap(mvar).size()),true,QSettings().value("smooth_uav").toBool()),
  uavPixmap(getIconPixmap(mvar)),uavIcon(uavPixmap),
  hdgWheel(QPixmap(":/icons/old/hdg-wheel.svg")),
  hdgArrow(QPixmap(":/icons/old/hdg-arrow.svg"))
{
  noUpdateBinds=true;
  memset(theta,0xFF,sizeof(theta));
  setZValue(500);
  QPixmap ipm(pixmap().size());
  ipm.fill(Qt::transparent);
  setPixmap(ipm);
  setFlag(ItemHasNoContents);

  //qDebug()<<"ItemUav::new";

  uavIcon.setTransformationMode(Qt::SmoothTransformation);
  uavIcon.setCacheMode(DeviceCoordinateCache);
  uavIcon.setOffset(-uavPixmap.width()/2.0,-uavPixmap.height()/2.0);
  uavIcon.setParentItem(this);
  //uavShadow.setBlurRadius(12);
  //uavShadow.setOffset(1,1);
  //uavShadow.setColor(Qt::yellow);
  //uavIcon.setGraphicsEffect(&uavShadow);

  ahrsIcon=new QGraphicsPixmapItem(uavPixmap);
  ahrsIcon->setFlag(ItemIgnoresTransformations);
  ahrsIcon->setTransformationMode(Qt::SmoothTransformation);
  ahrsIcon->setCacheMode(DeviceCoordinateCache);
  ahrsIcon->setOffset(-uavPixmap.width()/2.0,-uavPixmap.height()/2.0);
  ahrsIcon->setOpacity(0.5);
  ahrsIcon->setVisible(false);
  addToScene(ahrsIcon);


  hdgWheel.setTransformationMode(Qt::SmoothTransformation);
  hdgWheel.setCacheMode(DeviceCoordinateCache);
  if(!hdgWheel.pixmap().isNull()) hdgWheel.setOffset(-hdgWheel.pixmap().width()/2.0,-hdgWheel.pixmap().height()/2.0);
  hdgWheel.setParentItem(this);

  hdgArrow.setTransformationMode(Qt::SmoothTransformation);
  hdgWheel.setCacheMode(DeviceCoordinateCache);
  if(!hdgArrow.pixmap().isNull()) hdgArrow.setOffset(-hdgArrow.pixmap().width()/2.0,-hdgArrow.pixmap().height()/2.0);
  hdgArrow.setParentItem(this);

  txtInfo.setParentItem(this);
  //txtInfo.setFlags(ItemStacksBehindParent);
  txtInfo.setZValue(1000);

  txtInfo.setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
  txtInfo.setColor(QColor(55,121,100));
  txtInfo.setFontColor(Qt::white);
  txtInfo.setText("H");
  txtInfo.setVisible(true);

  pathCam.setFlags(ItemStacksBehindParent);
  pathCam.setPen(QPen(Qt::gray,0));//,Qt::DashLine));
  pathCam.setBrush(QBrush(QColor(255,255,255,50)));
  pathCam.setParentItem(this);
  pathCam.setZValue(200);
  //pathCam.setCacheMode(DeviceCoordinateCache);

  pathCam2.setFlags(ItemStacksBehindParent);
  pathCam2.setPen(Qt::NoPen);
  pathCam2.setBrush(QBrush(QColor(155,255,155,50)));
  pathCam2.setParentItem(this);
  pathCam2.setZValue(200);
  //pathCam2.setCacheMode(DeviceCoordinateCache);

  if(!QSettings().contains("mapCamSpan"))QSettings().setValue("mapCamSpan",45);
  camSpan=QSettings().value("mapCamSpan").toDouble();
  camSpan2=QSettings().value("mapCamSpan2",camSpan*0.25).toDouble();
  if(!QSettings().contains("mapCamRelative"))QSettings().setValue("mapCamRelative",true);
  camRelative=QSettings().value("mapCamRelative").toBool();

  setShowHdg(false);//QSettings().value("mapShowHdg").toBool());

  {
    //pathCmdCrs.setFlags(ItemStacksBehindParent);
    pathCmdCrs.setParentItem(this);
    pathCmdCrs.setZValue(98);
    pathCmdCrs.setCacheMode(DeviceCoordinateCache);
    QPen pen=QPen(Qt::magenta,2,Qt::DashLine);
    pen.setCosmetic(true);
    pathCmdCrs.setPen(pen);
    QPainterPath path;
    double sz=100,pad=20;
    path.moveTo(0,-pad);
    path.lineTo(0,-sz);
    pathCmdCrs.setPath(path);
  }

  {
    //pathCrs.setFlags(ItemStacksBehindParent);
    pathCrs.setPen(QPen(Qt::white,0,Qt::DashLine));
    pathCrs.setParentItem(this);
    pathCrs.setZValue(98);
    pathCrs.setCacheMode(DeviceCoordinateCache);
    QPen pen=QPen(Qt::white,2,Qt::DashLine);
    pen.setCosmetic(true);
    pathCrs.setPen(pen);
    QPainterPath path;
    double sz=90,pad=50;
    path.moveTo(0,-pad);
    path.lineTo(0,-sz);
    pathCrs.setPath(path);
  }

  {
    //pathHdg.setFlags(ItemStacksBehindParent);
    pathHdg.setParentItem(this);
    pathHdg.setZValue(98);
    pathHdg.setCacheMode(DeviceCoordinateCache);
    QPen pen=QPen(Qt::white,2,Qt::SolidLine);
    pen.setCosmetic(true);
    pathHdg.setPen(pen);
    QPainterPath path;
    double sz=50,pad=40;
    path.moveTo(0,-pad);
    path.lineTo(0,-sz);
    pathHdg.setPath(path);
  }

  traceFromFile=new QGraphicsItemGroup();
  addToScene(traceFromFile);
  traceCurrent=new QGraphicsItemGroup();
  addToScene(traceCurrent);
  connect(QMandala::instance()->local->rec,SIGNAL(fileLoaded()),this,SLOT(mandalaFileLoaded()),Qt::QueuedConnection);
  setShowTrace(false);
  connect(view,SIGNAL(scaled()),this,SLOT(updateTraceFromFile()),Qt::QueuedConnection);
  connect(view,SIGNAL(scaled()),this,SLOT(updateTraceCurrent()),Qt::QueuedConnection);

  cmd_NE=new QGraphicsEllipseItem();
  cmd_NE->setCacheMode(DeviceCoordinateCache);
  cmd_NE->setPen(QPen(Qt::magenta,0));
  cmd_NE->setBrush(QColor(0,255,0,20));
  //cmd_NE->setVisible(false);
  cmd_NE->setZValue(99);
  //cmd_NE->setOpacity(0.6);
  addToScene(cmd_NE);
  dist=0;
  dist_file=0;

  stbyCircle=new QGraphicsEllipseItem();
  stbyCircle->setCacheMode(DeviceCoordinateCache);
  QPen pen(QColor(100,100,255),0);
  pen.setCosmetic(true);
  stbyCircle->setPen(pen);
  stbyCircle->setBrush(QBrush(QColor(10,10,100,20)));
  //stbyCircle->setVisible(false);
  stbyCircle->setZValue(99);
  addToScene(stbyCircle);

  //venergy
  veCircle=new QGraphicsEllipseItem();
  QPen veCirclePen=QPen(Qt::cyan,2,Qt::DashLine);
  veCirclePen.setCosmetic(true);
  veCircle->setPen(veCirclePen);
  //veCircle->setParentItem(this);
  veCircle->setZValue(90);
  veCircle->setVisible(false);
  addToScene(veCircle);

  //restricted
  restrictedCircle=new QGraphicsEllipseItem();
  QPen restrictedCirclePen=QPen(Qt::cyan,2,Qt::DashLine);
  restrictedCirclePen.setCosmetic(true);
  restrictedCircle->setPen(restrictedCirclePen);
  //restrictedCircle->setParentItem(this);
  restrictedCircle->setZValue(90);
  restrictedCircle->setVisible(false);
  addToScene(restrictedCircle);

  testLine=new QGraphicsLineItem();
  QPen testLinePen=QPen(Qt::white,4,Qt::DashLine);
  testLinePen.setCosmetic(true);
  testLine->setPen(testLinePen);
  testLine->setZValue(90);
  testLine->setVisible(false);
  addToScene(testLine);

  //gPerformance
  gPerf=0;
  veCircleGS=new QGraphicsEllipseItem();
  QPen veCircleGSPen=QPen(QColor(0,255,255,50),10,Qt::SolidLine);
  veCircleGSPen.setCosmetic(true);
  veCircleGS->setPen(veCircleGSPen);
  //veCircleGS->setParentItem(this);
  veCircleGS->setZValue(90);
  veCircleGS->setCacheMode(DeviceCoordinateCache);
  veCircleGS->setVisible(false);
  double rGS=view->mapMetersToScene(50,0);
  QRectF rectGS(-rGS,-rGS,rGS*2,rGS*2);
  veCircleGS->setRect(rectGS);
  addToScene(veCircleGS);

  //camTarget
  camTarget=new QGraphicsEllipseItem();
  QPen camTargetPen=QPen(QColor(255,255,0,90),20,Qt::SolidLine);
  camTargetPen.setCosmetic(true);
  camTarget->setPen(camTargetPen);
  //camTarget->setParentItem(this);
  camTarget->setZValue(90);
  camTarget->setCacheMode(DeviceCoordinateCache);
  camTarget->setVisible(false);
  double camTargetR=view->mapMetersToScene(50,0);
  camTarget->setRect(-camTargetR,-camTargetR,camTargetR*2,camTargetR*2);
  addToScene(camTarget);

  connect(this,SIGNAL(smoothMoved(QPointF)),view,SLOT(followUAV(QPointF)));

  connect(view,SIGNAL(scaled()),this,SLOT(update_LD()));
  connect(view,SIGNAL(scaled()),this,SLOT(update_stats()));

  connect(this,SIGNAL(statsUpdated()),view,SIGNAL(updateStats()),Qt::QueuedConnection);

  statsTimer.setSingleShot(true);
  statsTimer.setInterval(1000);
  connect(&statsTimer,SIGNAL(timeout()),this,SLOT(update_stats_do()));

  //var bindings
  bind(SLOT(update_theta()),QStringList()<<"roll"<<"pitch"<<"yaw"<<"cmd_course"<<"course"<<"camcmd_yaw"<<"cam_yaw");
  bind(SLOT(update_gPerf()),QStringList()<<"gSpeed"<<"gps_Vdown");
  bind(SLOT(update_pos()),QStringList()<<"gps_lat"<<"gps_lon"<<"home_lat"<<"home_lon");
  bind(SLOT(update_cmdNE()),QStringList()<<"cmd_north"<<"cmd_east"<<"home_lat"<<"home_lon");
  bind(SLOT(update_LD()),QStringList()<<"altitude"<<"airspeed"<<"ldratio"<<"venergy"<<"cas2tas"<<"windHdg"<<"windSpd");
  bind(SLOT(update_stats()),QStringList()<<"altitude"<<"gSpeed"<<"course"<<"mode");
  bind(SLOT(update_stby()),QStringList()<<"mode"<<"turnR"<<"cmd_north"<<"cmd_east"<<"home_lat"<<"home_lon");
  bind(SLOT(update_Restricted()),QStringList()<<"gps_lat"<<"gps_lon");

  connect(this,SIGNAL(apcfgChanged()),this,SLOT(update_cmdNE()),Qt::QueuedConnection);
  connect(this,SIGNAL(apcfgChanged()),this,SLOT(update_stats()),Qt::QueuedConnection);
  update_all_binds();

  connect(this,SIGNAL(moved()),this,SLOT(update_Restricted()),Qt::QueuedConnection);

  connect(mvar,SIGNAL(dlinkDataChanged(bool)),this,SLOT(updateLinkState()));
  connect(mvar,SIGNAL(xpdrDataChanged(bool)),this,SLOT(updateLinkState()));
  connect(QMandala::instance(),SIGNAL(currentChanged(QMandalaItem*)),this,SLOT(updateLinkState()));
  updateLinkState();
}
//=============================================================================
void ItemUav::setCurrent(bool v)
{
  //qDebug()<<"ItemUav::setCurrent"<<v;
  m_current=v;
  update_all_binds();

  bool bShowCrs=mvar->ident.vclass!=IDENT::GCU;
  pathCmdCrs.setVisible(v&&bShowCrs);
  pathHdg.setVisible(v&&bShowCrs);
  pathCrs.setVisible(v&&bShowCrs);
  cmd_NE->setVisible(v&&bShowCrs);

  traceCurrent->setVisible(v&&m_showTrace);
  traceFromFile->setVisible(v&&m_showTrace);

  setShowHdg(false);

  QPixmap px=uavPixmap;
  if(!v){
    setZValue(510); //nonselected overlap higher
    //scale
    bool bScale=mvar->ident.vclass!=IDENT::GCU;
    if(bScale)
      px=px.scaled(px.size()*0.7,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    //grayscale
    QGraphicsScene scene;
    QGraphicsPixmapItem item(px);
    QGraphicsColorizeEffect effect;
    effect.setColor(QColor(0,0,0));
    effect.setStrength(0.7);
    item.setGraphicsEffect(&effect);
    scene.addItem(&item);
    QImage res(px.size(), QImage::Format_ARGB32);
    res.fill(Qt::transparent);
    QPainter ptr(&res);
    scene.render(&ptr,QRectF(),QRectF(0,0,px.width(),px.height()));
    px=QPixmap::fromImage(res);
  }else setZValue(500); //lower
  uavIcon.setPixmap(px);
  uavIcon.setOffset(-px.width()/2.0,-px.height()/2.0);
}
//=============================================================================
QPixmap ItemUav::getIconPixmap(QMandalaItem *m)
{
  if(m->ident.vclass==IDENT::GCU) return QPixmap(":/icons/old/gcu.png");
  return QPixmap(":/icons/old/uav.png");
}
//=============================================================================
//=============================================================================
void ItemUav::update_theta()
{
  double yaw=mvar->theta[2];
  QTransform tf;
  const double psf=0.05;
  tf.scale(psf,psf);
  tf.rotate(yaw,Qt::ZAxis);
  bool bScale=mvar->ident.vclass!=IDENT::GCU;
  bool bAtt=mvar->dlinkData()&&bScale;
  if(bAtt){
    tf.rotate(-mvar->limit(mvar->theta[1]*8.0,-40,40),Qt::XAxis);
    tf.rotate(-mvar->limit(mvar->theta[0]*2.0,-60,60),Qt::YAxis);
  }
  tf.scale(1.0/psf,1.0/psf);
  uavIcon.setTransform(tf);

  if(mvar->cmode&cmode_ahrs){
    ahrsIcon->setPos(view->mapNEtoScene(mvar->user1+mvar->pos_NE[0],mvar->user2+mvar->pos_NE[1],mvar->home_pos[0],mvar->home_pos[1]));
    ahrsIcon->setTransform(tf);
    ahrsIcon->setVisible(m_current);
  }else if(ahrsIcon->isVisible()) ahrsIcon->setVisible(false);

  pathCmdCrs.setRotation(mvar->field(idx_cmd_course)->value());
  pathCrs.setRotation(mvar->course);
  pathHdg.setRotation(mvar->theta[2]);


  hdgArrow.setRotation(mvar->boundAngle(mvar->course));

  //camera view
  const double camDist=2*sqrt(view->size().width()*view->size().width()+view->size().height()*view->size().height());//view->mapMetersToScene(1000,mvar->gps_pos[0]);
  const double camA=camRelative?mvar->theta[2]:0;
  QPainterPath path=QPainterPath();
  double sz=camDist;
  path.moveTo(0,-sz);
  path.lineTo(0,0);
  path.moveTo(0,0);
  path.arcTo(-sz,-sz,sz*2,sz*2,90-(camSpan),(camSpan));
  pathCam.setPath(path);
  pathCam.setRotation(camA+mvar->cam_theta[2]-camSpan/2.0);
  path=QPainterPath();
  sz=camDist;
  path.moveTo(0,-sz);
  path.lineTo(0,0);
  path.moveTo(0,0);
  path.arcTo(-sz,-sz,sz*2,sz*2,90-(camSpan2),(camSpan2));
  pathCam2.setPath(path);
  pathCam2.setRotation(camA+mvar->cam_theta[2]-camSpan2/2.0);
}
//=============================================================================
void ItemUav::update_gPerf()
{
  //calc gPerf
  _var_float spd=mvar->gSpeed;
  _var_float dspd=mvar->gps_vel[2];
  if(dspd>0.5 && spd>0.5){
    _var_float v=(dspd==0)?0:spd/dspd;
    mvar->filter_m(v,&gPerf,(v<0?0.001:0.01));
  }
}
//=============================================================================
void ItemUav::update_pos()
{
  QPointF uavPos=QPointF(mvar->gps_pos[0],mvar->gps_pos[1]);
  if(uavPos==QPointF(0,0))
    uavPos=QPointF(mvar->home_pos[0],mvar->home_pos[1]);
  if((mvar->status&status_gps)||(uavPos!=QPointF(0,0))){
    setPosLL(uavPos);
    trace(uavPos.x(),uavPos.y(),mvar->gps_pos[2]);
  }
  veCircle->setPos(pos());
  restrictedCircle->setPos(pos());
  testLine->setPos(pos());

  //gPerf
  double sf=view->mapMetersToScene(1,getPosLL().x());//*view->transform().m11();
  Point pGS(mvar->rotate(Point(-mvar->altitude*gPerf*sf,0),mvar->course));
  veCircleGS->setPos(pos()+QPointF(pGS[1],pGS[0]));
  veCircleGS->setVisible(m_current && gPerf>3 && mvar->gSpeed>1 && mvar->altitude>5);

  //camTarget
  double camAlt=mvar->gps_pos[2]-mvar->home_pos[2];
  if((mvar->power&power_payload)&&(camAlt>0)&&(!mvar->cam_theta.isNull())){
    Point camTargetNE;
    if(mvar->cam_theta[0]==0.0){
      //down looking gimal
      camTargetNE=mvar->rotate(Point(camAlt*tan((90.0+mvar->cam_theta[1])*D2R),0),-mvar->cam_theta[2]);
    }else{
      //front gimbal
      camTargetNE[1]=camAlt*tan(-mvar->cam_theta[0]*D2R);
      camTargetNE[0]=mvar->distance(camAlt,camTargetNE[1])*tan((90.0+mvar->cam_theta[1])*D2R);
      camTargetNE=mvar->rotate(camTargetNE,-mvar->theta[2]);
    }
    camTarget->setPos(pos()+QPointF(camTargetNE[1],-camTargetNE[0])*sf);
    camTarget->setVisible(m_current);
    //update track pos for stats
    Point ll=mvar->ne2ll(camTargetNE,Vect(posLL.x(),posLL.y(),mvar->home_pos[2]));
    mvar->cam_tpos=Vect(ll[0],ll[1],mvar->home_pos[2]);
  }else camTarget->setVisible(false);
}
//=============================================================================
void ItemUav::update_cmdNE()
{
  cmd_NE->setPos(view->mapNEtoScene(mvar->cmd_NE[0],mvar->cmd_NE[1],mvar->home_pos[0],mvar->home_pos[1]));
  double r=mvar->apcfg.value("wpt_snap").toUInt();
  if(r<=0 && (!posLL.isNull()))r=2;
  else r=view->mapMetersToScene(r,posLL.x());
  cmd_NE->setRect(QRectF(-r,-r,r*2,r*2));
}
//=============================================================================
void ItemUav::update_LD(void)
{
  if(mvar->altitude<=0)return;
  if(mvar->airspeed<=0)return;
  double v=mvar->altitude*mvar->ldratio;//*k;
  double sf=view->mapMetersToScene(1,getPosLL().x());//*view->transform().m11();
  double r=v*sf;
  QRectF rect(-r,-r,r*2,r*2);
  double wnd_r=(mvar->windHdg)*D2R;
  rect.translate(r*mvar->windSpd/(mvar->airspeed*(mvar->cas2tas>0?mvar->cas2tas:1.0))*QPointF(sin(wnd_r),-cos(wnd_r)));
  veCircle->setRect(rect);

  if(mvar->venergy>0.5){
    QPen pen(veCircle->pen());
    pen.setColor(Qt::cyan);
    veCircle->setPen(pen);
  }else if(mvar->venergy<-0.5){
    QPen pen(veCircle->pen());
    pen.setColor(Qt::red);
    veCircle->pen().setColor(Qt::red);
    veCircle->setPen(pen);
  }
  veCircle->setVisible(m_current && mvar->airspeed>0.5);
}
//=============================================================================
void ItemUav::update_Restricted(void)
{/*return;


  //mvar->field(idx_cmd_course)->setValue(var->heading(Point(var->user2,var->user3)));


  double v=var->user1;
  //if(isinf(v))v=0;
  double sf=view->mapMetersToScene(1,getPosLL().x());
  double r=v*sf;
  QRectF rect(-r,-r,r*2,r*2);
  restrictedCircle->setRect(rect);

  double Ksfl=500000;
  Point Lne(var->user2*sf*Ksfl,var->user3*sf*Ksfl);
  testLine->setLine(0,0,Lne[1],-Lne[0]);
  testLine->setPos(pos());
  testLine->setVisible(true);

  if(var->user4==0){
    QPen pen(restrictedCircle->pen());
    pen.setColor(Qt::cyan);
    restrictedCircle->setPen(pen);
  }else{
    QPen pen(restrictedCircle->pen());
    pen.setColor(Qt::red);
    restrictedCircle->pen().setColor(Qt::red);
    restrictedCircle->setPen(pen);
  }
  restrictedCircle->setPos(pos());
  restrictedCircle->setVisible(true);*/
}
//=============================================================================
void ItemUav::update_stats(void)
{
  if(statsTimer.isActive())return;
  statsTimer.start();
}
//=============================================================================
void ItemUav::update_stats_do(void)
{
  QString s;
  if(mvar->ident.vclass==IDENT::GCU){
    s+=QString("%1").arg(mvar->ident.callsign);
  }else{
    if(QMandala::instance()->size()>1)s+=QString("%1\n").arg(mvar->ident.callsign);
    s+=QString("H%1").arg(mvar->altitude>50?(int)mvar->altitude/10*10:(int)mvar->altitude);
    if(fabs(mvar->vspeed)>=1)s+=QString(" %1%2").arg(mvar->vspeed<0?"":"+").arg((int)mvar->vspeed);
    s+=QString("\n%1").arg(mvar->field(idx_mode)->text());
    if(mvar->dlinkData() && mvar->stage>1)s+=QString("/%1").arg(mvar->stage);
  }
  txtInfo.setText(s);
  if(mvar->ident.vclass!=IDENT::GCU)txtInfo.setAlignment(Qt::AlignHCenter|Qt::AlignLeft);
  else txtInfo.setAlignment(Qt::AlignTop|Qt::AlignRight);//|Qt::AlignHCenter);
}
//=============================================================================
void ItemUav::updateLinkState(void)
{
  if(mvar==QMandala::instance()->local){
    setVisible(QMandala::instance()->isLocal() || mvar->dlinkData() || mvar->xpdrData());
  }
  if(mvar->ident.vclass==IDENT::GCU){
    txtInfo.setColor(QColor(55,121,197));
  }else{
    if(mvar->dlinkData())txtInfo.setColor(QColor(55,121,100));
    else if(mvar->xpdrData())txtInfo.setColor(QColor(55,100,121));
    else txtInfo.setColor(QColor(121,55,100));
  }
}
//=============================================================================
void ItemUav::update_stby()
{
  stbyCircle->setVisible(m_current && mvar->mode==mode_STBY);
  stbyCircle->setPos(view->mapNEtoScene(mvar->cmd_NE[0],mvar->cmd_NE[1],mvar->home_pos[0],mvar->home_pos[1]));
  double r=fabs(mvar->turnR);
  r=view->mapMetersToScene(r,getPosLL().x());
  stbyCircle->setRect(QRectF(-r,-r,r*2,r*2));
}
//=============================================================================
//=============================================================================
void ItemUav::clearTrace()
{
  dist=0;
  traceData.clear();
  qDeleteAll(traceCurrent->childItems());
}
void ItemUav::clearTraceF()
{
  dist_file=0;
  traceDataFromFile.clear();
  qDeleteAll(traceFromFile->childItems());
}
//=============================================================================
void ItemUav::mandalaFileLoaded()
{
  traceDataFromFile.clear();
  this->dist_file=0;
  Vect llh_prev;
  int igps_lat=QStringList(mvar->names).indexOf("gps_lat");
  int igps_lon=QStringList(mvar->names).indexOf("gps_lon");
  int igps_hmsl=QStringList(mvar->names).indexOf("gps_hmsl");
  foreach(const FlightDataFile::ListDouble &vlist,QMandala::instance()->local->rec->file.data){
    const double lat=vlist.at(igps_lat);
    const double lon=vlist.at(igps_lon);
    const double hmsl=vlist.at(igps_hmsl);
    if(lat==0.0&&lon==0.0)continue;
    Vect llh=Vect(lat,lon,hmsl);
    if(llh_prev.isNull())llh_prev=llh;
    this->dist_file+=mvar->distance(mvar->llh2ne(llh,llh_prev));
    llh_prev=llh;
    QPointF pt(view->mapToSceneLL(lat,lon));
    traceDataFromFile.append(pt);
    //QCoreApplication::processEvents();
  }
  updateTraceFromFile();
  emit statsUpdated();
}
//=============================================================================
void ItemUav::trace(double lat, double lon, double hmsl)
{
  if(!Datalink::instance()->online())return;
  if(mvar->airspeed<5.0)return;
  Vect llh=Vect(lat,lon,hmsl);
  if(!trace_llh_prev.isNull()) dist+=mvar->distance(mvar->llh2ne(llh,trace_llh_prev));
  trace_llh_prev=llh;
  QPointF pt(view->mapToSceneLL(lat,lon));
  traceData.append(pt);
  if(traceData.size()<=1){
    trace_s=pt;
    return;
  }
  QPointF pt_s(trace_s);
  double dpx=QPoint(view->mapFromScene(pt)-view->mapFromScene(pt_s)).manhattanLength();
  if(dpx<20)return;
  trace_s=pt;
  emit statsUpdated();
  traceData.append(pt);
  QGraphicsPathItem *traceSegment=new QGraphicsPathItem();
  traceSegment->setCacheMode(DeviceCoordinateCache);
  traceSegment->setPen(QPen(Qt::black,0,Qt::DashLine));
  traceSegment->setPos(pt_s);
  QPainterPath ps;
  ps.moveTo(0,0);
  ps.lineTo(pt-pt_s);
  pt_s=pt;
  //ps.addRect(ps.boundingRect());
  traceSegment->setPath(ps);
  traceSegment->setParentItem(traceCurrent);
}
//=============================================================================
void ItemUav::updateTraceCurrent(void)
{
  updateTrace(traceData,traceCurrent,QPen(Qt::black,0,Qt::DashLine));
}
void ItemUav::updateTraceFromFile(void)
{
  updateTrace(traceDataFromFile,traceFromFile,QPen(Qt::blue,0,Qt::DashLine));
}
//=============================================================================
void ItemUav::updateTrace(const QList<QPointF> &data, QGraphicsItem *item, const QPen &pen)
{
  QList<QGraphicsItem*>list;
  QPointF pt_s;
  if(!data.isEmpty())pt_s=data.first();
  foreach(const QPointF &pt,data){
    double dpx=QPoint(view->mapFromScene(pt)-view->mapFromScene(pt_s)).manhattanLength();
    if(dpx<20)continue;
    QGraphicsPathItem *traceSegment=new QGraphicsPathItem();
    traceSegment->setCacheMode(DeviceCoordinateCache);
    traceSegment->setPen(pen);
    traceSegment->setPos(pt_s);
    QPainterPath ps;
    ps.moveTo(0,0);
    ps.lineTo(pt-pt_s);
    pt_s=pt;
    //ps.addRect(ps.boundingRect());
    traceSegment->setPath(ps);
    list.append(traceSegment);
    //QCoreApplication::processEvents();
  }
  qDeleteAll(item->childItems());
  foreach(QGraphicsItem *i,list)
    i->setParentItem(item);
}
//=============================================================================
void ItemUav::setShowTrace(bool show)
{
  m_showTrace=show;
  traceCurrent->setVisible(show);
  traceFromFile->setVisible(show);
}
//=============================================================================
void ItemUav::setShowHdg(bool v)
{
  m_showHdg=v;
  hdgWheel.setVisible(v);
  hdgArrow.setVisible(v);
  pathCam.setVisible(v);
  pathCam2.setVisible(v);
  //QSettings().setValue("mapShowHdg",v);
}
//=============================================================================
//=============================================================================


