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
#ifndef CONSOLE_H
#define CONSOLE_H
//=============================================================================
#include <QtCore>
#include <QTextEdit>
//class QMandala;
//=============================================================================
class Console : public  QTextEdit
{
  Q_OBJECT
public:
  Console(QWidget *parent = NULL);

  static QTextCharFormat fPrompt,fUser,fAutocomplete,fResult,fStdOut,fStdErr,fStdWarn,fNode;

private:

  //QMandala *mandala;
  void replaceCurrentCommand(QString newCommand);
  QString getCurrentCommand(void);

  void displayPrompt();

  QString prompt;

  QStringList history;
  int historyIndex;
  QString replacedHistory;

  void wheelEvent ( QWheelEvent * e );

  QString last_msg;
  uint last_msg_cnt;
  QTime last_msg_time;

  //right aligned text
  QTextTableFormat fTable;
  QTextBlockFormat fRight;

  void exec_command(const QString &command); //linux style syntax

  void get_hints(QString *command,QStringList *hints);
protected:
  void keyPressEvent(QKeyEvent* e);
private slots:
  void on_cursorPositionChanged();
  void escPressed();
  void linesLimit();

public slots:
  void message(QString msg,QTextCharFormat fmt=fStdOut);  //on top of prompt
  void setFocus();
  void message_impl(QString msg,QTextCharFormat fmt);
};
Q_DECLARE_METATYPE(QTextCharFormat)
//=============================================================================
#endif
