#include "NodesView.h"
#include "ValueEditor.h"
#include "ValueEditorArray.h"
#include "ValueEditorScript.h"
#include "ValueEditorNgrp.h"
//=============================================================================
NodesTreeView::NodesTreeView(QWidget *parent)
 : QTreeView(parent)
{
  setContextMenuPolicy(Qt::CustomContextMenu);
  setSortingEnabled(true);
  sortByColumn(NodesItem::tc_field,Qt::AscendingOrder);
  header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  header()->setMinimumSectionSize(70);
  setEditTriggers(QAbstractItemView::AllEditTriggers);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setItemDelegate(new NodesItemDelegate(this));

  setAlternatingRowColors(true);
  setUniformRowHeights(true);
  setAnimated(false);
  setIndentation(10);
}
//=============================================================================
//=============================================================================
NodesItemDelegate::NodesItemDelegate(QObject *parent)
 : QItemDelegate(parent)
{
  progressBar=new QProgressBar();
  progressBar->setObjectName("nodeProgressBar");
}
NodesItemDelegate::~NodesItemDelegate()
{
  delete progressBar;
}
//=============================================================================
QWidget *NodesItemDelegate::createEditor(QWidget *parent,const QStyleOptionViewItem &option,const QModelIndex &index) const
{
  Q_UNUSED(option);
  NodesItem *i=index.data(NodesModel::NodesItemRole).value<NodesItem*>();
  //qDebug()<<i;
  if(!i)//(i && (index.column()==NodesItem::tc_value)))
    return QItemDelegate::createEditor(parent,option,index);
  QWidget *e=NULL;
  if(i->item_type==NodesItem::it_group){
    NodesItemGroup *group = static_cast<NodesItemGroup*>(i);
    e=new ValueEditorArray(group,parent);
  }else if(i->item_type==NodesItem::it_ngroup){
    e=new ValueEditorNgrp(i,parent);
  }else if(i->item_type>=NodesItem::it_field){
    NodesItemField *f = static_cast<NodesItemField*>(i);
    e=createEditorEx(parent,option,index,f);
    switch(f->ftype){
      case ft_option:
      case ft_varmsk:
        e=createEditorEx(parent,option,index,f);
        break;
    }
  }
  if(!e){
    e=QItemDelegate::createEditor(parent,option,index);
    e->setFont(index.data(Qt::FontRole).value<QFont>());
    e->setAutoFillBackground(true);
  }
  return e;
}
QWidget *NodesItemDelegate::createEditorEx(QWidget *parent, const QStyleOptionViewItem &option,const QModelIndex &index,NodesItemField *f) const
{
  QWidget *e=NULL;
  QString su;
  switch(f->ftype){
    case ft_option:{
      QComboBox *cb=new QComboBox(parent);
      cb->setFrame(false);
      cb->addItems(f->opts);
      cb->view()->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Ignored);
      //cb->view()->setMaximumWidth(cb->view()->sizeHintForColumn(0));
      e=cb;
    }break;
    case ft_varmsk:{
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
      e=new ValueEditorScript(f,parent);
    break;
    default:
      su=f->units();
  }
  if(!e) e=QItemDelegate::createEditor(parent,option,index);
  e->setFont(index.data(Qt::FontRole).value<QFont>());
  //e->setAutoFillBackground(true);
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
void NodesItemDelegate::setEditorData(QWidget *editor,const QModelIndex &index) const
{
  if(editor->inherits("QComboBox")){
    QComboBox *cb=static_cast<QComboBox*>(editor);
    cb->setCurrentIndex(cb->findText(index.model()->data(index, Qt::EditRole).toString()));
    return;
  }
  NodesItem *i=index.data(NodesModel::NodesItemRole).value<NodesItem*>();
  if(i && i->item_type==NodesItem::it_field && static_cast<NodesItemField*>(i)->ftype==ft_script)
    return;
  QItemDelegate::setEditorData(editor,index);
}
void NodesItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const
{
  if(editor->inherits("QComboBox")) {
    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    //comboBox->interpretText();//is this important for the QComboBox delegate??
    QString str  = comboBox->currentText();
    model->setData(index, str, Qt::EditRole);
    return;
  }
  NodesItem *i=index.data(NodesModel::NodesItemRole).value<NodesItem*>();
  if(i && i->item_type==NodesItem::it_field && static_cast<NodesItemField*>(i)->ftype==ft_script)
    return;
  QItemDelegate::setModelData(editor,model,index);
}
//=============================================================================
void NodesItemDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,const QModelIndex &index) const
{
  NodesItem *i=index.data(NodesModel::NodesItemRole).value<NodesItem*>();
  while(i){
    if(index.column()!=NodesItem::tc_descr)break;
    if(i->item_type==NodesItem::it_node){
      if(drawProgress(painter,option,index,static_cast<NodesItemNode*>(i)->progress()))
        return;
      break;
    }
    if(i->item_type==NodesItem::it_ngroup){
      if(drawProgress(painter,option,index,static_cast<NodesItemNgrp*>(i)->progress()))
        return;
      break;
    }
    break;
  }
  QItemDelegate::paint(painter,option,index);
}
//=============================================================================
bool NodesItemDelegate::drawProgress(QPainter *painter,const QStyleOptionViewItem &option,const QModelIndex &index,uint progress) const
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
//=============================================================================
QStringList NodesSortFilterProxyModel::sortNames=QStringList()
  <<"shiva"<<"nav"<<"ifc"<<"swc"<<"cas"<<"gps"<<"mhx"<<"servo"<<"bldc";
