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
#include "TelemetryPlot.h"
#include <QtGui>
//=============================================================================
TelemetryPlot::TelemetryPlot(QWidget *parent)
  : QwtPlot(parent)
{
  setAutoFillBackground(true);
  setAutoReplot(false);
  setCanvasBackground(QColor(Qt::black));
  setStyleSheet("background-color: rgb(0, 0, 0); gridline-color: rgb(255, 255, 255); color: rgb(255, 255, 255);");

  // legend
  legend = new PlotLegend;
  legend->setFrameStyle(QFrame::NoFrame);//|QFrame::Plain);
  legend->setDefaultItemMode(QwtLegendData::Checkable);
  insertLegend(legend, QwtPlot::RightLegend);
  connect(legend, SIGNAL(checked(QVariant,bool,int)), this, SLOT(showCurve(QVariant,bool,int)));

  // grid
  grid = new QwtPlotGrid;
  grid->setMajorPen(QPen(QColor(80,80,80), 0, Qt::DotLine));
  grid->attach(this);

  //move pan
  panner = new QwtPlotPanner(canvas());
  panner->setMouseButton(Qt::MidButton);
  panner2 = new QwtPlotPanner(canvas());
  panner2->setMouseButton(Qt::LeftButton,Qt::ShiftModifier);

  //zoom
  zoomer=new QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yLeft,canvas());
  zoomer->setMousePattern(QwtEventPattern::MouseSelect1,Qt::LeftButton);
  zoomer->setRubberBand(QwtPicker::RectRubberBand);
  zoomer->setRubberBandPen(QColor(Qt::green));
  zoomer->setTrackerMode(QwtPicker::AlwaysOff);
  zoomer->setTrackerPen(QColor(Qt::white));

  mag=new PlotMagnifier(canvas());
  magX=new PlotMagnifier(canvas());
  magX->setAxisEnabled(QwtPlot::yLeft,false);
  magX->setWheelModifiers(Qt::ShiftModifier);
  magY=new PlotMagnifier(canvas());
  magY->setAxisEnabled(QwtPlot::xBottom,false);
  magY->setWheelModifiers(Qt::ControlModifier);

  picker=new PlotPicker(canvas());
  picker->setStateMachine(new QwtPickerDragPointMachine());
  picker->setTrackerPen(QPen(Qt::white));
  picker->setRubberBandPen(QPen(Qt::white));
  picker->setMousePattern(QwtEventPattern::MouseSelect1,Qt::LeftButton,Qt::ControlModifier);

  pickerPoint=new QwtPlotPicker(canvas());
  pickerPoint->setStateMachine(new QwtPickerClickPointMachine());
  //pickerPoint->setMousePattern(QwtEventPattern::MouseSelect1,Qt::LeftButton,Qt::ControlModifier);
  connect(pickerPoint,SIGNAL(selected(QPointF)),this,SLOT(pointSelected(QPointF)));

  timeCursor=new QwtPlotMarker();
  timeCursor->setLinePen(QPen(Qt::gray));
  timeCursor->setLineStyle(QwtPlotMarker::VLine);
  timeCursor->attach(this);

  canvas()->setCursor(Qt::ArrowCursor);

  /*foreach(VehicleMandalaFact *f,Vehicles::instance()->f_local->f_mandala->allFacts){
    //fill params
    uint type=f->_vtype;
    uint varmsk=f->id();
    QString sn=f->name();
    uint ci=0;
    if(type==vt_vect || type==vt_point) ci=(varmsk>>8)+1;
    QColor c(Qt::cyan);
    if(sn.contains("ctr_"))c=Qt::magenta;
    else if(sn.contains("ctrb_"))c=Qt::magenta;
    else if(type==vt_flag)c=QColor(Qt::blue).lighter();
    else if(ci==1)c=Qt::red;
    else if(ci==2)c=Qt::green;
    else if(ci==3)c=Qt::yellow;
    //if(sv.size()>=2)c=QColor(sv.at(1));
    //if(sv.size()>=3)divider_dsp=sv.at(2).toDouble();
    Qt::PenStyle style = Qt::SolidLine;
    if(sn.contains("cmd_"))style=Qt::DotLine;
    else if(sn.contains("gps_"))style=Qt::DashLine;
    else if(sn.contains("rc_"))style=Qt::DotLine;
    registerField(f,sn,f->descr(),QPen(c, 0, style));
  }

  //QtScript calculated
  fcalculated=registerField(NULL,"calculated",tr("Calculated user variable"),QColor(Qt::yellow).lighter());

  //restore visible
  if(!QSettings().contains("plots"))QSettings().setValue("plots",QStringList()<<"pitch"<<"cmd_airspeed"<<"cmd_roll"<<"cmd_pitch"<<"altitude"<<"cmd_altitude"<<"roll"<<"airspeed");
  QStringList sps=QSettings().value("plots").toStringList();
  foreach (const QString &name,fields.uniqueKeys())
    showCurve(itemToInfo(fields.value(name)->curve), (name=="calculated") ? false : sps.contains(name));
    */
}
//=============================================================================
TelemetryPlot::~TelemetryPlot()
{
  //clear();
  delete zoomer;
  delete legend;
  delete grid;
  delete panner;
  delete panner2;
  delete mag;
  delete magX;
  delete magY;
  delete picker;
  delete timeCursor;
}
//=============================================================================
QwtPlotCurve * TelemetryPlot::addCurve(const QString &name, const QString &descr, const QString &units, const QPen &pen)
{
  PlotCurve c;
  c.name=name;
  c.descr=descr;
  c.units=units;
  c.curve=new QwtPlotCurve();
  c.curve->setVisible(false);
  c.curve->setTitle(name);
  c.curve->setPen(pen);
  c.curve->setYAxis(QwtPlot::yLeft);
  c.curve->setLegendAttribute(QwtPlotCurve::LegendShowLine);
  c.curve->setRenderHint(QwtPlotItem::RenderAntialiased);
  c.curve->attach(this);
  curves.append(c);
  //legend tooltip
  QString s="<html><NOBR>";
  s+="<div style='background-color: black;font-family: monospace; font-weight: bold;'><font size=+1>"+name+"</font></div>";
  if(!descr.isEmpty())s+="<div style='background-color: black;'>"+descr+"</div>";
  if(!units.isEmpty())s+="<div style='background-color: black;'>["+units+"]</div>";
  QWidget *w = legend->legendWidget(itemToInfo(c.curve));
  if(w) w->setToolTip(s);
  return c.curve;
}
//=============================================================================
void TelemetryPlot::restoreSettings()
{
  if(!QSettings().contains("plots"))QSettings().setValue("plots",QStringList()<<"pitch"<<"cmd_airspeed"<<"cmd_roll"<<"cmd_pitch"<<"altitude"<<"cmd_altitude"<<"roll"<<"airspeed");
  QStringList sps=QSettings().value("plots").toStringList();
  foreach (const PlotCurve &c, curves)
    showCurve(itemToInfo(c.curve), (c.name=="calculated") ? false : sps.contains(c.name));
}
//=============================================================================
void TelemetryPlot::saveSettings()
{
  QStringList sps;
  foreach(const PlotCurve &c, curves){
    if(c.curve->isVisible())
      sps.append(c.name);
  }
  QSettings().setValue("plots",sps);
}
//=============================================================================
void TelemetryPlot::showCurves(bool on, const QStringList &names, bool toggle)
{
  if(toggle&&on){
    bool bAllOn=true;
    foreach (const PlotCurve &c, curves){
      if((!names.size())||names.contains(c.name)){
        bAllOn&=c.curve->isVisible();
      }
    }
    if(bAllOn)on=false;
  }
  foreach (const PlotCurve &c, curves){
    if((!names.size())||names.contains(c.name)){
      showCurve(itemToInfo(c.curve),on);
    }
  }
  resetZoom();
}
//=============================================================================
void TelemetryPlot::pointSelected( const QPointF &pos )
{
  double x=pos.x();
  if(x<0)x=0;
  timeCursor->setXValue(x);
  replot();
  emit timeCursorChanged(x);
}
void TelemetryPlot::setTimeCursor(quint64 time_ms)
{
  timeCursor->setXValue(time_ms/1000.0);
  replot();
}
quint64 TelemetryPlot::timeCursorValue()
{
  return timeCursor->xValue()*1000.0;
}
//=============================================================================
void TelemetryPlot::showCurve(const QVariant &itemInfo, bool on, int index)
{
  Q_UNUSED(index);
  QwtPlotItem *item = infoToItem(itemInfo);
  if(item->isVisible()==on)return;
  //check if was empty plot
  bool allWereHidden=true;
  foreach(const PlotCurve &c, curves){
    if(c.curve->isVisible()){
      allWereHidden=false;
      break;
    }
  }
  //show or hide item
  item->setVisible(on);
  emit itemVisibleChanged(item);
  //update legend
  QwtLegendLabel *w = qobject_cast<QwtLegendLabel*>(legend->legendWidget(itemInfo));
  if(w)w->setChecked(on);
  //update zoom or replot
  if(allWereHidden) resetZoom();
  else replot();
}
//=============================================================================
/*void TelemetryPlot::refreshCalculated(void)
{
  bool ok;
  if(expCalc.isEmpty())expCalc="altitude+down";
  QString exp=QInputDialog::getText(NULL, tr("Calculated field"),
                                    tr("QtScript expression:"), QLineEdit::Normal,
                                    expCalc, &ok);
  if(!ok)return;
  expCalc=exp;

  //fill internal data
  fcalculated->points.clear();
  QJSEngine engine;

  QVector<_telemetry_field*> flist;
  QVector<QString> fnames;
  QVector<double> fvalues;
  QVector<int> fpidx;
  foreach (_telemetry_field *f, fields) {
    flist.append(f);
    fnames.append(f->curve->title().text());
    fvalues.append(f->points.isEmpty()?0:f->points.first().y());
    fpidx.append(0);
    engine.globalObject().setProperty(fnames.last(),fvalues.last());
  }
  double vcalc=0;
  for(int ti=0;ti<times.size();++ti){
    setProgress(ti*100/times.size());
    double t=times.at(ti);
    engine.globalObject().setProperty("time",t);
    //collect values
    for(int i=0;i<flist.size();++i){
      _telemetry_field *f=flist.at(i);
      int pidx=fpidx.at(i);
      if(f->points.size()<=pidx)continue;
      bool bUpd=false;
      while(t>f->points.at(pidx).x()){
        pidx++;
        if(f->points.size()<=pidx)break;
        if(t<f->points.at(pidx).x())break;
        fvalues[i]=f->points.at(pidx).y();
        fpidx[i]=pidx;
        bUpd=true;
      }
      if(bUpd){
        engine.globalObject().setProperty(fnames.at(i),fvalues.at(i));
      }
    }
    double v=engine.evaluate(expCalc).toNumber();
    if(v==vcalc)continue;
    vcalc=v;
    fcalculated->points.append(QPointF(t,v));
  }
  setProgress(0);
  //install data
  fcalculated->curve->setData(new QwtPointSeriesData(fcalculated->points));
  fcalculated->curve->setVisible(true);
  resetZoom();
}*/
//=============================================================================
void TelemetryPlot::resetZoom()
{
  double vmax=0.0,vmin=0.0,tMax=0.0;
  foreach (const PlotCurve &c, curves)
    for(size_t i=0;i<c.curve->data()->size();++i){
      const QPointF &p=c.curve->data()->sample(i);
      if(tMax<p.x())tMax=p.x();
      double v=p.y();
      if(c.curve->isVisible()&&(!isnan(v))&&(!isinf(v))){
        if (vmax<v)vmax=v;
        if (vmin>v)vmin=v;
      }
    }
  //reset zoom
  QRectF r(0,vmin,tMax,vmax-vmin);
  zoomer->zoom(r);
  zoomer->setZoomBase(r);
  //zoomer->zoom(r);
  //zoomer->setZoomBase(r);
  //QCoreApplication::processEvents();
  //replot();
}
//=============================================================================
/*void TelemetryPlot::copyFromPlot(TelemetryPlot *plot)
{
  isCopy=true;
  //install data
  foreach(const QString &name,fields.uniqueKeys()){
    fields.value(name)->curve->setData(new QwtPointSeriesData(((QwtPointSeriesData*)plot->fields.value(name)->curve->data())->samples()));
    showCurve(itemToInfo(fields.value(name)->curve),plot->fields.value(name)->curve->isVisible());
  }
  //same zoom
  setAxisScaleDiv(QwtPlot::yLeft, plot->axisScaleDiv(QwtPlot::yLeft));
  setAxisScaleDiv(QwtPlot::xBottom, plot->axisScaleDiv(QwtPlot::xBottom));
  zoomer->setZoomBase(plot->zoomer->zoomBase());

  replot();
}*/
//=============================================================================
//=============================================================================
QwtText PlotPicker::trackerText(const QPoint &pos)const
{
  double t=plot()->invTransform(QwtPlot::xBottom,pos.x());
  QString s="<html><NOBR><table>";
  s+="<tr><td colspan=2 align=left style='font-family: monospace; font-weight: bold;'><PRE><font size=+4>";
  s+=QTime(0,0).addSecs(t).toString("hh:mm:ss");//+QString::number(t)+" sec)";
  s+="</font></PRE></td></tr>";
  const QwtPlotItemList &items=plot()->itemList(QwtPlotItem::Rtti_PlotCurve);
  for(int i=0;i<items.size();++i){
    QwtPlotCurve *curve=static_cast<QwtPlotCurve*>(items.at(i));
    if(!curve->isVisible())continue;
    double v=0;
    for(size_t i2=0;i2<curve->data()->size();++i2){
      if(curve->data()->sample(i2).x()>=t){
        v=curve->data()->sample(i2).y();
        break;
      }
    }
    QColor c=curve->pen().color();
    if(curve->pen().style()!=Qt::SolidLine)
      c=c.darker();
    //QString units;
    //Fact *fact=Vehicles::instance()->f_local->f_mandala->factByName(name);
    //if(fact)units=fact->units();
    s+="<tr><td align=right><font size=+2 color="+c.name()+">"+curve->title().text()+"</font>&nbsp;&nbsp;</td><td align=left><font size=+2>"+QString().sprintf("%.2f",v)+"</font></td></tr>";
  }
  s+="</table>";
  return QwtText(s);
}
//=============================================================================
void LegendItem::paintEvent(QPaintEvent *e)
{
  const QRect cr=contentsRect();
  QPainter painter( this );
  painter.setClipRegion(e->region());
  if (isChecked()){
    painter.save();
    painter.setBrush(QColor(50,50,50));
    painter.setPen(QPen(QColor(100,100,100),1,Qt::DotLine));
    painter.drawRoundedRect(cr.adjusted(0,0,-1,-1),2,2);
    painter.restore();
  }
  painter.save();
  painter.setClipRect( cr );
  drawContents(&painter);
  QRect identRect = cr;
  identRect.setX( identRect.x() + margin() );
  identRect.setSize( icon().size() );
  identRect.moveCenter( QPoint( identRect.center().x(), cr.center().y() ) );
  painter.drawPixmap( identRect, icon() );
  painter.restore();
}
//=============================================================================
void PlotMagnifier::widgetMouseMoveEvent(QMouseEvent *mouseEvent)
{
  mwPos=mouseEvent->pos();
  QwtPlotMagnifier::widgetMouseMoveEvent(mouseEvent);
}
void PlotMagnifier::rescale( double factor )
{
  factor = qAbs( factor );
  if ( factor == 1.0 || factor == 0.0 )
    return;
  bool doReplot = false;
  QwtPlot* plt = plot();
  const bool autoReplot = plt->autoReplot();
  plt->setAutoReplot( false );
  for ( int axisId = 0; axisId < QwtPlot::axisCnt; axisId++ ){
    const QwtScaleDiv &scaleDiv = plt->axisScaleDiv( axisId );
    if ( isAxisEnabled( axisId )){
      // get the range of the axis
      double min_value = scaleDiv.lowerBound();
      double max_value = scaleDiv.upperBound();
      // convert to screen coordinates
      QwtScaleMap mapper = plt->canvasMap(axisId);
      double min_pixel = mapper.transform(min_value);
      double max_pixel = mapper.transform(max_value);
      // get the mouse position on this axis
      double center;
      if ( axisId == QwtPlot::yLeft || axisId == QwtPlot::yRight )
        center = mwPos.y();
      else
        center = mwPos.x();
      // use the mouse as the center when we can,
      // if its -1, then its a keyboard event. use the window center.
      if (center == -1)
        center = min_pixel + (max_pixel-min_pixel)/2;
      // convert back to real values
      min_value = mapper.invTransform( center - (center - min_pixel)*factor );
      max_value = mapper.invTransform( center + (max_pixel - center)*factor );
      // set the new range
      plt->setAxisScale(axisId, min_value, max_value);
      //plt->setAxisScale( axisId, center - width_2, center + width_2 );
      doReplot = true;
    }
  }
  plt->setAutoReplot( autoReplot );
  if ( doReplot ) plt->replot();
}
//=============================================================================
PlotLegend::PlotLegend(QWidget *parent) :
  QwtLegend(parent)
{
}
//=============================================================================
QWidget *PlotLegend::createWidget(const QwtLegendData &data) const
{
  Q_UNUSED(data);
  QwtLegendLabel *w = new LegendItem();
  w->setItemMode(defaultItemMode());
  w->setSpacing(3);
  w->setMargin(0);
  connect(w, SIGNAL(clicked()), this, SLOT(itemClicked()));
  connect(w, SIGNAL(checked(bool)), this, SLOT(itemChecked(bool)));
  return w;
}
//=============================================================================
