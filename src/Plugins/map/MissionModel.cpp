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
#include "MissionModel.h"
#include "QMandala.h"
#include "Mission.h"
#include "MapView.h"
#include "MissionItemCategory.h"
#include "MissionItemWp.h"
#include "MissionItemRw.h"
#include "ValueEditorArray.h"
#include "FactSystem.h"
//=============================================================================
MissionModel::MissionModel(MapView *mapView)
  :QAbstractItemModel(mapView),mapView(mapView)
{
  delegate=new MissionDelegate(this);
  rootItem = new MissionItem(NULL,"mission");

  selectionModel=new QItemSelectionModel(this);

  runways=new MissionItemCategory<MissionItemRw>(this,"runways",tr("Runways"),"runway",Mission::mi_rw);
  waypoints=new MissionItemCategory<MissionItemWp>(this,"waypoints",tr("Waypoints"),"waypoint",Mission::mi_wp);
  taxiways=new MissionItemCategory<MissionItemTw>(this,"taxiways",tr("Taxiways"),"taxiway",Mission::mi_tw);
  points=new MissionItemCategory<MissionItemPi>(this,"points",tr("Points"),"point",Mission::mi_pi);
  restricted=new MissionItemCategory<MissionItemArea>(this,"restricted",tr("Restricted"),"area",Mission::mi_restricted);
  emergency=new MissionItemCategory<MissionItemArea>(this,"emergency",tr("Emergency"),"area",Mission::mi_emergency);

  connect(this,SIGNAL(addedRemoved()),this,SIGNAL(changed()));
  connect(this,SIGNAL(changed()),this,SLOT(emit_layoutChanged()),Qt::QueuedConnection);
  connect(&updTimer,SIGNAL(timeout()),this,SIGNAL(layoutChanged()));
  updTimer.setSingleShot(true);

  connect(this,SIGNAL(changed()),mapView,SIGNAL(updateStats()),Qt::QueuedConnection);

  connect(this,SIGNAL(addedRemoved()),rootItem,SLOT(invalidate()));

  connect(QMandala::instance(),SIGNAL(currentChanged(QMandalaItem*)),this,SLOT(mandalaCurrentChanged(QMandalaItem*)));
  mandalaCurrentChanged(QMandala::instance()->current);
}
MissionModel::~MissionModel()
{
  clear();
  delete rootItem;
  selectionModel->deleteLater();
  delegate->deleteLater();
}
//=============================================================================
void MissionModel::mandalaCurrentChanged(QMandalaItem *m)
{
  //QScriptValue mobj=m->engine.newQObject(this,QScriptEngine::QtOwnership,QScriptEngine::ExcludeSuperClassMethods);
  //m->engine.globalObject().setProperty(objectName(),mobj);

  foreach(QMetaObject::Connection c,mcon) disconnect(c);
  mcon.clear();
  mcon.append(connect(m,SIGNAL(data(uint8_t,QByteArray)),this,SLOT(mandala_data(uint8_t,QByteArray))));
}
//=============================================================================
void MissionModel::clear(void)
{
  beginResetModel();
  foreach(MissionItem *i,rootItem->childItems)
    i->clear();
  endResetModel();
  emit addedRemoved();
}
//=============================================================================
QList<MissionItem*> MissionModel::selectedItems(void) const
{
  QList<MissionItem *> list;
  foreach(QModelIndex index,selectionModel->selectedIndexes())
    if(index.internalPointer()&&index.column()==0){
      MissionItem *i=static_cast<MissionItem*>(index.internalPointer());
      if(!i->childCount())continue;//skip fields i=i->parent();
      if(!list.contains(i))list.append(i);
    }
  return list;
}
//=============================================================================
void MissionModel::remove()
{
  beginResetModel();
  foreach(MissionItem *i,selectedItems()){
    i->remove();
  }
  endResetModel();
  emit addedRemoved();
}
//=============================================================================
void MissionModel::upload(void)
{
  saveToTelemetry();
  QMandala::instance()->current->send(idx_mission,pack());
}
void MissionModel::request(void)
{
  QMandala::instance()->current->request(idx_mission);
}
//=============================================================================
void MissionModel::newFile()
{
  clear();
  missionFileName="";
  missionName="";
  emit missionNameChanged(missionName);
}
//=============================================================================
void MissionModel::saveToFile(QString fname)
{
  QFile file(fname);
  if(!file.open(QFile::WriteOnly | QFile::Text)) {
    qWarning("%s",QString(tr("Cannot write file")+" %1:\n%2.").arg(fname).arg(file.errorString()).toUtf8().data());
    return;
  }
  QDomDocument doc;
  doc.appendChild(doc.createProcessingInstruction("xml","version=\"1.0\" encoding=\"UTF-8\""));
  saveToXml(doc);
  rootItem->backup();
  QTextStream stream(&file);
  doc.save(stream,2);
  file.close();
  missionFileName=fname;
  missionName=QFileInfo(missionFileName).baseName();
  emit missionNameChanged(missionName);
}
//=============================================================================
void MissionModel::loadFromFile(const QString &fname)
{
  QFile file(fname);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    qWarning("%s",QString(tr("Cannot read file")+" %1:\n%2.").arg(fname).arg(file.errorString()).toUtf8().data());
    return;
  }
  QDomDocument doc;
  if(!(doc.setContent(&file) && doc.documentElement().nodeName()==rootItem->objectName())){
    qWarning("%s",QString(tr("The file format is not correct.")).toUtf8().data());
    file.close();
    return;
  }
  file.close();
  clear();
  loadFromXml(doc.documentElement());
  emit addedRemoved();
  rootItem->backup();
  missionFileName=fname;
  missionName=QFileInfo(missionFileName).baseName();
  emit missionNameChanged(missionName);
  mapView->goHome();
  saveToTelemetry();
}
//=============================================================================
void MissionModel::loadFromTelemetry(void)
{
  QDomDocument doc;
  if(!QMandala::instance()->local->rec->file.xmlParts.value(rootItem->objectName()).values().isEmpty())
    doc.setContent(QMandala::instance()->local->rec->file.xmlParts.value(rootItem->objectName()).values().first());
  if(doc.documentElement().nodeName()!=rootItem->objectName())return;
  clear();
  loadFromXml(doc.documentElement());
  emit addedRemoved();
  missionFileName="";
  missionName=tr("From Telemetry");
  emit missionNameChanged(missionName);
}
void MissionModel::saveToTelemetry(void)
{  
  QByteArray hash=rootItem->md5();
  if(telemetry_hash==hash)return;
  telemetry_hash=hash;

  QDomDocument doc("");
  saveToXml(doc);
  QBuffer b;
  b.open(QBuffer::WriteOnly);
  QTextStream stream(&b);
  stream<<"\n";
  doc.save(stream,2);
  QByteArray ba(b.data());
  if(ba.endsWith('\n'))ba.resize(ba.size()-1);
  QMandala::instance()->current->rec->saveXmlPart(rootItem->objectName(),ba);
}
void MissionModel::mandala_data(uint8_t id,QByteArray data)
{
  if(id==idx_mission){
    qDebug("%s",tr("Mission received from UAV.").toUtf8().data());
    unpack(data);
  }
}
//=============================================================================
void MissionModel::saveToXml(QDomNode dom) const
{
  QDomDocument doc=dom.isDocument()?dom.toDocument():dom.ownerDocument();
  dom=doc.appendChild(doc.createElement(rootItem->objectName()));
  dom.toElement().setAttribute("href","http://www.uavos.com/");
  dom.toElement().setAttribute("title",missionName);
  dom.toElement().setAttribute("version",FactSystem::version());
  dom.appendChild(doc.createElement("hash")).appendChild(doc.createTextNode(rootItem->md5().toHex()));
  dom.appendChild(doc.createElement("timestamp")).appendChild(doc.createTextNode(QDateTime::currentDateTimeUtc().toString()));
  QDomNode e=dom.appendChild(doc.createElement("home"));
  e.appendChild(doc.createElement("lat")).appendChild(doc.createTextNode(QString::number(QMandala::instance()->current->home_pos[0],'f',8)));
  e.appendChild(doc.createElement("lon")).appendChild(doc.createTextNode(QString::number(QMandala::instance()->current->home_pos[1],'f',8)));
  e.appendChild(doc.createElement("hmsl")).appendChild(doc.createTextNode(QString::number((int)QMandala::instance()->current->home_pos[2])));
  rootItem->saveToXml(dom);
}
//=============================================================================
void MissionModel::loadFromXml(QDomNode dom)
{
  beginResetModel();
  QDomDocument doc=dom.isDocument()?dom.toDocument():dom.ownerDocument();
  //read home pos
  QDomElement e=dom.firstChildElement("home");
  if(!e.isNull()){
    QMandala::instance()->current->field("home_lat")->setValueLocal(e.firstChildElement("lat").text().toDouble());
    QMandala::instance()->current->field("home_lon")->setValueLocal(e.firstChildElement("lon").text().toDouble());
    QMandala::instance()->current->field("home_hmsl")->setValueLocal(e.firstChildElement("hmsl").text().toDouble());
  }
  rootItem->loadFromXml(dom);
  endResetModel();
}
//=============================================================================
bool MissionModel::isModified(void) const
{
  return rootItem->isModified();
}
//=============================================================================
bool MissionModel::isEmpty(void) const
{
  return !(rootItem->childCount());
}
//=============================================================================
QVariant MissionModel::data(const QModelIndex &index, int role) const
{
  if(!index.isValid()) return QVariant();
  return item(index)->data(index.column(),role);
}
//=============================================================================
bool MissionModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
  if ((!index.isValid()) || (role!=Qt::EditRole) || (!index.internalPointer()) || index.column()!=MissionItem::tc_value)
    return false;
  MissionItem *item = static_cast<MissionItem*>(index.internalPointer());
  if (data(index,role)==value)return true;
  bool rv=item->setData(index.column(),value);
  if(rv)emit layoutChanged();
  return rv;
}
//=============================================================================
QModelIndex MissionModel::index(int row, int column, const QModelIndex &parent) const
{
  if(!hasIndex(row, column, parent)) return QModelIndex();
  MissionItem *parentItem;
  if(!parent.isValid()) parentItem = rootItem;
  else parentItem = item(parent);
  MissionItem *childItem = parentItem->child(row);
  if(childItem) return createIndex(row, column, childItem);
  else return QModelIndex();
}
//=============================================================================
QModelIndex MissionModel::parent(const QModelIndex &index) const
{
  if(!index.isValid()) return QModelIndex();
  MissionItem *parentItem = item(index)->parent();
  if (parentItem==NULL || parentItem == rootItem)
    return QModelIndex();
  return createIndex(parentItem->row(), 0, parentItem);
}
//=============================================================================
int MissionModel::rowCount(const QModelIndex &parent) const
{
  MissionItem *parentItem;
  if (parent.column() > 0) return 0;
  if (!parent.isValid()) parentItem = rootItem;
  else parentItem = item(parent);
  return parentItem->childCount();
}
//=============================================================================
int MissionModel::columnCount(const QModelIndex &parent) const
{
  if (parent.isValid()) return item(parent)->columnCount();
  else return rootItem->columnCount();
}
//=============================================================================
QVariant MissionModel::headerData(int section, Qt::Orientation orientation,int role) const
{
  if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return rootItem->data(section);
  return QVariant();
}
//=============================================================================
Qt::ItemFlags MissionModel::flags(const QModelIndex & index) const
{
  if (!index.isValid()) return 0;
  MissionItem *i=item(index);
  Qt::ItemFlags f=i->flags(index.column());
  //if(i==rootWp || i==rootRw)f|=Qt::ItemIsDropEnabled;
  return f;
}
//=============================================================================
QModelIndexList MissionModel::getPersistentIndexList()
{
  return persistentIndexList();
}
MissionItem *MissionModel::item(const QModelIndex &index) const
{
  return static_cast<MissionItem*>(index.internalPointer());
}
//=============================================================================
QModelIndex MissionModel::findIndex(MissionItem *item) const
{
  if(item->parentItem!=rootItem)
    return index(item->row(),0,findIndex(item->parentItem));
  return index(item->row(),0);
}
//=============================================================================
Qt::DropActions MissionModel::supportedDropActions() const
{
  return Qt::MoveAction;
}
bool MissionModel::dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent)
{
  Q_UNUSED(data)
  Q_UNUSED(column)
  if(action!=Qt::MoveAction)return false;
  MissionItem *dItem=item(parent);
  if(dItem==NULL || dItem==rootItem || dItem->childCount()==0)return false;
  //collect selected childs of drop item
  QMap<int,MissionItem *> sel;
  foreach(QModelIndex index,selectionModel->selectedIndexes()){
    if(index.internalPointer()&&index.column()==0){
      MissionItem *i=static_cast<MissionItem*>(index.internalPointer());
      if(i->parent()==dItem) sel.insert(i->row(),i);
    }
  }
  if(!sel.size())return false;
  //check selection integrity
  if((sel.keys().last()-sel.keys().first()+1)!=sel.size())return false;
  if(row>=sel.keys().first() && row<=sel.keys().last())return false;
  //move list items within parent dItem
  beginResetModel();
  if(row>=dItem->childItems.size()){
    foreach(MissionItem *i,sel.values()){
      dItem->childItems.removeOne(i);
      dItem->childItems.append(i);
    }
  }else{
    MissionItem *iN=dItem->childItems.at(row);
    foreach(MissionItem *i,sel.values()){
      dItem->childItems.removeOne(i);
      dItem->childItems.insert(dItem->childItems.indexOf(iN),i);
    }
  }
  endResetModel();
  emit addedRemoved();
  return true;
}
//=============================================================================
QWidget *MissionDelegate::createEditor(QWidget *parent,const QStyleOptionViewItem &option,const QModelIndex &index) const
{
  Q_UNUSED(option);
  if(!(index.internalPointer()))//&&(index.column()==MissionItem::tc_value)))
    return QItemDelegate::createEditor(parent,option,index);
  QWidget *e=NULL;
  if((static_cast<MissionItem*>(index.internalPointer()))->inherits("MissionItemField")){
    MissionItemField *item = static_cast<MissionItemField*>(index.internalPointer());
    switch(item->ftype){
      case MissionItemField::dt_option:
      case MissionItemField::dt_varmsk:
        e=createEditorEx(parent,option,index,item->ftype,item->opts);
        break;
      case MissionItemField::dt_lat:
        e=createEditorEx(parent,option,index,item->ftype,item->opts);
        break;
      case MissionItemField::dt_distance:{
        QSpinBox *sb=new QSpinBox(parent);
        sb->setRange(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
        sb->setSuffix(" m");
        sb->setSingleStep(10);
        e=sb;
      }break;
      case MissionItemField::dt_angle:{
        QSpinBox *sb=new QSpinBox(parent);
        sb->setRange(-359,359);
        sb->setSuffix(" deg");
        sb->setSingleStep(1);
        e=sb;
      }break;
      case MissionItemField::dt_time:{
        QTimeEdit *te=new QTimeEdit(parent);
        te->setDisplayFormat("HH:mm:ss");
        e=te;
      }break;
    }
  }else if((static_cast<MissionItem*>(index.internalPointer()))->inherits("MissionItemCategoryBase")){
    e=new ValueEditorArray(static_cast<MissionItemCategoryBase*>(index.internalPointer()),parent);
  }
  if(!e){
    e=QItemDelegate::createEditor(parent,option,index);
    e->setFont(index.data(Qt::FontRole).value<QFont>());
    e->setAutoFillBackground(true);
  }
  return e;
}
QWidget *MissionDelegate::createEditorEx(QWidget *parent, const QStyleOptionViewItem &option,const QModelIndex &index,uint ftype,const QStringList opts) const
{
  QWidget *e=NULL;
  switch(ftype){
    case MissionItemField::dt_option:{
      QComboBox *cb=new QComboBox(parent);
      cb->setFrame(false);
      cb->addItems(opts);
      cb->view()->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Ignored);
      //cb->view()->setMaximumWidth(cb->view()->sizeHintForColumn(0));
      e=cb;
    }break;
    case MissionItemField::dt_varmsk:{
      QComboBox *cb=new QComboBox(parent);
      cb->setFrame(false);
      cb->addItem("");
      cb->addItems(QMandala::instance()->current->names);
      cb->setEditable(true);
      cb->view()->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Ignored);
      //cb->view()->setMaximumWidth(cb->view()->sizeHintForColumn(0)*2);
      e=cb;
    }break;
  }
  if(!e) e=QItemDelegate::createEditor(parent,option,index);
  e->setFont(index.data(Qt::FontRole).value<QFont>());
  e->setAutoFillBackground(true);
  return e;
}
void MissionDelegate::setEditorData(QWidget *editor,const QModelIndex &index) const
{
  if(editor->inherits("QComboBox")){
    QComboBox *cb=static_cast<QComboBox*>(editor);
    cb->setCurrentIndex(cb->findText(index.model()->data(index, Qt::EditRole).toString()));
    return;
  }
  QItemDelegate::setEditorData(editor,index);
}
void MissionDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const
{
 if(editor->inherits("QComboBox")) {
   QComboBox *comboBox = static_cast<QComboBox*>(editor);
   //comboBox->interpretText();//is this important for the QComboBox delegate??
   QString str  = comboBox->currentText();
   model->setData(index, str, Qt::EditRole);
   return;
 }
 if(editor->inherits("QTimeEdit")){
   QTimeEdit *te=static_cast<QTimeEdit*>(editor);
   model->setData(index, -te->time().secsTo(QTime(0,0)), Qt::EditRole);
   return;
 }
 QItemDelegate::setModelData(editor,model,index);
}
//=============================================================================
QByteArray MissionModel::pack() const
{
  QByteArray ba=rootItem->pack();
  Mission::_item_hdr v;
  v.type=Mission::mi_stop;
  v.option=0;
  ba.append((const char*)&v,sizeof(v));
  return ba;
}
//=============================================================================
void MissionModel::unpack(const QByteArray &ba)
{
  if(!ba.size())return;
  beginResetModel();
  clear();
  int cnt=rootItem->unpack(ba);
  endResetModel();
  if(cnt!=ba.size()){
    qWarning("%s (%i/%i)",tr("Error extracting mission").toUtf8().data(),cnt,ba.size());
    clear();
    return;
  }
  missionFileName="";
  missionName=QString("%1 %2").arg(rootItem->objectName()).arg(QMandala::instance()->uavName());
  emit missionNameChanged(missionName);
  saveToTelemetry();
}
//=============================================================================
