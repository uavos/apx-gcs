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
#include "DatalinkInspector.h"
#include <ApxApp.h>
#include <ApxLog.h>
#include <QDesktopServices>
#include <QQmlEngine>
#define MAX_HISTORY 50
//=============================================================================
static DatalinkInspector *terminal;
static QtMessageHandler messageHandlerChain;
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
//=============================================================================
DatalinkInspector::DatalinkInspector(Fact *parent)
    : Fact(parent,
           PLUGIN_NAME,
           tr("DatalinkInspector"),
           tr("Serial data analyzer"),
           Group,
           "swap-vertical")
{
    setQmlPage("qrc:/" PLUGIN_NAME "/DatalinkInspector.qml");

    _model = new DatalinkInspectorListModel(this);
    connect(this,
            &DatalinkInspector::newMessage,
            _model,
            &DatalinkInspectorListModel::append,
            Qt::QueuedConnection);

    _history = QSettings().value("consoleHistory").toStringList();
    historyReset();

    qmlRegisterUncreatableType<DatalinkInspector>("APX.DatalinkInspector",
                                                  1,
                                                  0,
                                                  "DatalinkInspector",
                                                  "Reference only");
    qmlRegisterUncreatableType<DatalinkInspectorListModel>("APX.DatalinkInspector",
                                                           1,
                                                           0,
                                                           "DatalinkInspector",
                                                           "Reference only");

    terminal = this;
    messageHandlerChain = qInstallMessageHandler(messageHandler);
}
//=============================================================================
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    static QMutex mutex;
    QMutexLocker lock(&mutex);
    messageHandlerChain(type, context, message);
    terminal->handleMessage(type, context, message);
}
//============================================================================
void DatalinkInspector::handleMessage(QtMsgType type,
                                      const QMessageLogContext &context,
                                      const QString &message)
{
    if (!ApxLog::display(context))
        return;
    emit newMessage(type, context.category, message);
    //_model->append(type,context.category,message);
}
//=============================================================================
void DatalinkInspector::exec(const QString &cmd)
{
    QString s = cmd.simplified();
    if (s.isEmpty())
        return;

    if (s.startsWith(';') || s.startsWith('!')) {
        s.remove(0, 1);
    } else if (!(s.contains("(") || s.contains("=") || s.contains("+") || s.contains("-"))) {
        QStringList st = s.split(' ');
        if (!st.size())
            return;
        QString sc = st.takeFirst();
        if ((sc.startsWith("set") || sc.startsWith("req") || sc.startsWith("send")) && st.size()) {
            st.insert(0, "'" + st.takeFirst() + "'"); //quote var name
        }
        s = sc + "(" + st.join(",") + ")";
    }
    QJSValue v = ApxApp::jsexec(s);
    //history
    _history.removeAll(cmd);
    //if(!v.isError())
    _history.append(cmd);
    while (_history.size() > MAX_HISTORY)
        _history.removeFirst();
    QSettings().setValue("consoleHistory", _history);
    historyReset();
    _model->enterResult(!v.isError());
}
//=============================================================================
void DatalinkInspector::historyReset()
{
    _historyIndex = -1;
    _replacedHistory = "";
}
QString DatalinkInspector::historyNext(const QString &cmd)
{
    //qDebug()<<_history<<_historyIndex<<cmd;
    if (_historyIndex < 0) {
        _historyIndex = _history.size();
        _replacedHistory = cmd;
    }

    if (_historyIndex > _history.size()) {
        _historyIndex = 0;
        return cmd;
    }
    if (_replacedHistory.isEmpty()) {
        if (_historyIndex > 0)
            _historyIndex--;
        return _history[_historyIndex].trimmed();
    }
    while (_historyIndex > 0) {
        _historyIndex--;
        QString s = _history[_historyIndex].trimmed();
        if (s.startsWith(_replacedHistory))
            return s;
    }
    return cmd;
}
QString DatalinkInspector::historyPrev(const QString &cmd)
{
    //qDebug()<<_history<<_historyIndex<<cmd;
    if (_historyIndex < 0)
        return cmd;
    if (_replacedHistory.isEmpty()) {
        _historyIndex++;
    } else {
        while (_historyIndex < _history.size()) {
            _historyIndex++;
            if (_historyIndex == _history.size())
                break;
            QString s = _history[_historyIndex].trimmed();
            if (s == cmd)
                continue;
            if (s.startsWith(_replacedHistory))
                break;
        }
    }
    if (_historyIndex >= _history.size()) {
        _historyIndex = _history.size();
        return _replacedHistory;
    }
    return _history[_historyIndex].trimmed();
}
//=============================================================================
QString DatalinkInspector::autocomplete(const QString &cmd)
{
    QStringList hints;
    QString prefix = cmd;
    QString c = cmd;
    hints.clear();
    c.remove(0, c.lastIndexOf(';') + 1);
    if (c.endsWith(' '))
        c = c.trimmed().append(' ');
    else
        c = c.trimmed();

    QString scope = "this";
    QJSValue result;
    QRegExp del("[\\ \\,\\:\\t\\{\\}\\[\\]\\(\\)\\=]");
    if (!(c.contains(del) || prefix.startsWith('!') || c.contains('.'))) {
        //first word input (std command?)
        result = ApxApp::jsexec(QString("(function(){var s='';for(var v in "
                                        "%1)if(typeof(%1[v])=='function')s+=v+';';return s;})()")
                                    .arg(scope));
        QStringList st(result.toString().replace('\n', ',').split(';', QString::SkipEmptyParts));
        st = st.filter(QRegExp("^" + c));
        if (st.size()) {
            if (st.size() == 1)
                st.append(prefix.left(prefix.size() - c.size()) + st.takeFirst() + " ");
        } else {
            result = ApxApp::jsexec(
                QString("(function(){var s='';for(var v in %1)s+=v+';';return s;})()").arg(scope));
            st = result.toString().replace('\n', ',').split(';', QString::SkipEmptyParts);
            st = st.filter(QRegExp("^" + c));
            if (st.size() == 1)
                st.append(prefix.left(prefix.size() - c.size()) + st.takeFirst());
        }
        if (!result.isError())
            hints = st;
    } else {
        //parameter or not a command
        c = c.remove(0, c.lastIndexOf(del) + 1).trimmed();
        bool bDot = c.contains('.');
        if (bDot) {
            scope = c.left(c.lastIndexOf('.'));
            c.remove(0, c.lastIndexOf('.') + 1);
        }
        result = ApxApp::jsexec(
            QString("(function(){var s='';for(var v in %1)s+=v+';';return s;})()").arg(scope));
        QStringList st(result.toString().replace('\n', ',').split(';', QString::SkipEmptyParts));
        //QStringList st(FactSystem::instance()->jsexec(QString("var s='';for(var v in %1)s+=v+';';").arg(scope)).toString().replace('\n',',').split(';',QString::SkipEmptyParts));
        st = st.filter(QRegExp("^" + c));
        if (st.size() == 1)
            st.append(prefix.left(prefix.size() - c.size()) + st.takeFirst() + (bDot ? "" : " "));
        if (!result.isError())
            hints = st;
    }
    //hints collected
    if (hints.isEmpty())
        return cmd;
    if (hints.size() == 1)
        return hints.first();
    hints.sort();
    //partial autocompletion
    if (c.isEmpty()) {
        c = cmd;
    } else {
        //partial autocompletion
        bool bMatch = true;
        QString s = c;
        while (bMatch && s.size() < hints.first().size()) {
            s += hints.first().at(s.size());
            foreach (const QString &hint, hints) {
                if (hint.startsWith(s))
                    continue;
                s.chop(1);
                bMatch = false;
                break;
            }
        }
        //qDebug()<<c<<prefix<<s;
        if (s != c) {
            c = prefix.left(prefix.size() - c.size()) + s;
        } else
            c = cmd;
    }
    //hints output formatting
    for (int i = 0; i < hints.size(); i++) {
        if (ApxApp::jsexec(QString("typeof(%1['%2'])=='function'").arg(scope).arg(hints.at(i)))
                .toBool()) {
            hints[i] = "<font color='white'><b>" + hints.at(i) + "</b></font>";
        } else if (ApxApp::jsexec(QString("typeof(%1['%2'])=='object'").arg(scope).arg(hints.at(i)))
                       .toBool()) {
            hints[i] = "<font color='yellow'>" + hints.at(i) + "</font>";
        } else if (ApxApp::jsexec(QString("typeof(%1['%2'])=='number'").arg(scope).arg(hints.at(i)))
                       .toBool()) {
            hints[i] = "<font color='cyan'>" + hints.at(i) + "</font>";
        } else
            hints[i] = "<font color='gray'>" + hints.at(i) + "</font>";
    }
    hints.removeDuplicates();
    hints.sort();
    _model->enter(hints.join(" "));
    return c;
}
//=============================================================================
//=============================================================================
DatalinkInspectorListModel *DatalinkInspector::outModel() const
{
    return _model;
}
//=============================================================================
