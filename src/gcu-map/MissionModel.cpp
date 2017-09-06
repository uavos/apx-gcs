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
#include "MissionItemCategory.h"
#include "MissionItemWp.h"
#include "MissionItemRw.h"
#include "QMandala.h"
#include "Mission.h"
//=============================================================================
MissionModel::MissionModel(QObject *parent)
 : QAbstractItemModel(parent)
{
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

  connect(this,SIGNAL(addedRemoved()),rootItem,SLOT(invalidate()));

  connect(this,SIGNAL(addedRemoved()),this,SIGNAL(nameChanged()));

  connect(QMandala::instance(),SIGNAL(currentChanged(QMandalaItem*)),this,SLOT(mandalaCurrentChanged(QMandalaItem*)));
  mandalaCurrentChanged(QMandala::instance()->current);
}
MissionModel::~MissionModel()
{
  clear();
  delete rootItem;
  selectionModel->deleteLater();
}
//=============================================================================
void MissionModel::mandalaCurrentChanged(QMandalaItem *m)
{
  QScriptValue mobj=m->engine.newQObject(this,QScriptEngine::QtOwnership,QScriptEngine::ExcludeSuperClassMethods);
  m->engine.globalObject().setProperty(objectName(),mobj);

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
  m_startPoint=QPointF();
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
  qDebug()<<"MissionModel::request";
  QMandala::instance()->current->request(idx_mission);
}
//=============================================================================
void MissionModel::newFile()
{
  clear();
  missionFileName="";
  setName("");
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
  setName(QFileInfo(missionFileName).baseName());
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
  setName(QFileInfo(missionFileName).baseName());
  //mapView->goHome();
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
  setName(tr("telemetry mission"));
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
  dom.toElement().setAttribute("title",name());
  dom.toElement().setAttribute("version",QMandala::version);
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
  foreach(MissionItem *i,rootItem->childItems) {
    if(i->childCount())return false;
  }
  return true;
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
  emit addedRemoved();
  if(cnt!=ba.size()){
    qWarning("Error extracting mission (%i/%i)",cnt,ba.size());
    clear();
    return;
  }
  missionFileName="";
  setName(QString("%1 %2").arg(rootItem->objectName()).arg(QMandala::instance()->uavName()));
  saveToTelemetry();
}
//=============================================================================
//=============================================================================
MissionItem * MissionModel::root()
{
  return rootItem;
}
QPointF MissionModel::startPoint()
{
  return m_startPoint;
}
double MissionModel::startCourse()
{
  return m_startPoint.isNull()?0:m_startCourse;
}
double MissionModel::startLength()
{
  return m_startPoint.isNull()?0:m_startLength;
}
void MissionModel::setStartPointVector(const QPointF &ll, double crs, double len)
{
  if(m_startPoint==ll && m_startCourse==crs && m_startLength==len)return;
  m_startPoint=ll;
  m_startCourse=crs;
  m_startLength=len;
  emit startPointChanged();
}
//=============================================================================
QString MissionModel::name() const
{
  return isEmpty()?tr("no mission"):m_name.isEmpty()?"mission":m_name;
}
void MissionModel::setName(QString v)
{
  if(m_name==v)return;
  m_name=v;
  emit nameChanged();
}
//=============================================================================
