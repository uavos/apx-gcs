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
#include "FactDelegateDialog.h"
#include "SvgIcon.h"
#include <Nodes.h>
//=============================================================================
QHash<QString,FactDelegateDialog*> FactDelegateDialog::dlgMap;
//=============================================================================
FactDelegateDialog::FactDelegateDialog(Fact *fact, QWidget *parent)
 : QDialog(parent),fact(fact),widget(NULL)
{
  setObjectName(fact->title());
  //setWindowFlags(Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);
  setWindowTitle((fact->descr().size()?fact->descr():fact->title())+QString(" (%1)").arg(fact->titlePath()));

  vlayout=new QVBoxLayout(this);
  vlayout->setMargin(0);
  vlayout->setSpacing(0);

  toolBar=new QToolBar(this);
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  //toolBar->setIconSize(QSize(14,14));
  //toolBar->layout()->setMargin(0);

  NodeField *nodeField=qobject_cast<NodeField*>(fact->child(0));
  if(!nodeField) nodeField=qobject_cast<NodeField*>(fact);
  if(nodeField){
    Nodes *nodes=nodeField->node->nodes;
    aUpload=new QAction(SvgIcon(":/icons/sets/ionicons/android-upload.svg"),tr("Upload"),this);
    connect(aUpload,&QAction::triggered,nodes->f_upload,&Fact::trigger);
    connect(nodes->f_upload,&Fact::enabledChanged,this,[=](){
      NodeField *nf=qobject_cast<NodeField*>(fact->child(0));
      if(!nf) nf=qobject_cast<NodeField*>(fact);
      aUpload->setEnabled(nf && nf->node->nodes->f_upload->enabled());
    });
    aUpload->setEnabled(nodes->f_upload->enabled());
    toolBar->addAction(aUpload);
    toolBar->widgetForAction(aUpload)->setObjectName("greenAction");
  }

  aUndo=new QAction(SvgIcon(":/icons/sets/ionicons/ios-undo.svg"),tr("Undo"),this);
  connect(aUndo,&QAction::triggered,fact,&Fact::restore);
  connect(fact,&Fact::modifiedChanged,this,[=](){
    aUndo->setEnabled(fact->modified());
  });
  aUndo->setEnabled(fact->modified());
  toolBar->addAction(aUndo);

  toolBar->addSeparator();

  aSep=toolBar->addSeparator();



  //toolbar spacer
  /*QWidget* spacer = new QWidget();
  spacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
  toolBar->addWidget(spacer);*/

  //close button
  QAction *aClose=new QAction(SvgIcon(":/icons/sets/ionicons/android-close.svg"),tr("Close"),this);
  connect(aClose,&QAction::triggered,[=](){
    if(aboutToClose()) accept();
  });
  toolBar->addAction(aClose);
  toolBar->widgetForAction(aClose)->setObjectName("redAction");

  /*QAction *a=new QAction(SvgIcon(":/icons/sets/ionicons/android-close.svg"),tr("Close"),this);
  connect(a,&QAction::triggered,[=](){
    if(aboutToClose()) accept();
  });
  toolBar->addAction(a);*/

  setLayout(vlayout);
  vlayout->addWidget(toolBar);

  doRestoreGeometry();
  connect(this,&FactDelegateDialog::finished,this,&FactDelegateDialog::doSaveGeometry);
  connect(this,&FactDelegateDialog::finished,this,&FactDelegateDialog::deleteLater);
  connect(fact,&Fact::removed,this,&FactDelegateDialog::deleteLater);
  connect(fact,&Fact::destroyed,this,&FactDelegateDialog::deleteLater);
}
FactDelegateDialog::~FactDelegateDialog()
{
  //qDebug()<<"delete FactDelegateDialog";
  dlgMap.remove(dlgMap.key(this));
}
//=============================================================================
void FactDelegateDialog::addAction(QAction *a)
{
  toolBar->insertAction(aSep,a);
}
//=============================================================================
void FactDelegateDialog::setWidget(QWidget *w)
{
  QSizePolicy sp=w->sizePolicy();
  sp.setVerticalPolicy(QSizePolicy::Expanding);
  w->setSizePolicy(sp);
  //vlayout->insertWidget(0,w);
  vlayout->addWidget(w);
  widget=w;

  QString s=fact->titlePath();
  if(dlgMap.contains(s)){
    FactDelegateDialog *dlg=dlgMap.value(s);
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
    deleteLater();
    return;
  }
  dlgMap.insert(s,this);

  doRestoreGeometry();
  show();
}
//=============================================================================
void FactDelegateDialog::closeEvent(QCloseEvent * event)
{
  doSaveGeometry();
  if(!aboutToClose()){
    event->ignore();
    return;
  }
  QDialog::closeEvent(event);
}
//=============================================================================
void FactDelegateDialog::doSaveGeometry()
{
  QSettings s;
  s.beginGroup("geometry");
  s.setValue(objectName()+"_Geometry",saveGeometry());
  if(widget){
    foreach(QSplitter *w,widget->findChildren<QSplitter*>()){
      s.setValue(objectName()+"_"+w->objectName()+"State",w->saveState());
    }
  }
}
//=============================================================================
void FactDelegateDialog::doRestoreGeometry()
{
  QSettings s;
  s.beginGroup("geometry");
  restoreGeometry(s.value(objectName()+"_Geometry").toByteArray());
  if(widget){
    foreach(QSplitter *w,widget->findChildren<QSplitter*>()){
      w->restoreState(s.value(objectName()+"_"+w->objectName()+"State").toByteArray());
    }
  }
}
//=============================================================================
