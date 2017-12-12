#include "NodesFrame.h"
#include <AppDirs.h>
#include <Facts.h>
#include <node.h>
#include <SvgIcon.h>
//=============================================================================
NodesFrame::NodesFrame(QWidget *parent) :
  QWidget(parent)
{
  //setupUi(this);
  setWindowTitle(tr("Vehicle parameters"));
  setWindowFlags(Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);

  vlayout=new QVBoxLayout(this);
  setLayout(vlayout);
  vlayout->setMargin(0);
  vlayout->setSpacing(0);

  toolBar=new QToolBar(this);
  toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

  aUpload=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/android-upload.svg"),tr("Upload"),this,&NodesFrame::aUpload_triggered);
  toolBar->widgetForAction(aUpload)->setObjectName("greenAction");
  //static_cast<QToolButton*>(toolBar->widgetForAction(aUpload))->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

  aRequest=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/android-download.svg"),tr("Request parameters"),this,&NodesFrame::aRequest_triggered);
  aStats=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/stats-bars.svg"),tr("Request statistics"),this,&NodesFrame::aStats_triggered);
  aStop=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/android-cancel.svg"),tr("Stop downloading"),this,&NodesFrame::aStop_triggered);
  aReload=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/android-refresh.svg"),tr("Reload everything"),this,&NodesFrame::aReload_triggered);
  aLoad=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/android-folder-open.svg"),tr("Load from file"),this,&NodesFrame::aLoad_triggered);
  aSave=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/compose.svg"),tr("Save to file"),this,&NodesFrame::aSave_triggered);
  aUndo=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/ios-undo.svg"),tr("Revert"),this,&NodesFrame::aUndo_triggered);
  aLoadTelemetry=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/code-download.svg"),tr("Load from telemetry"),this,&NodesFrame::aLoadTelemetry_triggered);


  vlayout->addWidget(toolBar);
  lbUavName=new ClickableLabel(this);
  vlayout->addWidget(lbUavName);

  treeWidget=new FactTreeWidget(FactSystem::instance(),true,false,this);
  vlayout->addWidget(treeWidget);
  connect(treeWidget->tree,&FactTreeView::customContextMenuRequested,this,&NodesFrame::treeContextMenu);

  connect(treeWidget->tree->selectionModel(),&QItemSelectionModel::selectionChanged,this,&NodesFrame::updateActions);

  connect(lbUavName,&ClickableLabel::clicked,Vehicles::instance(),&Vehicles::selectNext);
  connect(Vehicles::instance(),&Vehicles::vehicleSelected,this,&NodesFrame::vehicleSelected);
  vehicleSelected(Vehicles::instance()->current());

  connect(Vehicles::instance()->f_local->f_recorder,&VehicleRecorder::fileLoaded,this,&NodesFrame::updateActions);

  restoreGeometry(QSettings().value(objectName()).toByteArray());
}
//=============================================================================
void NodesFrame::vehicleSelected(Vehicle *v)
{
  vehicle=v;
  Nodes *fNodes=v->f_nodes;
  treeWidget->setRoot(fNodes->f_list);
  lbUavName->setText(v->title());
  lbUavName->setToolTip(QString("squawk: %1").arg(v->squawk(),4,16,QChar('0')).toUpper());
  connect(fNodes,&Nodes::actionsUpdated,this,&NodesFrame::updateActions,Qt::UniqueConnection);
  updateActions();
}
//=============================================================================
void NodesFrame::updateActions(void)
{

  bool bMod=vehicle->f_nodes->modified();
  if(!bMod){
    foreach(Fact *i,selectedItems()){
      if(i->modified()){
        bMod=true;
        break;
      }
    }
  }
  bool busy=false;//model->requestManager.busy();
  bool upgrading=false;//model->isUpgrading();
  bool bEmpty=treeWidget->rootFact()->size()<=0;
  bool bTelemetry=Vehicles::instance()->f_local->f_recorder->file.xmlParts.contains("nodes");
  aUndo->setEnabled(bMod);
  aLoad->setEnabled(!(busy || upgrading));
  aSave->setEnabled(!(bEmpty||busy));
  aLoadTelemetry->setEnabled((!(busy || upgrading)) && bTelemetry);
  aStats->setEnabled(!(bEmpty));
  //aClearCache->setEnabled(!(busy));

  aReload->setEnabled(vehicle->f_nodes->f_reload->enabled());
  aUpload->setEnabled(vehicle->f_nodes->f_upload->enabled());
  aStop->setEnabled(vehicle->f_nodes->f_stop->enabled());
}
//=============================================================================
/*void NodesFrame::currentMandalaChanged(QMandalaItem *m)
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
}*/
//=============================================================================
void NodesFrame::treeContextMenu(const QPoint &pos)
{
  //scan selected items
  QList<QByteArray> sn_list;
  NodesList nlist=selectedItems<NodeItem>();
  foreach(NodeItem *node,nlist){
    sn_list.append(node->sn);
  }
  QMenu m(treeWidget);

  //node backups
  if(nlist.size()==1){
    NodesDB::NodeDataKeys bkeys=nlist.at(0)->nodes->db->nodeDataReadKeys(nlist.at(0));
    if(!bkeys.isEmpty()){
      if(!m.isEmpty())m.addSeparator();
      QMenu *mu=m.addMenu(tr("Backups"));
      for(int i=0;i<bkeys.size();++i){
        QString bname=bkeys.at(i).first;
        QAction *a=new QAction(bname,&m);
        connect(a,&QAction::triggered,this,&NodesFrame::nodeRestoreBackup);
        a->setData(QVariant::fromValue(bkeys.at(i).second));
        mu->addAction(a);
      }
    }

    /*if(item->valid() && item->backup_dir.count()){
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
    }*/
  }
  //recent backup
  if(!nlist.isEmpty()){
    foreach(NodeItem *item, nlist){
      /*if(item->isValid()&&item->backup_dir.count()){
        QAction *a=new QAction(tr("Restore recent backup"),&m);
        connect(a,SIGNAL(triggered()),this,SLOT(nodeRestoreRecentBackup()));
        m.addAction(a);
        break;
      }*/
    }
  }
  //vehicle backups
  /*if((!vehicle->isLocal()) && model->isValid() && model->backup_dir.count()){
    QMenu *mu=m.addMenu(QString("%1 (%2)").arg(tr("Vehicle backups")).arg(model->mvar->ident.callsign));
    QStringList lst=model->backup_dir.entryList();
    for(int i=0;i<lst.size();i++){
      QString bname=lst.at(lst.size()-1-i);
      QAction *a=new QAction(bname.left(bname.lastIndexOf('.')),&m);
      connect(a,SIGNAL(triggered()),this,SLOT(vehicleRestoreBackup()));
      a->setData(QVariant::fromValue(bname));
      mu->addAction(a);
    }
  }*/
  //node specific commands
  QHash<QString,QStringList> cmdHashMHX;
  if(sn_list.size()){
    QHash<QString,QStringList> cmdHash;
    QStringList cmdKeys;
    foreach(QByteArray sn,sn_list){
      NodeItem *item = vehicle->f_nodes->node(sn);
      for(int i=0;i<item->commands.cmd.size();i++){
        if(item->commands.cmd.at(i)<apc_user)continue;
        QString aname=item->commands.descr.at(i);
        QString adata=QByteArray(sn).append((unsigned char)i).toHex();
        if(item->commands.name.at(i)=="mhxfw"){
          cmdHashMHX[aname].append(QByteArray(sn).append((unsigned char)Nodes::UpgradeMHX).toHex());
        }else if(FactSystem::devMode()||(!item->commands.name.at(i).contains("_dev_"))){
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
          cname+=" ("+vehicle->f_nodes->node(sn)->title()+")";
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
      NodeItem *item = vehicle->f_nodes->node(sn);
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
      NodeItem *item = vehicle->f_nodes->node(sn);
      if(item->title().contains('.'))continue;
      QString aname=tr("Firmware");
      cmdHash[aname].append(QByteArray(sn).append((char)Nodes::UpgradeFirmware).toHex());
      if(!cmdKeys.contains(aname))cmdKeys.append(aname);
      aname=tr("Loader");
      cmdHash[aname].append(QByteArray(sn).append((char)Nodes::UpgradeLoader).toHex());
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
        a->setEnabled(vehicle->isLocal());
        connect(a,SIGNAL(triggered()),this,SLOT(nodeUpdateFirmware()));
        mu->addAction(a);
      }
      foreach(QString cname,cmdHashMHX.keys()) {
        QStringList nl=cmdHashMHX.value(cname);
        if(sn_list.size()>1 && nl.size()==1){
          QByteArray sn=QByteArray::fromHex(nl.first().toUtf8().data());
          sn.chop(1);
          cname+=" ("+vehicle->f_nodes->node(sn)->title()+")";
        }
        if(nl.size()>1)cname+=" ["+QString::number(nl.size())+"]";
        QAction *a=new QAction(cname,&m);
        a->setData(QVariant::fromValue(nl));
        a->setEnabled(vehicle->isLocal());
        connect(a,SIGNAL(triggered()),this,SLOT(nodeUpdateFirmware()));
        mu->addAction(a);
      }
    }
  }
  //editor actions
  if(aUndo->isEnabled()){
    foreach(Fact *i, selectedItems()){
      if(i->modified()){
        if(!m.isEmpty())m.addSeparator();
        m.addAction(aUndo);
        break;
      }
    }
  }
  //reset all nodes action
  if(!m.isEmpty())m.addSeparator();
  QAction *a=new QAction(tr("Reboot all nodes"),&m);
  connect(a,SIGNAL(triggered()),this,SLOT(nodeRebootAll()));
  m.addAction(a);

  if(!m.isEmpty())m.exec(treeWidget->tree->mapToGlobal(pos));
}
//=============================================================================
void NodesFrame::nodeCmdAction(void)
{
  QAction *a=(QAction*)sender();
  foreach(QString scmd,a->data().toStringList()) {
    QByteArray ba=QByteArray::fromHex(scmd.toUtf8());
    const QByteArray &sn=ba.left(sizeof(_node_sn));
    uint cmd_idx=(unsigned char)ba.at(ba.size()-1);
    NodeItem *item = vehicle->f_nodes->node(sn);
    if(!item)continue;
    if((int)cmd_idx>=item->commands.cmd.size())continue;
    qDebug("%s: %s",item->title().toUtf8().data(),a->text().remove('&').toUtf8().data());
    //model->setActive(true);
    //model->requestManager.enableTemporary();
    item->cmdexec(cmd_idx);
  }
}
//=============================================================================
void NodesFrame::nodeRestoreBackup(void)
{
  QAction *a=(QAction*)sender();
  NodesList nlist=selectedItems<NodeItem>();
  if(nlist.size()!=1)return;
  nlist.first()->nodes->db->nodeDataRead(nlist.first(),a->data().toULongLong());
}
void NodesFrame::vehicleRestoreBackup(void)
{
  /*QAction *a=(QAction*)sender();
  const QString bname=a->data().toString();
  model->restoreVehicleBackup(bname);*/
}
//=============================================================================
void NodesFrame::nodeRestoreRecentBackup(void)
{
  /*foreach(NodesItem *i,selectedItems(NodesItem::it_node)){
    QString bname=static_cast<NodeItem*>(i)->restoreRecentBackup();
    if(!bname.size())qWarning("%s",tr("Backup not found").toUtf8().data());
  }*/
}
//=============================================================================
void NodesFrame::nodeRebootAll(void)
{
  QAction *a=(QAction*)sender();
  qDebug("%s...",a->text().toUtf8().data());
  vehicle->nmtManager->request(apc_reboot,QByteArray(),QByteArray(),0,true);
}
//=============================================================================
void NodesFrame::nodeUpdateFirmware()
{
  /*mandala->setCurrent(mandala->local);
  resetTree();
  QAction *a=(QAction*)sender();
  QStringList scr;
  QList<NodeItem*>nodesListFirmware,nodesListLoader,nodesListMHX;
  foreach(QString scmd,a->data().toStringList()) {
    QByteArray ba=QByteArray::fromHex(scmd.toUtf8());
    const QByteArray &sn=ba.left(sizeof(_node_sn));
    if(!model->nodes.contains(sn))continue;
    uint cmd_idx=(unsigned char)ba.at(ba.size()-1);
    NodeItem *node = vehicle->f_nodes->node(sn);
    if(cmd_idx==NodesModel::UpgradeFirmware)nodesListFirmware.append(node);
    else if(cmd_idx==NodesModel::UpgradeLoader)nodesListLoader.append(node);
    else if(cmd_idx==NodesModel::UpgradeMHX)nodesListMHX.append(node);
  }
  model->requestManager.enableTemporary();
  if(!nodesListLoader.isEmpty())model->upgradeFirmware(nodesListLoader,NodesModel::UpgradeLoader);
  if(!nodesListFirmware.isEmpty())model->upgradeFirmware(nodesListFirmware,NodesModel::UpgradeFirmware);
  if(!nodesListMHX.isEmpty())model->upgradeFirmware(nodesListMHX,NodesModel::UpgradeMHX);
  updateActions();*/
}
//=============================================================================
//=============================================================================
void NodesFrame::aRequest_triggered(void)
{
  treeWidget->resetFilter();
  vehicle->f_nodes->f_request->trigger();
}
//=============================================================================
void NodesFrame::aReload_triggered(void)
{
  treeWidget->resetFilter();
  vehicle->f_nodes->f_reload->trigger();
}
//=============================================================================
void NodesFrame::aUpload_triggered(void)
{
  treeWidget->tree->setFocus();
  vehicle->f_nodes->f_upload->trigger();
}
//=============================================================================
void NodesFrame::aStop_triggered(void)
{
  treeWidget->tree->setFocus();
  vehicle->f_nodes->f_stop->trigger();
}
//=============================================================================
void NodesFrame::aUndo_triggered(void)
{
  updateActions();
  if(!aUndo->isEnabled())return;
  treeWidget->tree->setFocus();
  foreach(Fact *f,selectedItems()){
    if(f->modified())f->restore();
  }
}
//=============================================================================
void NodesFrame::aStats_triggered(void)
{
  /*proxy.setFilterRegExp(QRegExp());
  tree->setFocus();
  model->requestManager.enableTemporary();
  model->stats();*/
}
//=============================================================================
void NodesFrame::aLoadTelemetry_triggered(void)
{
  treeWidget->resetFilter();
  QDomDocument doc;
  if(!Vehicles::instance()->f_local->f_recorder->file.xmlParts.value("nodes").isEmpty())
    doc.setContent(Vehicles::instance()->f_local->f_recorder->file.xmlParts.value("nodes").values().first());
  if(doc.documentElement().nodeName()!="nodes")return;
  Vehicles::instance()->selectVehicle(Vehicles::instance()->f_local);
  vehicle->f_nodes->clear();
  vehicle->f_nodes->xml->read(doc.documentElement());
}
//=============================================================================
void NodesFrame::aSave_triggered(void)
{
  treeWidget->resetFilter();
  if(!AppDirs::configs().exists()) AppDirs::configs().mkpath(".");
  QFileDialog dlg(this,aSave->toolTip(),AppDirs::configs().canonicalPath());
  dlg.setAcceptMode(QFileDialog::AcceptSave);
  dlg.setOption(QFileDialog::DontConfirmOverwrite,false);
  QStringList filters;
  filters << tr("Node conf files")+" (*.nodes)"
          << tr("Any files")+" (*)";
  dlg.setNameFilters(filters);
  dlg.setDefaultSuffix("nodes");
  dlg.selectFile(AppDirs::configs().filePath(vehicle->fileTitle()+".nodes"));
  if(!dlg.exec() || dlg.selectedFiles().size()!=1)return;

  QString fname=dlg.selectedFiles().first();
  QFile file(fname);
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    qWarning("%s",QString(tr("Cannot write file")+" %1:\n%2.").arg(fname).arg(file.errorString()).toUtf8().data());
    return;
  }
  QTextStream stream(&file);
  vehicle->f_nodes->xml->write().save(stream,2);
  file.close();
}
//=============================================================================
void NodesFrame::aLoad_triggered(void)
{
  treeWidget->resetFilter();
  if(!AppDirs::configs().exists()) AppDirs::configs().mkpath(".");
  QFileDialog dlg(this,aLoad->toolTip(),AppDirs::configs().canonicalPath());
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  dlg.setFileMode(QFileDialog::ExistingFile);
  QStringList filters;
  filters << tr("Node conf files")+" (*.nodes)"
          << tr("Any files")+" (*)";
  dlg.setNameFilters(filters);
  if(vehicle->f_nodes->f_list->size()>0)
    dlg.selectFile(vehicle->fileTitle()+".nodes");
  if(!dlg.exec() || dlg.selectedFiles().size()!=1)return;

  QString fname=dlg.selectedFiles().first();
  QFile file(fname);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    qWarning("%s",QString(tr("Cannot read file")+" %1:\n%2.").arg(fname).arg(file.errorString()).toUtf8().data());
    return;
  }
  QDomDocument doc;
  if(!doc.setContent(&file)){
    qWarning("%s",QString(tr("The file format is not correct")).toUtf8().data());
    file.close();
    return;
  }
  file.close();

  int icnt=vehicle->f_nodes->xml->read(doc.documentElement());
  int nsz=vehicle->f_nodes->snMap.size();
  if(nsz>0 && icnt!=nsz){
    qDebug("%s",tr("Loaded %1 nodes of %2. Importing...").arg(icnt).arg(nsz).toUtf8().data());
    icnt=vehicle->f_nodes->xml->import(doc.documentElement());
    if(icnt>0){
      qDebug("%s",tr("Imported %1 nodes of %2").arg(icnt).arg(nsz).toUtf8().data());
    }else{
      qDebug("%s",tr("Nodes didn't match, nothing to import").toUtf8().data());
    }
  }
}
//=============================================================================
