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
#include <QtWidgets>
#include <FactTreeModel.h>
#include "FactDelegate.h"
//=============================================================================
FactDelegate::FactDelegate(QObject *parent)
 : QItemDelegate(parent)
{
  progressBar=new QProgressBar();
  progressBar->setObjectName("nodeProgressBar");
  progressBar->setMaximum(100);
}
FactDelegate::~FactDelegate()
{
  delete progressBar;
}
//=============================================================================
QWidget *FactDelegate::createEditor(QWidget *parent,const QStyleOptionViewItem &option,const QModelIndex &index) const
{
  Q_UNUSED(option);
  Fact *f=index.data(FactListModel::ModelDataRole).value<Fact*>();
  if(!f) return QItemDelegate::createEditor(parent,option,index);
  QWidget *e=NULL;
  QString su;
  switch(f->dataType()){
    case Fact::EnumData:{
      QComboBox *cb=new QComboBox(parent);
      cb->setFrame(false);
      cb->addItems(f->enumStrings());
      cb->view()->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Ignored);
      //cb->view()->setMaximumWidth(cb->view()->sizeHintForColumn(0));
      e=cb;
    }break;
    case Fact::BoolData:{
      QComboBox *cb=new QComboBox(parent);
      cb->setFrame(false);
      cb->addItems(QStringList()<<QVariant(false).toString()<<QVariant(true).toString());
      cb->view()->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Ignored);
      //cb->view()->setMaximumWidth(cb->view()->sizeHintForColumn(0));
      e=cb;
    }break;
    case Fact::ActionData:{
      QPushButton *btn=new QPushButton(parent);
      //btn->setFlat(true);
      QPalette newPalette = btn->palette();
      newPalette.setBrush(QPalette::Window, QBrush(QColor(255,255,255,80)));
      btn->setPalette(newPalette);
      btn->setBackgroundRole(QPalette::Window);
      btn->setObjectName("treeViewButton");
      connect(btn,&QPushButton::clicked,f,&Fact::trigger);
      return btn;
    }break;
    /*case ft_varmsk:{
      QComboBox *cb=new QComboBox(parent);
      cb->setFrame(false);
      cb->addItem("");
      cb->addItems(QMandala::instance()->local->names);
      cb->setEditable(true);
      cb->view()->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Ignored);
      cb->view()->setMaximumWidth(cb->view()->sizeHintForColumn(0)*2);
      e=cb;
    }break;
    case ft_script:
      //e=new ValueEditorScript(f,parent);
    break;*/
    default:
      su=f->units();
  }
  if(!e) e=QItemDelegate::createEditor(parent,option,index);
  //e->setAutoFillBackground(true);
  /*static_cast<QFrame*>(e)->setFrameShape(QFrame::NoFrame);
  static_cast<QFrame*>(e)->setLineWidth(0);
  static_cast<QFrame*>(e)->setMidLineWidth(0);*/
  e->setContentsMargins(-1,-1,-1,-1);
  QFont font(index.data(Qt::FontRole).value<QFont>());
  font.setPointSize(font.pointSize()+2);
  e->setFont(font);
  if(su.size()){
    if(su=="hex"){
      if(e->inherits("QSpinBox")){
        QSpinBox *sb=static_cast<QSpinBox*>(e);
        sb->setDisplayIntegerBase(16);
      }
    }else{
      su.prepend(" ");
      if(e->inherits("QDoubleSpinBox"))static_cast<QDoubleSpinBox*>(e)->setSuffix(su);
      else if(e->inherits("QSpinBox"))static_cast<QSpinBox*>(e)->setSuffix(su);
    }
  }
  return e;
}
void FactDelegate::setEditorData(QWidget *editor,const QModelIndex &index) const
{
  QItemDelegate::setEditorData(editor,index);
}
void FactDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const
{
  Fact *f=index.data(FactListModel::ModelDataRole).value<Fact*>();
  if(f->dataType()==Fact::ActionData)return;
  QItemDelegate::setModelData(editor,model,index);
}
//=============================================================================
void FactDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,const QModelIndex &index) const
{
  if(index.column()==FactTreeModel::FACT_MODEL_COLUMN_DESCR){
    Fact *f=index.data(FactListModel::ModelDataRole).value<Fact*>();
    if(f->progress()>0){
      //qDebug()<<f<<f->progress();
      if(drawProgress(painter,option,index,f->progress()))
        return;
    }
  }
  QItemDelegate::paint(painter,option,index);
}
//=============================================================================
bool FactDelegate::drawProgress(QPainter *painter,const QStyleOptionViewItem &option,const QModelIndex &index,uint progress) const
{
  if(progress==0)return false;
  QStyleOptionViewItem opt(option);
  int w=opt.rect.width()/2;
  if(w>150)w=150;
  else if(w<80)w=80;
  if(w>opt.rect.width()*0.9)w=opt.rect.width()*0.9;
  opt.rect.setWidth(opt.rect.width()-w);
  QItemDelegate::paint(painter,opt,index);
  QRect rect(option.rect);
  rect.translate(opt.rect.width(),0);
  rect.setWidth(w);
  painter->fillRect(rect,index.data(Qt::BackgroundRole).value<QColor>());
  progressBar->resize(rect.size());
  //progressBar->setMaximumHeight(12);
  progressBar->setValue(progress);
  painter->save();
  painter->translate(rect.left(),rect.top()+(rect.height()-progressBar->height())/2);
  progressBar->render(painter);
  painter->restore();
  return true;
}
//=============================================================================
