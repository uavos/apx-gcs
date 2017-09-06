#include "NodesFrame.h"
#include "QMandala.h"
#include "NodesView.h"
//=============================================================================
NodesFrame::NodesFrame(QWidget *parent) :
  QWidget(parent)
{
  mandala=qApp->property("Mandala").value<QMandala*>();

  setupUi(this);
  setWindowFlags(Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);

  progressBar->setVisible(false);
  progressCnt=0;
  progressBar->setFormat("%v/%m");
  progressBar->setTextVisible(true);
  progressBar->setObjectName("nodeProgressBar");

  aUpload->setShortcut(QKeySequence(Qt::Key_F5));

  toolBar=new QToolBar(this);
  //toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  toolBar->setIconSize(QSize(16,16));
  toolBar->layout()->setMargin(0);
  toolBarLayout->insertWidget(0,toolBar);
  btnUpload=new QToolButton(toolBar);
  btnUpload->setDefaultAction(aUpload);
  btnUpload->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  btnUpload->setObjectName("uploadButton");
  toolBar->addWidget(btnUpload);
  //toolBar->addSeparator();
  toolBar->addAction(aRequest);
  toolBar->addAction(aStats);
  toolBar->addAction(aStop);
  //toolBar->addSeparator();
  toolBar->addAction(aReload);
  toolBar->addAction(aLoad);
  toolBar->addAction(aSave);
  toolBar->addAction(aUndo);
  //toolBar->addSeparator();
  toolBar->addAction(aLoadTelemetry);
  //toolBar->addSeparator();
  toolBar->addAction(aClearCache);

  model=NULL;
  currentMandalaChanged(mandala->current);
  connect(mandala,SIGNAL(currentChanged(QMandalaItem*)),this,SLOT(currentMandalaChanged(QMandalaItem*)));
  connect(mandala,SIGNAL(uavNameChanged(QString)),lbUavName,SLOT(setText(QString)));
  connect(mandala,SIGNAL(sizeChanged(uint)),SLOT(mandalaSizeChanged(uint)),Qt::QueuedConnection);
  lbUavName->setVisible(false);

  tree->setModel(&proxy);

  connect(eFilter,SIGNAL(textChanged(QString)),this,SLOT(filterChanged()));

  connect(tree->selectionModel(),SIGNAL(selectionChanged(QItemSelection,QItemSelection)),SLOT(updateActions()));

  connect(mandala->local->rec,SIGNAL(fileLoaded()),SLOT(updateActions()),Qt::QueuedConnection);
  connect(mandala,SIGNAL(onlineChanged(bool)),SLOT(updateActions()),Qt::QueuedConnection);

  restoreGeometry(QSettings().value(objectName()).toByteArray());

  updateActionsTimer.setSingleShot(true);
  updateActionsTimer.setInterval(100);
  connect(&updateActionsTimer,SIGNAL(timeout()),this,SLOT(updateActionsDo()));

  updateActions();
}
//=============================================================================
void NodesFrame::currentMandalaChanged(QMandalaItem *m)
{
  if(model && model->isUpgrading()){
    qWarning("%s...",tr("Firmware upgrade in progress").toUtf8().data());
    mandala->setCurrent(models.key(model));
    return;
  }
  if(model){
    model->stop();
    model->requestManager.setEnabled(false);
  }
  if(models.contains(m)){
    model=models.value(m);
  }else{
    model=new NodesModel(m,this);
    models.insert(m,model);
    model->aUpload=aUpload;
    model->requestManager.setEnabled(false);
    connect(aClearCache,SIGNAL(triggered()),model,SLOT(cacheClear()));
    connect(aClearCache,SIGNAL(triggered()),model,SLOT(clear()));
    connect(model,SIGNAL(changed()),SLOT(updateActions()));
    connect(model,SIGNAL(syncProgress(uint)),SLOT(syncProgress(uint)));
    connect(aStop,SIGNAL(triggered()),model,SLOT(stop()));
    connect(aStop,SIGNAL(triggered()),&model->requestManager,SLOT(disableTimerTimeout()));
    connect(model,SIGNAL(done()),SLOT(updateActions()),Qt::QueuedConnection);
    connect(model,SIGNAL(done()),this,SLOT(resetTree()),Qt::QueuedConnection);
    connect(&model->requestManager,SIGNAL(busyChanged(bool)),progressBar,SLOT(setVisible(bool)));
    connect(&model->requestManager,SIGNAL(busyChanged(bool)),SLOT(updateActions()));
    connect(model,SIGNAL(modelReset()),this,SLOT(resetTree()),Qt::QueuedConnection);
  }
  proxy.setSourceModel(model);
  lbUavName->setToolTip(QString().sprintf("squawk: %u (%.4X)",m->ident.squawk,m->ident.squawk));
  progressBar->setVisible(model->requestManager.busy());
  resetTree();
  aRequest->trigger();
}
void NodesFrame::mandalaSizeChanged(uint sz)
{
  lbUavName->setVisible(sz>0);
}
//=============================================================================
void NodesFrame::on_lbUavName_clicked()
{
  mandala->changeCurrent();
}
//=============================================================================
void NodesFrame::filterChanged()
{
  QString s=eFilter->text();
  QRegExp regExp(s,Qt::CaseSensitive,QRegExp::WildcardUnix);
  proxy.setFilterRegExp(regExp);
  if(s.size()){
    //tree->setIndentation(2);
    tree->expandAll();
  }else{
    //tree->setIndentation(20);
    tree->collapseAll();
    resetTree();
  }
}
//=============================================================================
void NodesFrame::resetTree()
{
  //tree->collapseAll();
  //if(tree->selectionModel())tree->selectionModel()->clearSelection();
  QModelIndexList list=model->getGroupsIndexList();
  foreach(QModelIndex index,list){
    if(tree->isExpanded(proxy.mapFromSource(index)))return;
  }
  foreach(QModelIndex index,list){
    tree->expand(proxy.mapFromSource(index));
  }
}
//=============================================================================
QList<NodesItem*> NodesFrame::selectedItems(NodesItem::_item_type item_type) const
{
  QList<NodesItem*> list;
  foreach(QModelIndex index,tree->selectionModel()->selectedRows()){
    NodesItem *item = index.data(NodesModel::NodesItemRole).value<NodesItem*>();
    if(item && (item_type==NodesItem::it_root||item->item_type==item_type))
      list.append(item);
  }
  return list;
}
//=============================================================================
void NodesFrame::on_tree_customContextMenuRequested(const QPoint &pos)
{
  //scan selected items
  QList<QByteArray> sn_list;
  foreach(NodesItem *i,selectedItems(NodesItem::it_node)){
    NodesItemNode *node=static_cast<NodesItemNode*>(i);
    if(!model->nodes.values().contains(node)) continue;
    sn_list.append(model->nodes.key(node));
  }
  QMenu m(tree);

  //backups folder
  if(sn_list.size()==1){
    NodesItemNode *item = model->nodes.value(sn_list.first());
    if(item->isValid()&&item->backup_dir.count()){
      if(!m.isEmpty())m.addSeparator();
      QMenu *mu=m.addMenu(tr("Backups"));
      QStringList lst=item->backup_dir.entryList();
      for(int i=0;i<lst.size();i++){
        QString bname=lst.at(lst.size()-1-i);
        QAction *a=new QAction(bname.left(bname.lastIndexOf('.')),&m);
        connect(a,SIGNAL(triggered()),this,SLOT(nodeRestoreBackup()));
        a->setData(QVariant::fromValue(bname));
        mu->addAction(a);
      }
    }
  }
  //recent backup
  if(sn_list.size()){
    foreach(QByteArray sn,sn_list){
      NodesItemNode *item = model->nodes.value(sn);
      if(item->isValid()&&item->backup_dir.count()){
        QAction *a=new QAction(tr("Restore recent backup"),&m);
        connect(a,SIGNAL(triggered()),this,SLOT(nodeRestoreRecentBackup()));
        m.addAction(a);
        break;
      }
    }
  }
  //vehicle backups
  if(model->mvar!=mandala->local && model->isValid() && model->backup_dir.count()){
    QMenu *mu=m.addMenu(QString("%1 (%2)").arg(tr("Vehicle backups")).arg(model->mvar->ident.callsign));
    QStringList lst=model->backup_dir.entryList();
    for(int i=0;i<lst.size();i++){
      QString bname=lst.at(lst.size()-1-i);
      QAction *a=new QAction(bname.left(bname.lastIndexOf('.')),&m);
      connect(a,SIGNAL(triggered()),this,SLOT(vehicleRestoreBackup()));
      a->setData(QVariant::fromValue(bname));
      mu->addAction(a);
    }
  }
  //node specific commands
  QHash<QString,QStringList> cmdHashMHX;
  if(sn_list.size()){
    QHash<QString,QStringList> cmdHash;
    QStringList cmdKeys;
    foreach(QByteArray sn,sn_list){
      NodesItemNode *item = model->nodes.value(sn);
      for(int i=0;i<item->commands.cmd.size();i++){
        if(item->commands.cmd.at(i)<apc_user)continue;
        QString aname=item->commands.descr.at(i);
        QString adata=QByteArray(sn).append((unsigned char)i).toHex();
        if(item->commands.name.at(i)=="mhxfw"){
          cmdHashMHX[aname].append(QByteArray(sn).append((unsigned char)NodesModel::UpgradeMHX).toHex());
        }else if(QMandala::Global::devMode()||(!item->commands.name.at(i).contains("_dev_"))){
          cmdHash[aname].append(adata);
          if(!cmdKeys.contains(aname))cmdKeys.append(aname);
        }
      }
    }
    if(!cmdHash.isEmpty()){
      if(!m.isEmpty())m.addSeparator();
      QHash<QString,QMenu*> groups;
      foreach(QString cname,cmdKeys) {
        QStringList nl=cmdHash.value(cname);
        if(sn_list.size()>1 && nl.size()==1){
          QByteArray sn=QByteArray::fromHex(nl.first().toUtf8().data());
          sn.chop(1);
          cname+=" ("+model->nodes.value(sn)->name+")";
        }
        if(nl.size()>1)cname+=" ["+QString::number(nl.size())+"]";
        QAction *a=new QAction(cname,&m);
        a->setData(QVariant::fromValue(nl));
        if(cname.contains(":")){
          QString gname=cname.left(cname.indexOf(':')).trimmed();
          a->setText(cname.mid(cname.indexOf(':')+1).trimmed());
          QMenu *mGroup=groups.value(gname);
          if(!mGroup){
            mGroup=m.addMenu(gname);
            groups.insert(gname,mGroup);
          }
          mGroup->addAction(a);
        }else m.addAction(a);
        connect(a,SIGNAL(triggered()),this,SLOT(nodeCmdAction()));
      }
    }
  }

  //node system commands
  if(sn_list.size()){
    QHash<QString,QStringList> cmdHash;
    QStringList cmdKeys;
    foreach(QByteArray sn,sn_list){
      NodesItemNode *item = model->nodes.value(sn);
      for(int i=0;i<item->commands.cmd.size();i++){
        if(item->commands.cmd.at(i)>=apc_user)continue;
        QString aname=item->commands.descr.at(i);
        cmdHash[aname].append(QByteArray(sn).append((unsigned char)i).toHex());
        if(!cmdKeys.contains(aname))cmdKeys.append(aname);
      }
    }
    if(!cmdHash.isEmpty()){
      if(!m.isEmpty())m.addSeparator();
      foreach(QString cname,cmdKeys) {
        QStringList nl=cmdHash.value(cname);
        if(nl.size()>1)cname+=" ["+QString::number(nl.size())+"]";
        QAction *a=new QAction(cname,&m);
        a->setData(QVariant::fromValue(nl));
        connect(a,SIGNAL(triggered()),this,SLOT(nodeCmdAction()));
        m.addAction(a);
      }
    }
  }
  //flash update actions
  if(sn_list.size()){
    QHash<QString,QStringList> cmdHash;
    QStringList cmdKeys;
    foreach(QByteArray sn,sn_list){
      NodesItemNode *item = model->nodes.value(sn);
      if(item->name.contains('.'))continue;
      QString aname=tr("Firmware");
      cmdHash[aname].append(QByteArray(sn).append((char)NodesModel::UpgradeFirmware).toHex());
      if(!cmdKeys.contains(aname))cmdKeys.append(aname);
      aname=tr("Loader");
      cmdHash[aname].append(QByteArray(sn).append((char)NodesModel::UpgradeLoader).toHex());
      if(!cmdKeys.contains(aname))cmdKeys.append(aname);
    }
    if(!cmdHash.isEmpty()){
      if(!m.isEmpty())m.addSeparator();
      QMenu *mu=m.addMenu(tr("Update"));

      foreach(QString cname,cmdKeys) {
        QStringList nl=cmdHash.value(cname);
        if(nl.size()>1)cname+=" ["+QString::number(nl.size())+"]";
        QAction *a=new QAction(cname,&m);
        a->setData(QVariant::fromValue(nl));
        a->setEnabled(mandala->isLocal());
        connect(a,SIGNAL(triggered()),this,SLOT(nodeUpdateFirmware()));
        mu->addAction(a);
      }
      foreach(QString cname,cmdHashMHX.keys()) {
        QStringList nl=cmdHashMHX.value(cname);
        if(sn_list.size()>1 && nl.size()==1){
          QByteArray sn=QByteArray::fromHex(nl.first().toUtf8().data());
          sn.chop(1);
          cname+=" ("+model->nodes.value(sn)->name+")";
        }
        if(nl.size()>1)cname+=" ["+QString::number(nl.size())+"]";
        QAction *a=new QAction(cname,&m);
        a->setData(QVariant::fromValue(nl));
        a->setEnabled(mandala->isLocal());
        connect(a,SIGNAL(triggered()),this,SLOT(nodeUpdateFirmware()));
        mu->addAction(a);
      }
    }
  }
  //editor actions
  if(aUndo->isEnabled()){
    if(!m.isEmpty())m.addSeparator();
    m.addAction(aUndo);
  }
  //reset all nodes action
  if(!m.isEmpty())m.addSeparator();
  QAction *a=new QAction(tr("Reboot all nodes"),&m);
  connect(a,SIGNAL(triggered()),this,SLOT(nodeRebootAll()));
  m.addAction(a);

  if(!m.isEmpty())m.exec(tree->mapToGlobal(pos));
}
//=============================================================================
void NodesFrame::nodeCmdAction(void)
{
  QAction *a=(QAction*)sender();
  foreach(QString scmd,a->data().toStringList()) {
    QByteArray ba=QByteArray::fromHex(scmd.toUtf8());
    const QByteArray &sn=ba.left(sizeof(_node_sn));
    uint cmd_idx=(unsigned char)ba.at(ba.size()-1);
    if(!model->nodes.contains(sn))continue;
    NodesItemNode *item = model->nodes.value(sn);
    if((int)cmd_idx>=item->commands.cmd.size())continue;
    qDebug("%s: %s",item->name.toUtf8().data(),a->text().remove('&').toUtf8().data());
    //model->setActive(true);
    model->requestManager.enableTemporary();
    item->command(cmd_idx);
  }
}
//=============================================================================
void NodesFrame::nodeRestoreBackup(void)
{
  QAction *a=(QAction*)sender();
  const QString bname=a->data().toString();
  QList<NodesItem*> sel=selectedItems(NodesItem::it_node);
  if(sel.size()!=1)return;
  static_cast<NodesItemNode*>(sel.first())->restoreBackupFile(bname);
}
void NodesFrame::vehicleRestoreBackup(void)
{
  QAction *a=(QAction*)sender();
  const QString bname=a->data().toString();
  model->restoreVehicleBackup(bname);
}
//=============================================================================
void NodesFrame::nodeRestoreRecentBackup(void)
{
  foreach(NodesItem *i,selectedItems(NodesItem::it_node)){
    QString bname=static_cast<NodesItemNode*>(i)->restoreRecentBackup();
    if(!bname.size())qWarning("%s",tr("Backup not found").toUtf8().data());
  }
}
//=============================================================================
void NodesFrame::nodeRebootAll(void)
{
  QAction *a=(QAction*)sender();
  qDebug("%s...",a->text().toUtf8().data());
  model->mvar->send_srv(apc_reboot,QByteArray());
}
//=============================================================================
void NodesFrame::on_aUndo_triggered(void)
{
  updateActions();
  if(!aUndo->isEnabled())return;
  tree->setFocus();
  foreach(NodesItem *i,selectedItems()){
    if(i->isModified())i->restore();
  }
}
//=============================================================================
void NodesFrame::updateActionsDo(void)
{
  //qDebug()<<"updateActions";
  bool bMod=false;
  foreach(NodesItem *i,selectedItems()){
    if(i->isModified()){
      bMod=true;
      break;
    }
  }
  bool busy=model->requestManager.busy();
  bool upgrading=model->isUpgrading();
  bool bModAll=model->isModified();
  aUndo->setEnabled(bMod);
  aUpload->setEnabled(bModAll && (!(busy)));
  aStop->setEnabled(busy||upgrading);
  aLoad->setEnabled(!(busy || upgrading));
  aSave->setEnabled(!(model->isEmpty()||busy));
  aLoadTelemetry->setEnabled((!(busy || upgrading))&&mandala->local->rec->file.xmlParts.contains("nodes"));
  aReload->setEnabled(!(upgrading||model->isEmpty()));
  aStats->setEnabled(!(model->isEmpty()));
  aClearCache->setEnabled(!(busy));
}
void NodesFrame::updateActions(void)
{
  if(!updateActionsTimer.isActive())
    updateActionsTimer.start();
}
//=============================================================================
void NodesFrame::nodeUpdateFirmware()
{
  mandala->setCurrent(mandala->local);
  resetTree();
  QAction *a=(QAction*)sender();
  QStringList scr;
  QList<NodesItemNode*>nodesListFirmware,nodesListLoader,nodesListMHX;
  foreach(QString scmd,a->data().toStringList()) {
    QByteArray ba=QByteArray::fromHex(scmd.toUtf8());
    const QByteArray &sn=ba.left(sizeof(_node_sn));
    if(!model->nodes.contains(sn))continue;
    uint cmd_idx=(unsigned char)ba.at(ba.size()-1);
    NodesItemNode *node = model->nodes.value(sn);
    if(cmd_idx==NodesModel::UpgradeFirmware)nodesListFirmware.append(node);
    else if(cmd_idx==NodesModel::UpgradeLoader)nodesListLoader.append(node);
    else if(cmd_idx==NodesModel::UpgradeMHX)nodesListMHX.append(node);
  }
  model->requestManager.enableTemporary();
  if(!nodesListLoader.isEmpty())model->upgradeFirmware(nodesListLoader,NodesModel::UpgradeLoader);
  if(!nodesListFirmware.isEmpty())model->upgradeFirmware(nodesListFirmware,NodesModel::UpgradeFirmware);
  if(!nodesListMHX.isEmpty())model->upgradeFirmware(nodesListMHX,NodesModel::UpgradeMHX);
  updateActions();
}
//=============================================================================
void NodesFrame::on_aUpload_triggered(void)
{
  tree->setFocus();
  model->requestManager.enableTemporary();
  model->upload();
}
//=============================================================================
void NodesFrame::on_aRequest_triggered(void)
{
  proxy.setFilterRegExp(QRegExp());
  model->requestManager.enableTemporary();
  model->sync();
}
//=============================================================================
void NodesFrame::on_aReload_triggered(void)
{
  tree->setFocus();
  model->clear();
  aRequest->trigger();
}
//=============================================================================
void NodesFrame::on_aStats_triggered(void)
{
  proxy.setFilterRegExp(QRegExp());
  tree->setFocus();
  model->requestManager.enableTemporary();
  model->stats();
}
//=============================================================================
void NodesFrame::on_aLoadTelemetry_triggered(void)
{
  proxy.setFilterRegExp(QRegExp());
  tree->setFocus();
  mandala->setCurrent(mandala->local);
  model->loadFromTelemetry();
}
//=============================================================================
void NodesFrame::syncProgress(uint cnt)
{
  updateActions();
  if(!cnt){
    progressCnt=0;
    progressBar->setMaximum(0);
    //qDebug()<<progressBar->value();
  }
  //detect cnt grow
  if(progressCnt==cnt){
    progressBar->setValue(progressBar->value()+1);
  }else{
    progressCnt=cnt;
    if((int)cnt>progressBar->maximum())progressBar->setMaximum(cnt);
    int pos=progressBar->maximum()-cnt;
    int cur=progressBar->value();
    int max=progressBar->maximum();
    if(pos<=cur && cur<max)pos=progressBar->value()+1;
    progressBar->setValue(pos);
  }
}
//=============================================================================
void NodesFrame::on_aSave_triggered(void)
{
  QFileDialog dlg(this,aSave->toolTip(),QMandala::Global::uav().canonicalPath());
  dlg.setAcceptMode(QFileDialog::AcceptSave);
  dlg.setOption(QFileDialog::DontConfirmOverwrite,false);
  if(!model->title().isEmpty())
    dlg.selectFile(QMandala::Global::uav().filePath(model->title()+".nodes"));
  QStringList filters;
  filters << tr("Node conf files")+" (*.nodes)"
          << tr("Any files")+" (*)";
  dlg.setNameFilters(filters);
  dlg.setDefaultSuffix("nodes");
  if(!dlg.exec() || dlg.selectedFiles().size()!=1)return;
  model->saveToFile(dlg.selectedFiles().first());
}
//=============================================================================
void NodesFrame::on_aLoad_triggered(void)
{
  proxy.setFilterRegExp(QRegExp());
  QFileDialog dlg(this,aLoad->toolTip(),QMandala::Global::uav().canonicalPath());
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  if(!model->title().isEmpty())
    dlg.selectFile(QMandala::Global::uav().filePath(model->title()+".nodes"));
  QStringList filters;
  filters << tr("Node conf files")+" (*.nodes)"
          << tr("Any files")+" (*)";
  dlg.setNameFilters(filters);
  if(!dlg.exec() || dlg.selectedFiles().size()!=1)return;
  model->loadFromFile(dlg.selectedFiles().first());
}
//=============================================================================
