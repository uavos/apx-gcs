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
#include "ValueEditorArray.h"
//=============================================================================
ValueEditorArray::ValueEditorArray(NodesItemGroup *group, QWidget *parent)
  :ValueEditor(group,parent),group(group)
{
}
ValueEditorArray::~ValueEditorArray()
{
}
//=============================================================================
QWidget * ValueEditorArray::getWidget(QWidget *parent)
{
  treeView = new QTreeView(parent);
  QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  treeView->setSizePolicy(sizePolicy);

  treeView->setAlternatingRowColors(true);
  treeView->setRootIsDecorated(false);
  treeView->setEditTriggers(treeView->editTriggers()|QAbstractItemView::SelectedClicked);
  treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  treeView->header()->setMinimumSectionSize(50);
  treeView->setAutoFillBackground(false);

  model=new ValueEditorArrayModel(group,parent);
  treeView->setModel(model);
  treeView->setItemDelegate(new ValueEditorArrayDelegate(parent));

  return treeView;
}
//=============================================================================
//=============================================================================
ValueEditorArrayModel::ValueEditorArrayModel(NodesItemGroup *group, QObject *parent)
  :QAbstractItemModel(parent),group(group)
{
  showNum=group->name!="controls";
}
//=============================================================================
QVariant ValueEditorArrayModel::data(const QModelIndex &index, int role) const
{
  int col=index.column();
  uint item_n=index.row();

  bool binded=true;
  NodesItemField *bind_item=static_cast<NodesItemField*>(group->child(0));
  if(bind_item->ftype==ft_varmsk){
    if(bind_item->child(item_n)->isZero()) binded=false;
  }else bind_item=NULL;


  switch(role){
    case Qt::ForegroundRole:{
      //check if binding exists & enabled
      if(showNum&&col==0){
        //highlight binded numbers
        if(bind_item) return binded?QColor(Qt::darkCyan):QColor(Qt::gray);
        return QColor(Qt::cyan).lighter(180);
      }
      if(bind_item && (!binded))return QColor(Qt::darkGray);
      NodesItemField *f=field(index);
      if(!f)return QVariant();
      //zero values are gray
      if(f->ftype==ft_varmsk || f->field->conf_name.startsWith("ctr_ch[")){
        //check if same vars binded to other numbers
        if(match(index,Qt::DisplayRole,data(index,Qt::DisplayRole),2,Qt::MatchFlags(Qt::MatchExactly|Qt::MatchWrap)).size()>1)
          return QColor(Qt::blue).lighter(180);
        else return QColor(Qt::cyan);
      }
      if(f->isModified()) return QColor(Qt::red).lighter();
      if(f->isZero())return QColor(Qt::darkGray);
      return QVariant();
    }
    case Qt::FontRole:
      if(showNum&&col==0) return QFont("",-1,QFont::Bold);
      return QVariant();
    default:
      if(role==Qt::DisplayRole||role==Qt::EditRole) break;
      return QVariant();
  }
  if(showNum){
    if(col==0)return static_cast<NodesItemField*>(group->child(0))->child(item_n)->name;
    col--;
  }
  if(!index.internalPointer())return QVariant();
  NodesItemField *f=field(index);
  if(!f)return QVariant();

  if(col>0 && bind_item && (!binded) && f->isZero())return QVariant();

  QVariant v=f->getValue();
  if(f->field->conf_name.startsWith("ctr_ch[")){
    if(role==Qt::DisplayRole)return QString("CH%1").arg(v.toUInt()+1);
    else return v.toUInt()+1;
  }

  QString su=f->field->units();
  if(f->field->conf_name.startsWith("ctr_ch_") && su=="%"){
    int vi=v.toInt();
    if(role!=Qt::DisplayRole)return vi;
    return v>0?QString("+%1").arg(vi):QString("%1").arg(vi);
  }
  return f->data(NodesItem::tc_value,role);
}
//=============================================================================
bool ValueEditorArrayModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
  if (role!=Qt::EditRole || (!index.internalPointer()))return false;
  NodesItemField *f=field(index);
  if(!f)return false;
  bool rv;
  if(f->field->conf_name.startsWith("ctr_ch[")){
    int v=value.toInt()-1;
    if(v<0)v=0;
    foreach(NodesItemField *fx,f->field->node->fields){
      if(!fx->conf_name.startsWith("ctr_ch_"))continue;
      if(v>=fx->childCount())return false;
      break;
    }
    rv=f->setValue(v);
  }else rv=f->setValue(value);
  connect(f->field,SIGNAL(changed()),this,SIGNAL(layoutChanged()));
  //group->emit_changed();
  emit layoutChanged();
  return rv;
}
//=============================================================================
NodesItemField * ValueEditorArrayModel::field(const QModelIndex &index) const
{
  NodesItemField *f=static_cast<NodesItemField*>(index.internalPointer());
  if((int)f->array!=f->childCount())return NULL;//protect
  uint item_n=index.row();
  //check for controls array
  if(f->conf_name.startsWith("ctr_") && f->conf_name.contains('_')){
    QString s=f->conf_name;
    s=s.left(s.lastIndexOf('_'));
    //xx_yy_zz nested array
    if(s.contains('_')){
      foreach(NodesItem *i,group->childItems){
        NodesItemField *fi=static_cast<NodesItemField*>(i);
        if(!fi->conf_name.startsWith(s))continue;
        if((int)item_n>=fi->childCount())break; //protect
        item_n=fi->child(item_n)->getValue().toUInt();
        break;
      }
    }
  }
  if((int)item_n>=f->childCount())return NULL;
  return static_cast<NodesItemField*>(f->child(item_n));
}
//=============================================================================
QModelIndex ValueEditorArrayModel::index(int row, int column, const QModelIndex &parent) const
{
  if (!hasIndex(row, column, parent))return QModelIndex();
  if (parent.isValid()) //root
    return QModelIndex();
  if(showNum){
    if(column<1)return createIndex(row,column);
    column--;
  }
  return createIndex(row,column+(showNum?1:0),column<group->childCount()?group->child(column):NULL);
}
//=============================================================================
QModelIndex ValueEditorArrayModel::parent(const QModelIndex &child) const
{
  Q_UNUSED(child)
  return QModelIndex();
}
//=============================================================================
int ValueEditorArrayModel::rowCount(const QModelIndex &parent) const
{
  if(parent.isValid() || (!group->childCount()))return 0;
  return static_cast<NodesItemField*>(group->child(0))->array;
}
//=============================================================================
int ValueEditorArrayModel::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  return group->childCount()+(showNum?1:0);
}
//=============================================================================
QVariant ValueEditorArrayModel::headerData(int section, Qt::Orientation orientation,int role) const
{
  Q_UNUSED(orientation);
  if(showNum){
    if(section==0)return QVariant("#");
    section--;
  }
  if (role==Qt::DisplayRole){
    QString s=group->child(section)->name;
    return s.contains('_')?s.mid(s.lastIndexOf('_')+1):s;
  }
  if (role==Qt::ToolTipRole) return group->child(section)->descr;
  return QVariant();
}
//=============================================================================
Qt::ItemFlags ValueEditorArrayModel::flags(const QModelIndex & index) const
{
  const Qt::ItemFlags f=Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable;
  const Qt::ItemFlags f0=Qt::ItemIsEnabled|Qt::ItemIsSelectable;
  int column=index.column();
  if(showNum){
    if(column==0)return f0;
    column--;
  }
  if(column>0){
    //check if binding enabled
    NodesItemField *bind_item=static_cast<NodesItemField*>(group->child(0));
    if(bind_item->ftype==ft_varmsk && bind_item->child(index.row())->isZero())return f0;
  }
  return f;
}
//=============================================================================
QWidget *ValueEditorArrayDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,const QModelIndex &index) const
{
  if(!index.internalPointer())return NodesItemDelegate::createEditor(parent,option,index);
  NodesItemField *f=static_cast<NodesItemField*>(index.internalPointer());
  QWidget *e=createEditorEx(parent,option,index,f);
  if(f && f->conf_name.startsWith("ctr_ch[")){
    if(e->inherits("QSpinBox")){
      QSpinBox *sb=static_cast<QSpinBox*>(e);
      sb->setPrefix("CH");
      foreach(NodesItemField *fx,f->node->fields){
        if(!fx->conf_name.startsWith("ctr_ch_"))continue;
        sb->setMinimum(1);
        sb->setMaximum(fx->childCount());
        break;
      }
    }
  }
  return e;
}
//=============================================================================
