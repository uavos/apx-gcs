/*
 * Copyright (C) 2013 Kapacheuski Yury <ky@uavos.com>
 *
 */
#include "ServosPlugin.h"
#include "QMandala.h"
#include <QtWidgets>
//=============================================================================
void ServosPlugin::init(void)
{
  fobj = NULL;
  aShow = new QAction(QIcon(":/icons/old/bt.png"),tr("Generic Servo config"), this);
  connect(aShow, SIGNAL(triggered()), this, SLOT(showDlg()));
  obj = aShow;
}
//=============================================================================
void ServosPlugin::showDlg(void)
{
  if(fobj){
    fobj->show();
    fobj->raise();
    return;
  }
  fobj = new ServosForm();
  connect(fobj, SIGNAL(finished()), this, SLOT(dlgClosed()));
  QString stitle=aShow->text();
  stitle.remove('&');
  fobj->setWindowTitle(stitle);
  fobj->show();
}
//=============================================================================
void ServosPlugin::dlgClosed()
{
  fobj->deleteLater();
  fobj=NULL;
}
//=============================================================================
