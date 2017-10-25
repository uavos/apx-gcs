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
#include "Console.h"
#include "FactSystem.h"
//=============================================================================
#include <QtGui>
#include <QApplication>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <cassert>

#include <qtextstream.h>
#include <qsocketnotifier.h>
#include <QTextBlock>
#include <QShortcut>

#define MAX_HISTORY 50
void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
Console *console;
//=============================================================================
QTextCharFormat Console::fPrompt;
QTextCharFormat Console::fUser;
QTextCharFormat Console::fAutocomplete;
QTextCharFormat Console::fResult;
QTextCharFormat Console::fStdOut;
QTextCharFormat Console::fStdErr;
QTextCharFormat Console::fStdWarn;
QTextCharFormat Console::fNode;
//=============================================================================
Console::Console(QWidget *parent)
    : QTextEdit(parent)
{
  setWindowTitle(tr("Console"));

  //mandala=qApp->property("Mandala").value<QMandala*>();

  qRegisterMetaType<QTextCharFormat>();

  //setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Expanding);

  document()->setMaximumBlockCount(10000);

  QFont font;
  //font.setFamily("Ubuntu");
  font.setFixedPitch(true);
  double fontSize=QSettings().value("consoleFontSize",font.pointSize()).toDouble();
  font.setPointSize(fontSize);
  setFont(font);

  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setFrameShape(QFrame::StyledPanel);
  setCursorWidth(4);

  /*QPalette pc(palette());
  pc.setColor(QPalette::Text,QColor(85,170,255,180));
  setPalette(pc);*/

  historyIndex=0;
  //history.clear();
  history=QSettings().value("consoleHistory").toStringList();


  prompt=">";//QCoreApplication::applicationName()+">";

  connect(this,SIGNAL(cursorPositionChanged()),SLOT(on_cursorPositionChanged()));

  fPrompt.setForeground(Qt::lightGray);//Qt::black);
  fPrompt.setFontWeight(QFont::Bold);
  fUser.setForeground(QColor(Qt::cyan).lighter());//QColor(Qt::darkBlue));
  fAutocomplete.setForeground(Qt::cyan);//QColor(Qt::darkBlue));


  fResult.setForeground(Qt::gray);//QColor(Qt::black));
  fStdOut.setForeground(QColor(120,235,120));//QColor(Qt::darkGreen));
  fStdErr.setForeground(QColor(235,120,120));//QColor(Qt::darkRed));
  fStdErr.setFontWeight(QFont::Bold);
  fStdWarn.setForeground(Qt::yellow);//QColor(Qt::darkRed));
  fStdWarn.setFontWeight(QFont::Bold);

  //fNode.setBackground(Qt::black);
  fNode.setForeground(Qt::gray);//QColor(32,32,92));
  //fNode.setFontFixedPitch(true);
  //fNode.setFontUnderline(true);
  //fNode.setFontFamily("Monospace");
  //fNode.setFontPointSize(fontSize*0.8);

  fTable.setBorder(0);
  fTable.setWidth(QTextLength(QTextLength::PercentageLength,100));
  fTable.setCellPadding(0);
  fTable.setCellSpacing(0);
  fRight.setAlignment(Qt::AlignRight|Qt::AlignTop);



  //setStyleSheet("background-color: black; border: none;");
  setStyleSheet("border: none;");

  setAcceptRichText(false);

  console=this;
  qInstallMessageHandler(messageOutput);

  displayPrompt();


  QShortcut *sct=new QShortcut(QKeySequence(Qt::Key_T),this,0,0,Qt::ApplicationShortcut);
  connect(sct,SIGNAL(activated()),this,SLOT(setFocus()));

  QShortcut *sctEsc=new QShortcut(QKeySequence(Qt::Key_Escape),this,0,0,Qt::ApplicationShortcut);
  connect(sctEsc,SIGNAL(activated()),this,SLOT(escPressed()));
}
//=============================================================================
void Console::setFocus()
{
  QTextEdit::setFocus();
  window()->activateWindow();
}
//=============================================================================
void Console::linesLimit()
{
  //printf("blocks: %i\n",document()->blockCount());fflush(stdout);
  //remove blocks...
  const int maxcnt=document()->maximumBlockCount()*0.8;
  if(document()->blockCount()<=maxcnt)return;
  QTextBlock block=document()->begin();
  while(document()->blockCount()>maxcnt){
    if(!block.isValid())break;
    QTextCursor cursor(block);
    block=block.next();
    cursor.select(QTextCursor::BlockUnderCursor);
    cursor.removeSelectedText();
  }
  //printf("blocks: %i\n",document()->blockCount());fflush(stdout);
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
}
//=============================================================================
void Console::on_cursorPositionChanged()
{
  //ensure cursor is within last block after prompt
  QTextCursor c=textCursor();
  if(c.block()==document()->lastBlock() && c.positionInBlock()>=prompt.size())return;
  //if(c.hasSelection())return;
  c.movePosition(QTextCursor::End);
  c.movePosition(QTextCursor::StartOfBlock);
  c.movePosition(QTextCursor::Right,QTextCursor::MoveAnchor,prompt.size());
  setTextCursor(c);
}
//=============================================================================
void Console::message(QString msg,QTextCharFormat fmt)
{
  QMetaObject::invokeMethod(this,"message_impl", Qt::QueuedConnection, Q_ARG(QString, msg), Q_ARG(QTextCharFormat, fmt));
}
//=============================================================================
void Console::message_impl(QString msg,QTextCharFormat fmt)
{
  if(last_msg==msg){
    last_msg_cnt++;
    if(last_msg_cnt>10 && last_msg_time.elapsed()<(int)(500*last_msg_cnt/1000))return;
  }else{
    last_msg_cnt=0;
    last_msg=msg;
  }
  bool bHtml=msg.trimmed().left(6)=="<html>";
  if(last_msg_cnt>=9 && (!bHtml))msg.append(QString(" (%1)").arg(last_msg_cnt+1));
  last_msg_time.start();
  if(msg.startsWith("::"))fmt=fNode; //ToolTip
  else if(msg.contains("error",Qt::CaseInsensitive))fmt=fStdErr;
  else if(msg.contains("fail",Qt::CaseInsensitive))fmt=fStdErr;
  else if(msg.contains("timeout",Qt::CaseInsensitive))fmt=fStdErr;
  else if(msg.contains("warning",Qt::CaseInsensitive))fmt=fStdWarn;
  if ((msg.right(1)!="\n")) msg.append("\n");//msg.remove(msg.size()-1,1);
  QTextCursor c=textCursor();
  if (c.blockNumber()!=(document()->blockCount()-1))c.movePosition(QTextCursor::End);
  int cp=c.positionInBlock();//c.position();
  c.movePosition(QTextCursor::StartOfBlock);
  //int ps=c.position();
  if(bHtml){
    c.insertText(" ");
    c.insertHtml("<font color=white>"+msg+"</font></html>");
  }else{
    /*if(msg.startsWith('{')&&msg.contains('}')){
      int ie=msg.indexOf('}');
      QString n=msg.mid(1,ie-1).trimmed();
      msg=msg.remove(0,ie+1).trimmed();
      c.insertHtml("<font color=white>"+n+"</font></html>");
    }*/
    if(msg.startsWith('[')&&msg.contains(']')){
      int ie=msg.indexOf(']');
      QString n=msg.mid(0,ie+1).trimmed();
      msg=msg.remove(0,ie+1).trimmed();
      //c.beginEditBlock();
      QTextTable *table = c.insertTable(1, 2, fTable);
      //table->cellAt(0,0).firstCursorPosition().insertText(msg,fmt);
      table->cellAt(0,1).firstCursorPosition().setBlockFormat(fRight);
      table->cellAt(0,1).firstCursorPosition().insertText(n,fNode);
      c=table->cellAt(0,0).firstCursorPosition();
      //c.insertBlock();
    }
    c.insertText(msg,fmt);
  }
  c.movePosition(QTextCursor::End);
  c.movePosition(QTextCursor::StartOfBlock);
  c.setPosition(c.block().position()+cp);
  setTextCursor(c);
  linesLimit();
}
//=============================================================================
void Console::replaceCurrentCommand(QString newCommand)
{
  //save edited string..
  QString cmd=getCurrentCommand().simplified();
  if (cmd.size() && !history.contains(cmd))replacedHistory=cmd;
  //replace
  QTextCursor c=textCursor();
  c.clearSelection();
  c.movePosition(QTextCursor::StartOfBlock);
  c.movePosition(QTextCursor::Right,QTextCursor::MoveAnchor,prompt.size());
  c.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
  c.insertText(newCommand);
  linesLimit();
}
//=============================================================================
void Console::keyPressEvent(QKeyEvent* e)
{
  QTextCursor c=textCursor();

  int key=e->key();

  // execute command
  if (key==Qt::Key_Return||key==Qt::Key_Enter) {
    c.movePosition(QTextCursor::End);
    setTextCursor(c);
    QString cmd=getCurrentCommand().simplified();
    //qDebug("cmd: %s",cmd.toUtf8().data());
    if (cmd.size()) {
      //history
      history.removeOne(cmd);
      history.append(cmd);
      while (history.size()>MAX_HISTORY)history.removeFirst();
      historyIndex=history.size();
      replacedHistory="";
      QSettings().setValue("consoleHistory",history);
      // exec command..
      c.insertBlock();
      exec_command(cmd);
    }
    displayPrompt();
    return;
  }

  //prevent delete prompt
  if(c.positionInBlock()<=prompt.size() && key==Qt::Key_Backspace)return;
  if(c.positionInBlock()<prompt.size() && key==Qt::Key_Delete)return;

  //scrolling
  if(e->modifiers()==Qt::ShiftModifier){
    if (key==Qt::Key_Up){
      QWheelEvent w(QPoint(10,10),45,Qt::NoButton,Qt::NoModifier);
      wheelEvent(&w);
      return;
    }else if (key==Qt::Key_Down){
      QWheelEvent w(QPoint(10,10),-45,Qt::NoButton,Qt::NoModifier);
      wheelEvent(&w);
      return;
    }
  }

  // History
  if (key==Qt::Key_Up) {
    if (historyIndex)replaceCurrentCommand(history[--historyIndex]);
    c.movePosition(QTextCursor::End);
    setTextCursor(c);
    return;
  }
  if (key==Qt::Key_Down) {
    if ((historyIndex+1)<history.size())replaceCurrentCommand(history[++historyIndex]);
    else if (replacedHistory.size())replaceCurrentCommand(replacedHistory);
    c.movePosition(QTextCursor::End);
    setTextCursor(c);
    return;
  }

  //completion
  if (key==Qt::Key_Tab){
    if(c.atBlockEnd()) {
      QString command = getCurrentCommand();
      QStringList hints;
      get_hints(&command,&hints);
      if (hints.count() == 1){
        replaceCurrentCommand(hints.first());
      }else if (hints.count() > 1) {
        replaceCurrentCommand(command); //partial autocompletion
        //show options
        c.insertBlock();
        c.insertHtml(hints.join(" "));
        //c.insertText(hints.join(" "),fAutocomplete);
        displayPrompt();
        c.insertText(command);
      }
    }
    return;
  }

  //If Ctrl + C pressed, then undo the current command
  if (key==Qt::Key_C && e->modifiers()==Qt::ControlModifier) {
    displayPrompt();
    return;
  }
  //ignore Ctrl + ..
  if (e->modifiers()==Qt::ControlModifier) {
    return;
  }

  historyIndex=history.size();  //reset history idx on user input

  //  other
  if (key==Qt::Key_Escape) {
    replaceCurrentCommand("");
    return;
  }


  QTextEdit::keyPressEvent(e);
}
//=============================================================================
void Console::get_hints(QString *command,QStringList *hints)
{
  QString cmd=*command;
  QString prefix=cmd;
  hints->clear();
  cmd.remove(0,cmd.lastIndexOf(';')+1);
  if(cmd.endsWith(' '))cmd=cmd.trimmed().append(' ');
  else cmd=cmd.trimmed();

  QString scope="this";
  QJSValue result;
  QRegExp del("[\\ \\,\\:\\t\\{\\}\\[\\]\\(\\)\\=]");
  if(!(cmd.contains(del)||prefix.startsWith('!')||cmd.contains('.'))){
    //first word input (std command?)
    result=FactSystem::instance()->jsexec(QString("(function(){var s='';for(var v in %1)if(typeof(%1[v])=='function')s+=v+';';return s;})()").arg(scope));
    QStringList st(result.toString().replace('\n',',').split(';',QString::SkipEmptyParts));
    st=st.filter(QRegExp("^"+cmd));
    if(st.size()){
      if(st.size()==1) st.append(prefix.left(prefix.size()-cmd.size())+st.takeFirst()+" ");
    }else{
      result=FactSystem::instance()->jsexec(QString("(function(){var s='';for(var v in %1)s+=v+';';return s;})()").arg(scope));
      st=result.toString().replace('\n',',').split(';',QString::SkipEmptyParts);
      st=st.filter(QRegExp("^"+cmd));
      if(st.size()==1) st.append(prefix.left(prefix.size()-cmd.size())+st.takeFirst());
    }
    if(!result.isError()) *hints=st;
  }else{
    //parameter or not a command
    cmd=cmd.remove(0,cmd.lastIndexOf(del)+1).trimmed();
    bool bDot=cmd.contains('.');
    if(bDot){
      scope=cmd.left(cmd.lastIndexOf('.'));
      cmd.remove(0,cmd.lastIndexOf('.')+1);
    }
    result=FactSystem::instance()->jsexec(QString("(function(){var s='';for(var v in %1)s+=v+';';return s;})()").arg(scope));
    QStringList st(result.toString().replace('\n',',').split(';',QString::SkipEmptyParts));
    //QStringList st(FactSystem::instance()->jsexec(QString("var s='';for(var v in %1)s+=v+';';").arg(scope)).toString().replace('\n',',').split(';',QString::SkipEmptyParts));
    st=st.filter(QRegExp("^"+cmd));
    if(st.size()==1)
      st.append(prefix.left(prefix.size()-cmd.size())+st.takeFirst()+(bDot?"":" "));
    if(!result.isError()) *hints=st;

  }
  if(hints->size()<=1)return;
  hints->sort();
  //partial autocompletion
  if(cmd.size()>0){
    //partial autocompletion
    bool bMatch=true;
    QString s=cmd;
    while(bMatch&&s.size()<hints->first().size()){
      s+=hints->first().at(s.size());
      foreach(const QString &hint,*hints){
        if(hint.startsWith(s))continue;
        s.chop(1);
        bMatch=false;
        break;
      }
    }
    if(s!=cmd){
      *command=prefix.left(prefix.size()-cmd.size())+s;
    }
  }
  //hints output formatting
  for(int i=0;i<hints->size();i++){
    if(FactSystem::instance()->jsexec(QString("typeof(%1['%2'])=='function'").arg(scope).arg(hints->at(i))).toBool()){
      (*hints)[i]="<font color='white'><b>"+hints->at(i)+"</b></font>";
    }else if(FactSystem::instance()->jsexec(QString("typeof(%1['%2'])=='object'").arg(scope).arg(hints->at(i))).toBool()){
      (*hints)[i]="<font color='yellow'>"+hints->at(i)+"</font>";
    }else if(FactSystem::instance()->jsexec(QString("typeof(%1['%2'])=='number'").arg(scope).arg(hints->at(i))).toBool()){
      (*hints)[i]="<font color='cyan'>"+hints->at(i)+"</font>";
    }else (*hints)[i]="<font color='gray'>"+hints->at(i)+"</font>";
  }
  hints->sort();
}
//=============================================================================
void Console::exec_command(const QString &command)
{
  QString s=command.simplified();
  if(s.startsWith('!')){
    FactSystem::instance()->jsexec(s.remove(0,1));
    return;
  }
  if(s.contains(';')){
    foreach(const QString &tcmd,s.split(';',QString::SkipEmptyParts))
      exec_command(tcmd);
    return;
  }
  if(s.contains("(")||s.contains("=")){
    FactSystem::instance()->jsexec(command);
    return;
  }
  QStringList st=s.split(' ');
  if(!st.size())return;
  QString sc=st.takeFirst();
  if((sc.startsWith("set")||sc.startsWith("req")||sc.startsWith("send")) && st.size()){
    st.insert(0,"'"+st.takeFirst()+"'"); //quote var name
    FactSystem::instance()->jsexec(sc+"("+st.join(",")+")");
  }else
    FactSystem::instance()->jsexec(sc+"("+st.join(",")+")");
}
//=============================================================================
void Console::escPressed()
{
  clearFocus();
}
//=============================================================================
QString Console::getCurrentCommand(void)
{
  return document()->lastBlock().text().remove(0,prompt.size());
}
//=============================================================================
void Console::wheelEvent ( QWheelEvent * e )
{
  QTextEdit::wheelEvent(e);
}
//=============================================================================
void Console::displayPrompt()
{
  QTextCursor c=textCursor();
  c.movePosition(QTextCursor::End);
  if (!c.atBlockStart())c.insertBlock();

  c.insertText(prompt,fPrompt);
  setTextCursor(c);

  setUndoRedoEnabled(false);
  setUndoRedoEnabled(true);
  document()->setModified(false);

  historyIndex=history.size();

  linesLimit();
}
//=============================================================================
//=============================================================================
void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  Q_UNUSED(context);
  QString s(msg);
  s=s.trimmed();
  if(s.contains("\n")){ //multiline
    QStringList st=s.split("\n");
    //printf("ML: %u",st.size());
    foreach(QString sl,st)
      messageOutput(type, QMessageLogContext(), sl.toUtf8().data());
    return;
  }
  bool bFlt=true;
  while(1){
    if(s.isEmpty())break;
    if(s.startsWith('@')){
      s.remove(0,1);
      break;
    }
    if(s=="''"||s=="\"\""||s=="\"")break;
    if(s.contains("freedesktop",Qt::CaseInsensitive))break;
    if(s.contains("ibus-daemon",Qt::CaseInsensitive))break;
    if(s.contains("QXcb",Qt::CaseInsensitive))break;
    if(s.contains("GTK",Qt::CaseInsensitive))break;
    if(s.contains("sycoca",Qt::CaseInsensitive))break;
    if(s.contains("QSGContext",Qt::CaseInsensitive))break;
    if(s.contains("Theme tree",Qt::CaseInsensitive))break;
    if(s.contains("Invalid URL:",Qt::CaseInsensitive))break;
    if(s.contains("QScreen",Qt::CaseInsensitive))break;
    if(s.contains("Xlib:",Qt::CaseInsensitive))break;
    if(s.contains("linux-gnu",Qt::CaseInsensitive))break;
    bFlt=false;
    break;
  }
  if(bFlt){
    fprintf(stdout, "%s\n", s.toUtf8().data());
    fflush(stdout);
    return;
  }
  switch (type) {
  case QtDebugMsg:
    if(!s.startsWith("<html>"))
      fprintf(stdout, "Debug: %s\n", s.toUtf8().data());
    else {
      console->message(s);
      break;
    }
    if(s.startsWith("<")){
      s.remove(0,1);
      console->message(s,Console::fUser);
    }else{
      if(s.size()>2){
        char c=s.at(0).toLatin1();
        QTextCharFormat fmt=Console::fStdOut;
        switch(c){
          case '#': fmt.setForeground(Qt::gray);break;
          case '!': fmt.setFontWeight(QFont::Bold);break;
          default:
            QStringList flt;
            flt<<"Moving from "<<"State change"<<"About to finish"<<"Transitioning to state "<<"Setting new source"<<"New source:  "<<"Stream changed to ";
            foreach(const QString &sf,flt)
              if(s.startsWith(sf))return;
            console->message(s);
            return;
        }
        console->message(s.remove(0,1),fmt);
      }else console->message(s);
    }
    break;
  case QtWarningMsg:
     fprintf(stderr, "Warning: %s\n", s.toUtf8().data());
     console->message(s,Console::fStdErr);
     break;
  case QtCriticalMsg:
     fprintf(stderr, "Critical: %s\n", s.toUtf8().data());
     console->message(s,Console::fStdErr);
     break;
  case QtFatalMsg:
     fprintf(stderr, "Fatal: %s\n", s.toUtf8().data());
     console->message(s,Console::fStdErr);
     //abort();
     break;
  default:
    break;
  }
}
//============================================================================
