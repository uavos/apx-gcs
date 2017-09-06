#include "MissionListView.h"
#include "QMandala.h"
#include "MapView.h"
//=============================================================================
MissionListView::MissionListView(QSplitter *splitter, MissionModel *model)
  :QWidget(splitter->widget(0)),widget(splitter->widget(0)),splitter(splitter),model(model)
{
  setupUi(this);
  setWindowFlags(Qt::SubWindow);

  splitter->setChildrenCollapsible(false);
  saveStateTimer.setSingleShot(true);
  saveStateTimer.setInterval(1000);
  connect(&saveStateTimer,SIGNAL(timeout()),this,SLOT(saveState()));
  connect(splitter,SIGNAL(splitterMoved(int,int)),&saveStateTimer,SLOT(start()));

  setCursor(Qt::ArrowCursor);
  adjustSize();
#ifdef __APPLE__
  wptExpand->setVisible(true);
  on_lbTitle_clicked();
#else
  wptExpand->setVisible(false);
  resize(QSettings().value("MissionListViewWidth",width()).toInt(),lbTitle->height());
#endif

  tree->setModel(model);
  tree->setItemDelegate(model->delegate);
  tree->setSelectionModel(model->selectionModel);
  treeReset();
  tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  tree->setContextMenuPolicy(Qt::ActionsContextMenu);
  tree->setEditTriggers(QAbstractItemView::AllEditTriggers);
  tree->setAlternatingRowColors(true);
  tree->setAnimated(false);
  tree->setIndentation(8);
  tree->setUniformRowHeights(true);

  connect(model,SIGNAL(addedRemoved()),this,SLOT(treeReset()),Qt::QueuedConnection);
  connect(model,SIGNAL(missionNameChanged(QString)),this,SLOT(setTitle(QString)));

  setVisible(true);
}
//=============================================================================
void MissionListView::treeReset()
{
  //tree->expandToDepth(0);
  tree->collapseAll();
  tree->expand(model->index(model->runways->row(),0));
  tree->expand(model->index(model->waypoints->row(),0));
}
//=============================================================================
void MissionListView::on_lbTitle_clicked()
{
#ifndef __APPLE__
  if(wptExpand->isVisible()){
    saveState();
    wptExpand->setVisible(false);
    resize(width(),lbTitle->height());
    treeReset();
    wptExpand->setVisible(false);
    setParent(widget);
    setVisible(true);
  }else
#endif
  {
    wptExpand->setVisible(true);
    treeReset();
    /*int h=QSettings().value("MissionListViewHeight",parentWidget()->height()*0.3).toInt();
    int maxh=parentWidget()->height()*0.7;
    if(h>maxh) h=maxh;
    if(h!=0)resize(width(),h);
    else adjustSize();*/
    splitter->insertWidget(0,this);
    //resize(QSettings().value("MissionListViewWidth",width()).toInt(),height());
    splitter->restoreState(QSettings().value("MissionListViewState").toByteArray());
    QSettings().setValue("MissionListViewWidth",width());
  }
}
//=============================================================================
void MissionListView::saveState()
{
  if(wptExpand->isVisible())QSettings().setValue("MissionListViewState",splitter->saveState());
  QSettings().setValue("MissionListViewWidth",width());
}
//=============================================================================
void MissionListView::setTitle(QString title)
{
  if(title.isEmpty())title=lbTitle->toolTip();
  lbTitle->setText(title);
}
//=============================================================================
void MissionListView::on_tree_clicked(const QModelIndex &index)
{
  MissionItem *mi=model->item(index);
  if(!mi)return;
  if(!mi->inherits("MissionItemObject"))return;
  MissionItemObject *pi=static_cast<MissionItemObject*>(mi);
  emit point_clicked(pi);
}
//=============================================================================