bool NodesSortFilterProxyModel::filterAcceptsRow(int sourceRow,const QModelIndex &sourceParent) const
{
  //if(!sourceParent.isValid())return true;
  QModelIndex index = sourceModel()->index(sourceRow,NodesItem::tc_field,sourceParent);
  return showThis(index);
}
bool NodesSortFilterProxyModel::showThis(const QModelIndex index) const
{
  QModelIndex useIndex=sourceModel()->index(index.row(),0,index.parent());
  if(sourceModel()->data(useIndex,Qt::DisplayRole).toString().contains(filterRegExp()))
    return true;
  //look for matching parents
  QModelIndex parentIndex=sourceModel()->parent(index);
  while(parentIndex.isValid()){
    if(sourceModel()->data(parentIndex,Qt::DisplayRole).toString().contains(filterRegExp()))
      return true;
    parentIndex=sourceModel()->parent(parentIndex);
  }
  //look for matching childs
  for(int i=0;i<sourceModel()->rowCount(index);i++){
    QModelIndex childIndex=sourceModel()->index(i,NodesItem::tc_field,index);
    if(!childIndex.isValid())break;
    if(showThis(childIndex))return true;
  }
  return false;
}
bool NodesSortFilterProxyModel::lessThan(const QModelIndex &left,const QModelIndex &right) const
{
  //only first col sorted
  if(left.column()!=NodesItem::tc_field || right.column()!=NodesItem::tc_field)
    return left.row()<right.row();
  NodesItem *item_left=left.data(NodesModel::NodesItemRole).value<NodesItem*>();
  NodesItem *item_right=right.data(NodesModel::NodesItemRole).value<NodesItem*>();
  //root?
  if(!(item_left && item_right))
    return QSortFilterProxyModel::lessThan(left,right);

  //fields not sorted
  if(item_left->item_type>NodesItem::it_node||item_right->item_type>NodesItem::it_node)
    return left.row()<right.row();

  //different types - node vs ngroup
  //if(item_left->item_type!=item_right->item_type)
    //return item_left->item_type<item_right->item_type;

  //try to sort by sortNames
  QString sleft=item_left->name;
  if(sleft.contains('.'))sleft=sleft.remove(0,sleft.indexOf('.')+1).trimmed();
  QString sright=item_right->name;
  if(sright.contains('.'))sright=sright.remove(0,sright.indexOf('.')+1).trimmed();
  if(sortNames.contains(sleft)){
    if(sortNames.contains(sright)){
      int ileft=sortNames.indexOf(sleft);
      int iright=sortNames.indexOf(sright);
      if(ileft!=iright) return ileft<iright;
    }else return true;
  }else if(sortNames.contains(sright)) return false;

  //compare names
  int ncmp=QString::localeAwareCompare(item_left->name,item_right->name);
  if(ncmp!=0)return ncmp<0;
  //nodes
  if(item_left->item_type==NodesItem::it_node && item_right->item_type==NodesItem::it_node){
    //try to sort by comment same names
    ncmp=QString::localeAwareCompare(QString(static_cast<NodesItemNode*>(item_left)->data(NodesItem::tc_value).toString()), QString(static_cast<NodesItemNode*>(item_right)->data(NodesItem::tc_value).toString()));
    if(ncmp==0){
      //try to sort by sn same names
      ncmp=QString::localeAwareCompare(QString(static_cast<NodesItemNode*>(item_left)->sn.toHex()), QString(static_cast<NodesItemNode*>(item_right)->sn.toHex()));
    }
  }
  if(ncmp==0) ncmp=left.row()-right.row();
  return ncmp<0;
}
//=============================================================================
