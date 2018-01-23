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
#include "Fact.h"
#include "FactSystem.h"
//=============================================================================
Fact::Fact(FactTree *parent, const QString &name, const QString &title, const QString &descr, ItemType treeItemType, DataType dataType)
 : FactData(parent,name,title,descr,treeItemType,dataType),
   m_enabled(true), m_visible(true), m_active(false), m_progress(0), m_busy(false)
{
  m_model=new FactListModel(this);
  if((treeItemType==GroupItem || treeItemType==SectionItem) && m_dataType==ConstData){
    connect(this,&Fact::sizeChanged,this,&Fact::statusChanged);
  }

  if(parent){
    parent->addItem(this);
    if(m_name.contains('#')){
      connect(parent,&FactData::sizeChanged,this,&FactData::nameChanged);
    }
    //connect(this,&FactData::modifiedChanged,static_cast<FactData*>(parent),&FactData::modifiedChanged);


    /*FactSystem *fs=FactSystem::instance();
    connect(this,&Fact::nameChanged,fs,&FactSystem::nameChanged);
    connect(this,&Fact::sizeChanged,fs,&FactSystem::sizeChanged);

    connect(this,&Fact::valueChanged,fs,&FactSystem::valueChanged);
    connect(this,&Fact::titleChanged,fs,&FactSystem::titleChanged);
    connect(this,&Fact::descrChanged,fs,&FactSystem::descrChanged);
    connect(this,&Fact::textChanged,fs,&FactSystem::textChanged);

    connect(this,&Fact::enabledChanged,fs,&FactSystem::enabledChanged);
    connect(this,&Fact::visibleChanged,fs,&FactSystem::visibleChanged);
    connect(this,&Fact::statusChanged,fs,&FactSystem::statusChanged);
    connect(this,&Fact::activeChanged,fs,&FactSystem::activeChanged);
    connect(this,&Fact::progressChanged,fs,&FactSystem::progressChanged);

    connect(this,&Fact::itemToBeInserted,fs,&FactSystem::itemToBeInserted);
    connect(this,&Fact::itemInserted,fs,&FactSystem::itemInserted);
    connect(this,&Fact::itemToBeRemoved,fs,&FactSystem::itemToBeRemoved);
    connect(this,&Fact::itemRemoved,fs,&FactSystem::itemRemoved);*/
  }
}
//=============================================================================
QVariant Fact::data(int col, int role) const
{
  switch(role){
    case ModelDataRole: return QVariant::fromValue(const_cast<Fact*>(this));
    case NameRole:      return name();
    case ValueRole:     return value();
    case TextRole:      return text();
    case Qt::ForegroundRole:
      if(col==Fact::FACT_MODEL_COLUMN_NAME){
        if(modified())return QColor(Qt::red).lighter();
        if(!enabled())return QColor(Qt::gray);
        if(active())return QColor(Qt::green).lighter();
        if(isZero())return QColor(Qt::gray);
        //if(treeItemType()==Fact::FactItem) return QColor(Qt::white);
        //return QColor(Qt::green).lighter(195);
        return QColor(Qt::white);//QVariant();
      }
      if(col==Fact::FACT_MODEL_COLUMN_VALUE){
        if(!enabled())return QColor(Qt::darkGray);
        if(dataType()==Fact::ActionData) return QColor(Qt::blue).lighter(170);
        if(size()) return QColor(Qt::darkGray); //expandable
        if(modified())return QColor(Qt::yellow);
        if(isZero())return QColor(Qt::gray);
        //if(ftype==ft_string) return QVariant();
        //if(ftype==ft_varmsk) return QColor(Qt::cyan);
        return QColor(Qt::cyan).lighter(180);
      }
      return QColor(Qt::darkCyan);
    case Qt::BackgroundRole:
      return QVariant();
    case Qt::FontRole: {
      QFont font(qApp->font());
      if(col==Fact::FACT_MODEL_COLUMN_DESCR) return QVariant();
      if(treeItemType()!=Fact::FactItem && col==Fact::FACT_MODEL_COLUMN_NAME)
        font.setBold(true);
      //if(ftype>=ft_regPID) return QFont("Monospace",-1,column==tc_field?QFont::Bold:QFont::Normal);
      //if(col==FACT_MODEL_COLUMN_NAME) return QFont("Monospace",-1,QFont::Normal,isModified());
      //if(ftype==ft_string) return QFont("",-1,QFont::Normal,true);
      return font;
    }
    case Qt::ToolTipRole:
      if(col==Fact::FACT_MODEL_COLUMN_NAME){
        QStringList st;
        QString sDataType;
        if(m_dataType!=NoData)sDataType=QMetaEnum::fromType<DataType>().valueToKey(m_dataType);
        if(!units().isEmpty())sDataType+=(sDataType.isEmpty()?"":", ")+units();
        if(sDataType.isEmpty())st<<QString("%1").arg(name());
        else st<<QString("%1 [%2]").arg(name()).arg(sDataType);
        if(!descr().isEmpty()) st<<descr();
        st<<path();
        if(!m_enumStrings.isEmpty()){
          if(m_enumStrings.size()>25)st<<QString("{%1}").arg(m_enumStrings.size());
          else st<<QString("{%1}").arg(m_enumStrings.join(','));
        }
        return st.join('\n');
      }else if(col==Fact::FACT_MODEL_COLUMN_VALUE){
        if(size()){
          QString s=name();
          foreach(FactTree *i,childItems()){
            Fact *fc=static_cast<Fact*>(i);
            s+=QString("\n%1: %2").arg(fc->title()).arg(fc->text());
          }
          return s;
        }else descr();
      }
      return data(col,Qt::DisplayRole);
  }

  //value roles
  if(role!=Qt::DisplayRole && role!=Qt::EditRole)
    return QVariant();

  switch(col){
    case Fact::FACT_MODEL_COLUMN_NAME: return title();
    case Fact::FACT_MODEL_COLUMN_VALUE:{
      if(dataType()==Fact::ActionData) return QString("<exec>");
      if(dataType()==Fact::ScriptData) return status();
      const QString s=text();
      if(s.isEmpty()){
        if(!status().isEmpty()) return status();
        if(treeItemType()==GroupItem || treeItemType()==SectionItem){
          //if(isZero())return tr("default");
          return QVariant();
        }
      }
      if(role==Qt::EditRole && enumStrings().size()<=1){
        if(dataType()==BoolData)return value().toBool();
        if(dataType()==IntData)return value().toInt();
        if(dataType()==FloatData)return value().toDouble();
      }
      return s;
    }
    case Fact::FACT_MODEL_COLUMN_DESCR: return descr();
  }
  return QVariant();
}
//=============================================================================
QByteArray Fact::hash() const
{
  QCryptographicHash h(QCryptographicHash::Md5);
  hashData(&h);
  return h.result();
}
void Fact::hashData(QCryptographicHash *h) const
{
  foreach(FactTree *item,childItems()){
    Fact *f=static_cast<Fact*>(item);
    f->hashData(h);
  }
  //h->addData(name().toUtf8());
  h->addData(title().toUtf8());
  h->addData(descr().toUtf8());
  //h->addData(section().toUtf8());
  //h->addData(QString::number(num()).toUtf8());
  h->addData(QString::number(precision()).toUtf8());
  h->addData(min().toString().toUtf8());
  h->addData(max().toString().toUtf8());
  h->addData(QString::number(dataType()).toUtf8());
  h->addData(QString::number(treeItemType()).toUtf8());
  h->addData(QString::number(size()).toUtf8());
  h->addData(enumStrings().join("").toUtf8());

  h->addData(text().toUtf8());
}
//=============================================================================
void Fact::bind(FactData *item)
{
  if(_bindedFact){
    disconnect(static_cast<Fact*>(_bindedFact),&Fact::statusChanged,this,&Fact::statusChanged);
  }
  if(item==this)return;
  FactData::bind(item);
  connect(static_cast<Fact*>(item),&Fact::statusChanged,this,&Fact::statusChanged);
  emit statusChanged();
}
//=============================================================================
QVariant Fact::findValue(const QString &namePath)
{
  Fact *f=fact(namePath);
  if(!f){
    qWarning("FactSystem fact not found: %s",namePath.toUtf8().data());
    return QVariant();
  }
  if(f->dataType()==Fact::EnumData)return f->text();
  return f->value();
}
//=============================================================================
Fact * Fact::childFact(int i) const
{
  return static_cast<Fact*>(child(i));
}
//=============================================================================
Fact * Fact::fact(const QString &factNamePath) const
{
  for(int i=0;i<size();++i){
    Fact *f=childFact(i);
    if(f->treeItemType()==FactItem && (f->name()==factNamePath || f->path().endsWith(factNamePath)))
      return f;
  }
  for(int i=0;i<size();++i){
    Fact *f=childFact(i);
    f=f->fact(factNamePath);
    if(f)return f;
  }
  return NULL;
}
//=============================================================================
Fact * Fact::childByTitle(const QString &factTitle) const
{
  for(int i=0;i<size();++i){
    Fact *f=childFact(i);
    if(f->title()==factTitle)
      return f;
  }
  return NULL;
}
//=============================================================================
/*Fact * Fact::byPath(const QString &itemNamePath) const
{
  QString s=itemNamePath;
  Fact *item=const_cast<Fact*>(this);
  while(!s.isEmpty()){
    if(item->isFactsList()){
      item=item->childByName(s);
      break;
    }else{
      int i=s.indexOf(pathDelimiter);
      if(i<=0)break;
      item=item->childByName(s.left(i));
      s.remove(0,i+1);
    }
  }
  return item;
}*/
//=============================================================================
QString Fact::titlePath(const QChar pathDelimiter) const
{
  QString s;
  for(const FactTree *i=this;i;i=i->parentItem()){
    const Fact *f=static_cast<const Fact*>(i);
    if(i->treeItemType()==RootItem){
      s.prepend(pathDelimiter);
      break;
    }
    if(i->treeItemType()==SectionItem)continue;
    QString s2=f->title();
    if(s.isEmpty())s=s2;
    else s.prepend(s2+pathDelimiter);
  }
  return s.isEmpty()?title():s;
}
//=============================================================================
bool Fact::lessThan(Fact *rightFact) const
{
  //no sorting by default
  return num()<rightFact->num();
}
//=============================================================================
void Fact::trigger(void)
{
  if(!enabled())return;
  //qDebug()<<"trigger"<<name();
  emit triggered();
}
//=============================================================================
FactListModel * Fact::model() const
{
  return m_model;
}
bool Fact::enabled() const
{
  return m_enabled;
}
void Fact::setEnabled(const bool &v)
{
  if(m_enabled==v)return;
  m_enabled=v;
  emit enabledChanged();
}
bool Fact::visible() const
{
  return m_visible;
}
void Fact::setVisible(const bool &v)
{
  if(m_visible==v)return;
  m_visible=v;
  emit visibleChanged();
}
QString Fact::section() const
{
  return m_section;
}
void Fact::setSection(const QString &v)
{
  QString s=v.trimmed();
  if(m_section==s)return;
  m_section=s;
  emit sectionChanged();
}
QString Fact::status() const
{
  if(_bindedFact) return static_cast<Fact*>(_bindedFact)->status();
  if((treeItemType()==GroupItem||treeItemType()==SectionItem) && m_dataType==ConstData && m_status.isEmpty()){
    return size()>0?QString::number(size()):QString();
  }
  return m_status;
}
void Fact::setStatus(const QString &v)
{
  if(_bindedFact){
    static_cast<Fact*>(_bindedFact)->setStatus(v);
    return;
  }
  QString s=v.trimmed();
  if(m_status==s)return;
  m_status=s;
  emit statusChanged();
}
bool Fact::active() const
{
  if(_bindedFact) return static_cast<Fact*>(_bindedFact)->active();
  return m_active;
}
void Fact::setActive(const bool &v)
{
  if(_bindedFact){
    static_cast<Fact*>(_bindedFact)->setActive(v);
    return;
  }
  if(m_active==v)return;
  m_active=v;
  emit activeChanged();
}
int Fact::progress() const
{
  if(_bindedFact) return static_cast<Fact*>(_bindedFact)->progress();
  return m_progress;
}
void Fact::setProgress(const int &v)
{
  if(_bindedFact){
    static_cast<Fact*>(_bindedFact)->setProgress(v);
    return;
  }
  if(m_progress==v)return;
  m_progress=v;
  emit progressChanged();
}
QString Fact::iconSource() const
{
  return m_iconSource;
}
void Fact::setIconSource(const QString &v)
{
  if(m_iconSource==v)return;
  m_iconSource=v;
  emit iconSourceChanged();
}
QString Fact::qmlMenu() const
{
  return m_qmlMenu;
}
void Fact::setQmlMenu(const QString &v)
{
  if(m_qmlMenu==v)return;
  m_qmlMenu=v;
  emit qmlMenuChanged();
}
QString Fact::qmlEditor() const
{
  return m_qmlEditor;
}
void Fact::setQmlEditor(const QString &v)
{
  if(m_qmlEditor==v)return;
  m_qmlEditor=v;
  emit qmlEditorChanged();
}
bool Fact::busy() const
{
  return m_busy;
}
void Fact::setBusy(const bool &v)
{
  if(m_busy==v)return;
  m_busy=v;
  emit busyChanged();
}
//=============================================================================
//=============================================================================
