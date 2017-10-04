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
#ifndef VALUEEDITOR_H
#define VALUEEDITOR_H
#include <QtWidgets>
#include "MissionModel.h"
class ValueEditorDialog;
//=============================================================================
class ValueEditor: public QPushButton
{
  Q_OBJECT
public:
  explicit ValueEditor(MissionItem *field,QWidget *parent = 0);
  virtual bool aboutToUpload(void){return true;}
  virtual bool aboutToClose(void){return true;}
protected:
  MissionItem *field;
  QAction *aUpload;
  virtual QWidget * getWidget(QWidget *parent);
private:
  ValueEditorDialog *dlg;
private slots:
  void showEditor();
  void aUpload_triggered();
  void aClose_triggered();
signals:
  void upload();
};
//=============================================================================
class ValueEditorDialog: public QDialog
{
  Q_OBJECT
public:
  explicit ValueEditorDialog(ValueEditor * parent, Qt::WindowFlags f = 0)
    :QDialog(parent,f),ve(parent){}
  void closeEvent ( QCloseEvent * event ) ;
  QWidget *widget;
private:
  ValueEditor *ve;
public slots:
  void doSaveGeometry();
  void doRestoreGeometry();
};
//=============================================================================
#endif //VALUEEDITOR_H
