#include <QHeaderView>
#include <QtWidgets>
#include "FactDelegate.h"
#include "FactTreeView.h"
#include "FactTreeModel.h"
#include "SvgIcon.h"
//=============================================================================
FactTreeView::FactTreeView(QWidget *parent)
 : QTreeView(parent)
{
  setContextMenuPolicy(Qt::CustomContextMenu);
  setFrameShape(QFrame::NoFrame);
  setSortingEnabled(true);
  sortByColumn(FactTreeModel::FACT_MODEL_COLUMN_NAME,Qt::AscendingOrder);
  header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  header()->setDefaultSectionSize(50);
  header()->setMinimumSectionSize(70);
  header()->setVisible(false);
  setEditTriggers(QAbstractItemView::AllEditTriggers);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  //setItemDelegate(new FactDelegate(this));
  setUniformRowHeights(true);
  setItemDelegate(new FactDelegate(this));

  setAlternatingRowColors(true);
  setUniformRowHeights(true);
  setAnimated(false);
  setIndentation(10);
}
//=============================================================================
//=============================================================================
FactProxyModel::FactProxyModel(QObject *parent)
  : QSortFilterProxyModel(parent)
{
  sortNames<<"shiva"<<"nav"<<"ifc"<<"swc"<<"cas"<<"gps"<<"mhx"<<"servo"<<"bldc";
}
//=============================================================================
bool FactProxyModel::filterAcceptsRow(int sourceRow,const QModelIndex &sourceParent) const
{
  //if(!sourceParent.isValid())return true;
  QModelIndex index = sourceModel()->index(sourceRow,FactTreeModel::FACT_MODEL_COLUMN_NAME,sourceParent);
  return showThis(index);
}
//=============================================================================
bool FactProxyModel::showThis(const QModelIndex index) const
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
    QModelIndex childIndex=sourceModel()->index(i,FactTreeModel::FACT_MODEL_COLUMN_NAME,index);
    if(!childIndex.isValid())break;
    if(showThis(childIndex))return true;
  }
  return false;
}
//=============================================================================
bool FactProxyModel::lessThan(const QModelIndex &left,const QModelIndex &right) const
{
  //only first col sorted
  if(left.column()!=FactTreeModel::FACT_MODEL_COLUMN_NAME || right.column()!=FactTreeModel::FACT_MODEL_COLUMN_NAME)
    return left.row()<right.row();
  Fact *item_left=left.data(FactListModel::ModelDataRole).value<Fact*>();
  Fact *item_right=right.data(FactListModel::ModelDataRole).value<Fact*>();
  if(!(item_left && item_right))
    return QSortFilterProxyModel::lessThan(left,right);
  return item_left->lessThan(item_right);
}
//=============================================================================
//=============================================================================
FactTreeWidget::FactTreeWidget(QWidget *parent)
  : QWidget(parent),
    model(NULL)
{
  setWindowTitle("FactTree");
  QVBoxLayout *layout=new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);
  tree=new FactTreeView(this);
  QSizePolicy sp=tree->sizePolicy();
  sp.setVerticalPolicy(QSizePolicy::Expanding);
  tree->setSizePolicy(sp);
  eFilter=new QLineEdit(this);
  eFilter->setFrame(false);
  eFilter->setClearButtonEnabled(true);
  eFilter->setVisible(false);

  toolBar=new QToolBar(this);
  //toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  toolBar->setIconSize(QSize(14,14));
  toolBar->layout()->setMargin(0);
  aBack=new QAction(SvgIcon(":/icons/sets/ionicons/android-arrow-back.svg"),tr("Back"),this);
  aBack->setVisible(false);
  connect(aBack,&QAction::triggered,this,&FactTreeWidget::back);
  toolBar->addAction(aBack);


  layout->addWidget(toolBar);
  layout->addWidget(tree);

  //model
  proxy=new FactProxyModel(this);
  tree->setModel(proxy);

  updateActions();

  connect(eFilter,&QLineEdit::textChanged,this,&FactTreeWidget::filterChanged);
  connect(tree,&FactTreeView::collapsed,this,&FactTreeWidget::collapsed);
}
//=============================================================================
void FactTreeWidget::setRoot(Fact *fact, bool filterEdit, bool backNavigation)
{
  setWindowTitle(fact->title());
  if(model){
    model->deleteLater();
  }
  model=new FactTreeModel(fact);
  proxy->setSourceModel(model);
  if(backNavigation){
    aBack->setVisible(true);
    connect(tree,&FactTreeView::doubleClicked,this,&FactTreeWidget::doubleClicked);
  }
  if(filterEdit){
    eFilter->setVisible(true);
    toolBar->addWidget(eFilter);
  }
  toolBar->setVisible(backNavigation||filterEdit);
}
//=============================================================================
void FactTreeWidget::filterChanged()
{
  QString s=eFilter->text();
  QRegExp regExp(s,Qt::CaseSensitive,QRegExp::WildcardUnix);
  proxy->setFilterRegExp(regExp);
  updateActions();
  if(s.size()){
    tree->expandAll();
  }else{
    tree->collapseAll();
    emit treeReset();
  }
}
//=============================================================================
void FactTreeWidget::doubleClicked(const QModelIndex &index)
{
  Fact *f=index.data(FactListModel::ModelDataRole).value<Fact*>();
  if(!f)return;
  if(!f->size())return;
  rootList.append(tree->rootIndex());
  tree->setRootIndex(index);
  updateActions();
}
//=============================================================================
void FactTreeWidget::collapsed(const QModelIndex &index)
{
  Fact *f=index.data(FactListModel::ModelDataRole).value<Fact*>();
  if(f)model->recursiveDisconnect(f);
}
//=============================================================================
void FactTreeWidget::updateActions()
{
  //Fact *f=tree->rootIndex().data(FactListModel::ModelDataRole).value<Fact*>();
  rootList.removeAll(tree->rootIndex());
  bool bBack=!rootList.isEmpty();
  aBack->setEnabled(bBack);
}
//=============================================================================
void FactTreeWidget::back()
{
  if(rootList.isEmpty())return;
  tree->setRootIndex(rootList.takeLast());
  updateActions();
}
//=============================================================================
//=============================================================================
