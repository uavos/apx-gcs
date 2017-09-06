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
#include "ValueEditorScript.h"
#include "PawnScript.h"
#include "QMandala.h"
#include "QMandala.h"
#include "crc.h"
//=============================================================================
ValueEditorScript::ValueEditorScript(NodesItemField *field, QWidget *parent)
  :ValueEditor(field,parent)
{
  pawn=static_cast<NodesItemField*>(field)->script;
}
ValueEditorScript::~ValueEditorScript()
{
}
//=============================================================================
QWidget * ValueEditorScript::getWidget(QWidget *parent)
{
  QWidget *w=new QWidget(parent);
  setupUi(w);

  QToolBar *toolBar=new QToolBar(w);
  //toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  toolBar->setIconSize(QSize(16,16));
  toolBar->layout()->setMargin(0);
  toolBarLayout->insertWidget(0,toolBar);
  toolBar->addAction(aLoad);
  toolBar->addAction(aSave);
  toolBar->addSeparator();
  toolBar->addAction(aUndo);
  toolBar->addAction(aCompile);
  toolBar->addSeparator();
  toolBar->addAction(aClear);

  editor->setPlainText(field->getValue().toString());

  pawn->getLog();

  label->setText(field->data(NodesItem::tc_value,Qt::DisplayRole).toString());


  connect(aLoad,SIGNAL(triggered()),this,SLOT(aLoad_triggered()));
  connect(aSave,SIGNAL(triggered()),this,SLOT(aSave_triggered()));
  connect(aUndo,SIGNAL(triggered()),this,SLOT(aUndo_triggered()));
  connect(aCompile,SIGNAL(triggered()),this,SLOT(compile()));
  connect(aClear,SIGNAL(triggered()),editor,SLOT(clear()));

  connect(logList,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(logView_itemClicked(QListWidgetItem*)));

  QTimer::singleShot(500,this,SLOT(compile()));
  return w;
}
//=============================================================================
bool ValueEditorScript::aboutToUpload(void)
{
  aCompile->trigger();
  return true;
}
//=============================================================================
bool ValueEditorScript::aboutToClose(void)
{
  aCompile->trigger();
  return true;
}
//=============================================================================
void ValueEditorScript::aUndo_triggered(void)
{
  field->restore();
  editor->setPlainText(field->getValue().toString());
}
//=============================================================================
void ValueEditorScript::aSave_triggered(void)
{
  QFileDialog dlg(this,aSave->toolTip(),QMandala::Global::scr().canonicalPath());
  dlg.setAcceptMode(QFileDialog::AcceptSave);
  dlg.setOption(QFileDialog::DontConfirmOverwrite,false);
  if(!scrName.isEmpty())
    dlg.selectFile(QMandala::Global::scr().filePath(scrName+".p"));
  QStringList filters;
  filters << tr("Script files")+" (*.p)"
          << tr("Any files")+" (*)";
  dlg.setNameFilters(filters);
  if(!dlg.exec() || dlg.selectedFiles().size()!=1)return;
  saveToFile(dlg.selectedFiles().first());
}
//=============================================================================
void ValueEditorScript::aLoad_triggered(void)
{
  QFileDialog dlg(this,aLoad->toolTip(),QMandala::Global::scr().canonicalPath());
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  if(!scrName.isEmpty())
    dlg.selectFile(QMandala::Global::scr().filePath(scrName+".p"));
  QStringList filters;
  filters << tr("Script files")+" (*.p)"
          << tr("Any files")+" (*)";
  dlg.setNameFilters(filters);
  if(!dlg.exec() || dlg.selectedFiles().size()!=1)return;
  loadFromFile(dlg.selectedFiles().first());
}
//=============================================================================
bool ValueEditorScript::compile()
{
  logList->clear();
  editor->cleanText();
  field->setValue(editor->toPlainText());
  label->setText(field->data(NodesItem::tc_value,Qt::DisplayRole).toString());
  uint icnt=0;
  foreach(QString s,pawn->getLog().split("\n",QString::SkipEmptyParts)){
    if(s.startsWith("Pawn"))continue;
    if(s.contains("error")||s.contains("warning")){
      QStringList w=s.split(' ');
      QListWidgetItem *i=new QListWidgetItem(s,logList);
      i->setBackgroundColor(s.contains("warning")?QColor(220,220,200):QColor(220,150,150));
      i->setForeground(Qt::black);
      icnt++;
    }else{
      new QListWidgetItem(s,logList);
      icnt++;
    }
  }
  if(!icnt){
    new QListWidgetItem(tr("Success")+".",logList);
  }

  return true;
}
//=============================================================================
bool ValueEditorScript::saveToFile(QString fname)
{
  QFile file(fname);
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    qWarning("%s",QString(tr("Cannot write file")+" %1:\n%2.").arg(fname).arg(file.errorString()).toUtf8().data());
    return false;
  }
  scrName=QFileInfo(fname).baseName();
  label->setText(scrName);

  editor->cleanText();
  QTextStream s(&file);
  s << editor->toPlainText();
  return true;
}
//=============================================================================
bool ValueEditorScript::loadFromFile(QString fname)
{
  QFile file(fname);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    qWarning("%s",QString(tr("Cannot read file")+" %1:\n%2.").arg(fname).arg(file.errorString()).toUtf8().data());
    return false;
  }
  scrName=QFileInfo(fname).baseName();
  label->setText(scrName);

  QTextStream s(&file);
  editor->setPlainText(s.readAll());
  compile();
  return true;
}
//=============================================================================
void ValueEditorScript::logView_itemClicked(QListWidgetItem *item)
{
  QString s=item->text();
  QRegExp exp("\\((.*)\\)");
  if(exp.indexIn(s.left(s.indexOf(':')))>=0){
    bool ok;
    int line=exp.cap(1).toInt(&ok);
    if((!ok) && exp.cap(1).contains("--"))
      line=exp.cap(1).left(exp.cap(1).indexOf("--")).toInt(&ok);
    if(ok && line>=0){
      QTextCursor text_cursor(editor->document()->findBlockByLineNumber(line-1));
      text_cursor.select(QTextCursor::LineUnderCursor);
      editor->setTextCursor(text_cursor);
    }
  }
}
//=============================================================================
