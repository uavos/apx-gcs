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
#include "Report.h"
#include "QMandala.h"
#include <QtCore>
#include <QtWidgets>
#include <QtWebKit>
#include <QtWebKitWidgets>
//=============================================================================
Report::Report(QMap<QString,QString> *data,QWidget *parent) :
    QDialog(parent),data(data)
{
  setupUi(this);
  resize(800,590);
  connect(webView,SIGNAL(loadFinished(bool)),this,SLOT(loadFinished(bool)));
  webView->load(QUrl().fromLocalFile(QMandala::Global::config().filePath("report.html")));

  QToolBar *toolBar=new QToolBar(this);
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  toolBar->setIconSize(QSize(16,16));
  toolBar->layout()->setMargin(0);
  verticalLayout->insertWidget(0,toolBar);

  toolBar->addAction(aSavePDF);
  //toolBar->addAction(aSaveHTML);
  toolBar->addAction(aPrint);
  toolBar->addAction(aEditTemplate);
}
//=============================================================================
//=============================================================================
void Report::loadFinished(bool ok)
{
  if(!ok)return;
  const QWebFrame &f=*webView->page()->mainFrame();
  foreach(QString key,data->keys()){
    QString v=data->value(key,"N/A");
    QWebElementCollection elements=f.findAllElements("#"+key);
    foreach(QWebElement e,elements){
      e.setAttribute("value",v);
      e.setPlainText(v);
    }
  }
}
//=============================================================================
//=============================================================================
void Report::on_aSavePDF_triggered()
{
  QFileDialog dlg(this,aSavePDF->toolTip(),QSettings().value("savePDFFileDir").toString());
  dlg.setAcceptMode(QFileDialog::AcceptSave);
  dlg.setOption(QFileDialog::DontConfirmOverwrite,false);
  dlg.selectFile(data->value("file","report")+".pdf");
  if(!dlg.exec() || !dlg.selectedFiles().size())return;
  /*QPrinter printer;
  printer.setOutputFormat(QPrinter::PdfFormat);
  printer.setOutputFileName(dlg.selectedFiles().at(0));
  webView->print(&printer);
  QSettings().setValue("savePDFFileDir",QFileInfo(printer.outputFileName()).absolutePath());*/
}
//=============================================================================
void Report::on_aPrint_triggered()
{
  /*QPrinter printer;
  printer.setOutputFormat(QPrinter::NativeFormat);
  QPrintDialog *dialog = new QPrintDialog(&printer, this);
  if(dialog->exec()!=QDialog::Accepted) return;
  webView->print(&printer);*/
}
//=============================================================================
void Report::on_aEditTemplate_triggered()
{
  QProcess::startDetached("kate",QStringList()<<QMandala::Global::config().filePath("report.html"));
}
//=============================================================================
//=============================================================================
//=============================================================================

