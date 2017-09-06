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
#ifndef CONFIG_H
#define CONFIG_H
//=============================================================================
#include <QDialog>
#include <QtWidgets>
#include "ui_Config.h"
//=============================================================================
class Config : public QDialog, public Ui::ConfigDlg
{
  Q_OBJECT
public:
  Config(QWidget *parent = 0);
  static void defaults(bool force=true);
private:
  void load();
  void save();
  QMap <QString,QComboBox*> plugins;
private slots:
  void on_buttonBox_clicked(QAbstractButton *button);
  void on_eLang_currentIndexChanged(const QString &text);
};
//=============================================================================
#endif // CTRFRAME_H
