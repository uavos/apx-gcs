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
#include "Terminal.h"
#include <App/App.h>
#include <App/AppLog.h>
#include <App/AppNotifyListModel.h>
#include <QDesktopServices>
#include <QQmlEngine>
#define MAX_HISTORY 50
//=============================================================================
Terminal::Terminal(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Terminal"),
           tr("System terminal"),
           Group,
           "console-line")
{
    setQmlPage("qrc:/" PLUGIN_NAME "/Terminal.qml");

    _enterIndex = 0;

    _history = QSettings().value("consoleHistory").toStringList();
    historyReset();

    qmlRegisterUncreatableType<Terminal>("APX.Terminal", 1, 0, "Terminal", "Reference only");

    loadQml("qrc:/" PLUGIN_NAME "/TerminalPlugin.qml");
}
//=============================================================================
void Terminal::exec(const QString &cmd)
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
    QJSValue v = App::jsexec(s);
    //history
    _history.removeAll(cmd);
    //if(!v.isError())
    _history.append(cmd);
    while (_history.size() > MAX_HISTORY)
        _history.removeFirst();
    QSettings().setValue("consoleHistory", _history);
    historyReset();
    enterResult(!v.isError());
}
//=============================================================================
void Terminal::historyReset()
{
    _historyIndex = -1;
    _replacedHistory = "";
}
QString Terminal::historyNext(const QString &cmd)
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
        return _history.value(_historyIndex).trimmed();
    }
    while (_historyIndex > 0) {
        _historyIndex--;
        QString s = _history.value(_historyIndex).trimmed();
        if (s.startsWith(_replacedHistory))
            return s;
    }
    return cmd;
}
QString Terminal::historyPrev(const QString &cmd)
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
QString Terminal::autocomplete(const QString &cmd)
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
        result = App::jsexec(QString("(function(){var s='';for(var v in "
                                     "%1)if(typeof(%1[v])=='function')s+=v+';';return s;})()")
                                 .arg(scope));
        QStringList st(result.toString().replace('\n', ',').split(';', QString::SkipEmptyParts));
        st = st.filter(QRegExp("^" + c));
        if (st.size()) {
            if (st.size() == 1)
                st.append(prefix.left(prefix.size() - c.size()) + st.takeFirst() + " ");
        } else {
            result = App::jsexec(
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
        result = App::jsexec(
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
        if (App::jsexec(QString("typeof(%1['%2'])=='function'").arg(scope).arg(hints.at(i)))
                .toBool()) {
            hints[i] = "<font color='white'><b>" + hints.at(i) + "</b></font>";
        } else if (App::jsexec(QString("typeof(%1['%2'])=='object'").arg(scope).arg(hints.at(i)))
                       .toBool()) {
            hints[i] = "<font color='yellow'>" + hints.at(i) + "</font>";
        } else if (App::jsexec(QString("typeof(%1['%2'])=='number'").arg(scope).arg(hints.at(i)))
                       .toBool()) {
            hints[i] = "<font color='cyan'>" + hints.at(i) + "</font>";
        } else
            hints[i] = "<font color='gray'>" + hints.at(i) + "</font>";
    }
    hints.removeDuplicates();
    hints.sort();
    enter(hints.join(" "));
    return c;
}
//=============================================================================
void Terminal::enter(const QString &line)
{
    App *app = App::instance();
    AppNotify::instance()->notification(line, "", AppNotify::FromInput, nullptr);
    _enterIndex = app->notifyModel()->rowCount() - 1;
}
void Terminal::enterResult(bool ok)
{
    App *app = App::instance();
    if (_enterIndex >= app->notifyModel()->rowCount())
        return;
    app->notifyModel()->updateItem(_enterIndex,
                                   ok ? tr("ok") : tr("error"),
                                   AppNotifyListModel::SubsystemRole);
    if (!ok)
        app->notifyModel()->updateItem(_enterIndex, AppNotify::Error, AppNotifyListModel::TypeRole);
}
//=============================================================================
