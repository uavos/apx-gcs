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
#include "ValueEditor.h"
//=============================================================================
ValueEditor::ValueEditor(NodesItem *field, QWidget *parent)
:QPushButton(parent),field(field)
{
  setFlat(true);
  QPalette newPalette = palette();
  newPalette.setBrush(QPalette::Window, QBrush(QColor(255,255,255,80)));
  setPalette(newPalette);
  setBackgroundRole(QPalette::Window);
  connect(this,SIGNAL(clicked()),SLOT(showEditor()));
}
//=============================================================================
void ValueEditor::showEditor()
{
  dlg=new ValueEditorDialog(this);
  dlg->setObjectName(field->name);
  dlg->setWindowFlags(Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);
  dlg->setWindowTitle(field->descr.size()?field->descr:field->name);

  QVBoxLayout *verticalLayout;
  QHBoxLayout *horizontalLayout;
  QSpacerItem *horizontalSpacer;
  //QSpacerItem *verticalSpacer;
  QDialogButtonBox *buttonBox;

  verticalLayout = new QVBoxLayout(dlg);
  verticalLayout->setSpacing(0);
  verticalLayout->setContentsMargins(0, 0, 0, 0);
  dlg->widget=getWidget(dlg);
  if(dlg->widget)verticalLayout->addWidget(dlg->widget);

  //verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
  //verticalLayout->addItem(verticalSpacer);

  horizontalLayout = new QHBoxLayout();
  horizontalLayout->setSpacing(0);

  if(NodesModel::aUpload){
    aUpload = NodesModel::aUpload;//new QAction(QIcon(QStringLiteral(":/icons/old/ark_extract.png")),tr("Upload"),dlg);
    //aUpload->setShortcut(QKeySequence(Qt::Key_F5));
    connect(aUpload,SIGNAL(triggered()),dlg->widget,SLOT(setFocus()));
    //connect(aUpload,SIGNAL(triggered()),this,SIGNAL(reqUpload()));
    //connect(aUpload,SIGNAL(triggered()),this,SLOT(aUpload_triggered()));

    QToolButton *btnUpload;
    btnUpload=new QToolButton();
    btnUpload->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btnUpload->setObjectName("uploadButton");
    btnUpload->setDefaultAction(aUpload);
    horizontalLayout->insertWidget(0,btnUpload);
  }

  horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
  horizontalLayout->addItem(horizontalSpacer);

  buttonBox = new QDialogButtonBox(dlg);
  buttonBox->setOrientation(Qt::Horizontal);
  buttonBox->setStandardButtons(QDialogButtonBox::Close);
  //connect(buttonBox,SIGNAL(clicked(QAbstractButton*)),dlg,SLOT(accept()));
  connect(buttonBox,SIGNAL(clicked(QAbstractButton*)),this,SLOT(aClose_triggered()));
  buttonBox->button(QDialogButtonBox::Close)->setObjectName("killButton");
  horizontalLayout->addWidget(buttonBox);
  verticalLayout->addLayout(horizontalLayout);

  dlg->doRestoreGeometry();
  connect(dlg,SIGNAL(finished(int)),dlg,SLOT(doSaveGeometry()));
  dlg->exec();
}
//=============================================================================
QWidget * ValueEditor::getWidget(QWidget *parent)
{
  Q_UNUSED(parent)
  return NULL;
}
//=============================================================================
//=============================================================================
void ValueEditorDialog::closeEvent(QCloseEvent * event)
{
  doSaveGeometry();
  if(!ve->aboutToClose()){
    event->ignore();
    return;
  }
  QDialog::closeEvent(event);
}
//=============================================================================
void ValueEditorDialog::doSaveGeometry()
{
  QSettings s;
  s.beginGroup("geometry");
  s.setValue(objectName()+"_Geometry",saveGeometry());
  foreach(QSplitter *w,widget->findChildren<QSplitter*>()){
    s.setValue(objectName()+"_"+w->objectName()+"State",w->saveState());
  }
}
//=============================================================================
void ValueEditorDialog::doRestoreGeometry()
{
  QSettings s;
  s.beginGroup("geometry");
  restoreGeometry(s.value(objectName()+"_Geometry").toByteArray());
  foreach(QSplitter *w,widget->findChildren<QSplitter*>()){
    w->restoreState(s.value(objectName()+"_"+w->objectName()+"State").toByteArray());
  }
}
//=============================================================================
//=============================================================================
void ValueEditor::aClose_triggered()
{
  if(!aboutToClose())return;
  dlg->accept();
}
//=============================================================================
