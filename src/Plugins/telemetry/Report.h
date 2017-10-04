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
#ifndef REPORT_H
#define REPORT_H
//=============================================================================
#include <QWidget>
#include "ui_Report.h"
//=============================================================================
class Report : public QDialog, public Ui::Report
{
  Q_OBJECT
public:
  explicit Report(QMap<QString,QString> *data, QWidget *parent = 0);
private:
  QMap<QString,QString> *data;
signals:
private slots:
  void loadFinished(bool ok);
  void on_aSavePDF_triggered();
  void on_aPrint_triggered();
  void on_aEditTemplate_triggered();
public slots:
};
//=============================================================================
#endif // REPORT_H
