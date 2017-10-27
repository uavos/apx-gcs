#include "NumbersPlugin.h"
#include <QMandala.h>
#include <QmlView.h>
#include <QAction>
#include <QInputDialog>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include "NumbersForm.h"
#include "AppDirs.h"
#define NUMBERS_CONF_NAME "Numbers"
//=============================================================================
void NumbersPlugin::init(void)
{
  item=NULL;
  srcBase=QStringLiteral("qrc:///");
  view=new QmlView(srcBase);
  obj=view->createWidget(tr("Numbers"));
  view->menu->addAction(QIcon(":/icons/old/configure.png"),tr("Configure")+"...",this,SLOT(configure()));
  view->menu->addAction(QIcon(":/icons/old/document-save.png"),tr("Export as plugin")+"...",this,SLOT(saveAs()));
  view->menu->addAction(QIcon(":/icons/old/document-open.png"),tr("Load exported plugin")+"...",this,SLOT(open()));
  getConfig(); //defaults
  connect(view,SIGNAL(sceneGraphInitialized()),this,SLOT(make()));
}
//=============================================================================
void NumbersPlugin::configure()
{
  QStringList sld=getConfig();
  for(int i=0;i<sld.size();i++)
    sld[i]=sld.at(i).trimmed();
  NumbersForm dlg;
  if(dlg.exec()!=QDialog::Accepted)return;
  QSettings().setValue(NUMBERS_CONF_NAME,dlg.itemsList());
  getConfig();
  make();
}
//=============================================================================
//=============================================================================
void NumbersPlugin::make()
{
  //qDebug("MAKE");
  //disconnect(view,SIGNAL(activeChanged()),this,SLOT(make()));
  QStringList slist=QSettings().value(NUMBERS_CONF_NAME).toStringList();
  QStringList s;
  QString si;
  QString sia(2,' ');
  s<<"import QtQuick 2.2";
  s<<"import \"qrc:///components\"";
  s<<"Item {";
  si+=sia;
  s<<si+"Column {";
  si+=sia;
  s<<si+"anchors.fill: parent";
  s<<si+"spacing: -3";
  s<<si+"property double txtHeight: app.limit(-spacing+height/"+QString::number(slist.size())+",10,width/4)";
  foreach(QString sx,slist){
    QStringList st=sx.trimmed().split(':');
    if(st.isEmpty())continue;
    QString smvar=st.at(0).trimmed();
    if(smvar.isEmpty())continue;
    s<<si+"Number {";
    si+=sia;
    s<<si+"height: parent.txtHeight";
    if(smvar.contains(".value")){
      s<<si+"value: "+smvar;
    }else{
      s<<si+"mfield: m."+smvar;
    }
    QString slabel=st.size()>1?st.at(1).trimmed():"";
    if(!slabel.isEmpty()){
      s<<si+"label: \""+slabel+"\"";
    }
    QString sprecision=st.size()>2?st.at(2).trimmed():"";
    if(sprecision.isEmpty()){
      s<<si+"precision: 1";
    }else{
      sprecision.remove('.');
      s<<si+"precision: "+sprecision;
    }
    QString swarn=st.size()>3?st.at(3).trimmed():"";
    if(!swarn.isEmpty()){
      s<<si+"warning: "+swarn;
    }
    QString salarm=st.size()>4?st.at(4).trimmed():"";
    if(!salarm.isEmpty()){
      s<<si+"alarm: "+salarm;
    }
    si.chop(sia.size());
    s<<si+"}";
  }
  si.chop(sia.size());
  s<<si+"}"; //Column
  //s<<"  ActionsMenu { id: menu }";
  //s<<"  MouseArea {anchors.fill: parent; onClicked: menu.popup(mouseX,mouseY); }";
  si.chop(sia.size());
  s<<si+"}"; //Item
  s<<"";
  content=s.join('\n');
  if(item){
    item->deleteLater();
  }
  QQmlComponent component(view->engine());
  component.setData(content.toUtf8(),srcBase);
  item = qobject_cast<QQuickItem *>(component.create());
  if(!item)return;
  QQmlEngine::setObjectOwnership(item,QQmlEngine::CppOwnership);
  view->setContent(QUrl(),0,item);
  item->setParent(this);
  //item->setVisible(true);
  //item->setParentItem(view->rootObject());
}
//=============================================================================
QStringList NumbersPlugin::getConfig()
{
  QStringList sld=QSettings().value(NUMBERS_CONF_NAME).toStringList();
  if(sld.isEmpty()||sld.join("").trimmed().isEmpty()){
    //make default list
    sld.clear();
    sld<<"user1::4";
    sld<<"user2::4";
    sld<<"user3::4";
    sld<<"user4::4";
    sld<<"agl:::value>5";
    sld<<"cam_pitch:CP:4";
    sld<<"cam_yaw:CY:4";
    sld<<"cas2tas:::value>1.8 || (value>0 && value<1):value>=2";
    sld<<"airspeed.value*cas2tas.value-gSpeed.value:dTAS";
    sld<<"Math.atan(pitch.value/roll.value):calc:2";
    sld<<"windHdg::0";
    QSettings().setValue(NUMBERS_CONF_NAME,sld);
  }
  return sld;
}
//=============================================================================
void NumbersPlugin::saveAs()
{
  QDir userp=AppDirs::userPlugins();
  if(!userp.exists())userp.mkpath(".");
  QFileDialog dlg(NULL,tr("User plugin name"),userp.canonicalPath());
  dlg.setAcceptMode(QFileDialog::AcceptSave);
  dlg.setOption(QFileDialog::DontConfirmOverwrite,false);
  dlg.selectFile("user.qml");
  QStringList filters;
  filters << tr("QML files")+" (*.qml)"
          << tr("Any files")+" (*)";
  dlg.setNameFilters(filters);
  dlg.setDefaultSuffix("qml");
  if(!dlg.exec() || dlg.selectedFiles().size()!=1)return;
  if(content.isEmpty())return;
  QString fileName=dlg.selectedFiles().at(0);
  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(NULL, QApplication::applicationName(),QString(tr("Cannot write file")+" %1:\n%2.").arg(fileName).arg(file.errorString()));
    return;
  }
  QTextStream out(&file);
  out<<"//NumbersPlugin: "+QSettings().value(NUMBERS_CONF_NAME).toStringList().join(',')+"\n";
  out << content;
  file.close();
  qDebug("%s",tr("Plugin '%1' exported").arg(QFileInfo(file).baseName()).toUtf8().data());
}
//=============================================================================
void NumbersPlugin::open()
{
  QDir userp=AppDirs::userPlugins();
  if(!userp.exists())userp.mkpath(".");
  QFileDialog dlg(NULL,tr("User plugin name"),userp.canonicalPath());
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  QStringList filters;
  filters << tr("QML files")+" (*.qml)"
          << tr("Any files")+" (*)";
  dlg.setNameFilters(filters);
  dlg.setDefaultSuffix("qml");
  if(!dlg.exec() || dlg.selectedFiles().size()!=1)return;
  if(content.isEmpty())return;
  QString fileName=dlg.selectedFiles().at(0);
  QFile file(fileName);
  if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::warning(NULL, QApplication::applicationName(),QString(tr("Cannot open file")+" %1:\n%2.").arg(fileName).arg(file.errorString()));
    return;
  }
  QTextStream stream(&file);
  bool bFound=false;
  while(!stream.atEnd()){
    QString s=stream.readLine().trimmed();
    if(s.isEmpty())continue;
    if(!s.startsWith("//NumbersPlugin: "))continue;
    s=s.mid(s.indexOf(':')).trimmed();
    bFound=true;
    QSettings().setValue(NUMBERS_CONF_NAME,s.split(','));
    getConfig();
    make();
    break;
  }
  file.close();
  if(bFound)qDebug("%s",tr("Content of '%1' loaded").arg(QFileInfo(file).baseName()).toUtf8().data());
  else qWarning("%s",tr("Content in '%1' not found").arg(QFileInfo(file).baseName()).toUtf8().data());
}
//=============================================================================
