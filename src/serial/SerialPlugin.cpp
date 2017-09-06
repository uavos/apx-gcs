/*
 * Copyright (C) 2013 Kapacheuski Yury <ky@uavos.com>
 *
 */
#include "SerialPlugin.h"
#include "QMandala.h"
#include <QtWidgets>
//=============================================================================
void SerialPlugin::init(void)
{
  fobj = NULL;
  aShow = new QAction(QIcon(":/icons/old/bt.png"),tr("Serial Port Console"), this);
  connect(aShow, SIGNAL(triggered()), this, SLOT(showDlg()));
  obj = aShow;
}
//=============================================================================
void SerialPlugin::showDlg(void)
{
  if(fobj){
    fobj->show();
    fobj->raise();
    return;
  }
  fobj = new SerialForm();
  connect(fobj, SIGNAL(finished()), this, SLOT(dlgClosed()));
  fobj->show();
}
//=============================================================================
void SerialPlugin::dlgClosed()
{
  fobj->deleteLater();
  fobj=NULL;
}
//=============================================================================
