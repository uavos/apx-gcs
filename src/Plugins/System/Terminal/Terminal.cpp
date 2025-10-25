/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "Terminal.h"
#include <App/App.h>
#include <App/AppLog.h>
#include <App/AppNotifyListModel.h>
#include <algorithm>
#include <QDesktopServices>
#include <QQmlEngine>

#define MAX_HISTORY 50

Terminal::Terminal(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Terminal"),
           tr("System terminal"),
           Group,
           "console-line")
{
    setOpt("page", "qrc:/" PLUGIN_NAME "/Terminal.qml");

    _enterIndex = 0;

    _history = QSettings().value("consoleHistory").toStringList();
    historyReset();

    qmlRegisterUncreatableType<Terminal>("APX.Terminal", 1, 0, "Terminal", "Reference only");

    loadQml("qrc:/" PLUGIN_NAME "/TerminalPlugin.qml");
}

void Terminal::exec(QString cmd)
{
    QString s = cmd.simplified();
    if (s.isEmpty())
        return;

    if (s == "cls") {
        App::instance()->notifyModel()->clear();
        return;
    }

    if (s.startsWith(';') || s.startsWith('!')) {
        s.remove(0, 1);
    } else if (!(s.contains("(") || s.contains("=") || s.contains("+") || s.contains("-"))) {
        QStringList st = s.split(' ');
        if (!st.size())
            return;
        QString sc = st.takeFirst();
        if ((sc.startsWith("set") || sc.startsWith("req") || sc.startsWith("send")) && st.size()) {
            st.insert(0, "'" + st.takeFirst() + "'"); //quote var name
        } else if (sc == "sh") {
            s = sc + "(['" + st.join("','") + "'])";
        } else {
            s = sc + "(" + st.join(",") + ")";
        }
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

void Terminal::historyReset()
{
    _historyIndex = -1;
    _replacedHistory = "";
}
QString Terminal::historyNext(QString cmd)
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
QString Terminal::historyPrev(QString cmd)
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

QString Terminal::autocomplete(QString cmd)
{
    QString prefix = cmd;
    QString c = cmd;

    c.remove(0, c.lastIndexOf(';') + 1);
    if (c.endsWith(' '))
        c = c.trimmed().append(' ');
    else
        c = c.trimmed();

    QString scope;
    QMap<QString, QJSValue> map;

    QRegExp del("[\\ \\,\\:\\t\\{\\}\\[\\]\\(\\)\\=]");
    if (!(c.contains(del) || prefix.startsWith('!') || c.contains('.'))) {
        //first word input (std command?)
        map = _get_js_properties(scope, c);
        for (auto i : map.keys()) {
            QJSValue v = map.value(i);
            if (v.isObject())
                continue;
            if (v.isCallable())
                continue;
            map.remove(i);
        }

    } else {
        //parameter or not a command
        c = c.remove(0, c.lastIndexOf(del) + 1).trimmed();
        bool bDot = c.contains('.');
        if (bDot) {
            scope = c.left(c.lastIndexOf('.'));
            c.remove(0, c.lastIndexOf('.') + 1);
        }

        map = _get_js_properties(scope, c);
    }
    //hints collected
    if (map.isEmpty())
        return cmd;
    if (map.size() == 1) {
        QJSValue v = map.first();
        cmd = prefix.left(prefix.size() - c.size()) + map.keys().first();
        if (v.isCallable())
            return cmd + " ";
        if (v.isObject())
            return cmd + ".";
        return cmd;
    }

    //partial autocompletion
    if (c.isEmpty()) {
        c = cmd;
    } else {
        //partial autocompletion
        bool bMatch = true;
        QString s = c;
        QString shortest = map.keys().first();
        while (bMatch && s.size() < shortest.size()) {
            s += shortest.at(s.size());
            for (auto hint : map.keys()) {
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
    QStringList hints;
    for (auto k : map.keys()) {
        QJSValue v = map.value(k);

        if (v.isCallable()) {
            hints.append("<font color='white'><b>" + k + "</b></font>");
        } else if (v.isObject() && v.toVariant().value<Fact *>()) {
            hints.append("<font color='green'>" + k + "</font>");
        } else if (QString(v.toVariant().typeName()) == "QVariantMap") {
            hints.append("<font color='yellow'>" + k + "</font>");
        } else if (v.isNumber()) {
            hints.append("<font color='cyan'>" + k + "</font>");
        } else {
            hints.append("<font color='gray'>" + k + "</font>");
        }
    }
    hints.removeDuplicates();
    hints.sort();
    enter(hints.join(" "));
    return c;
}

void Terminal::copyConsoleHistoryToClipboard() const
{
    const auto &modelItems = App::instance()->notifyModel()->m_items;
    QString log = std::accumulate(std::cbegin(modelItems),
                                  std::cend(modelItems),
                                  QString(),
                                  [](auto res, auto item) {
                                      return std::move(res).append('\n' + item->text);
                                  });
    _clipboard->setText(std::move(log));
}

void Terminal::copySelectedLinesToClipboard() const
{
    const auto &modelItems = App::instance()->notifyModel()->m_items;
    QString selectedText = std::accumulate(std::cbegin(_selectedLines),
                                           std::cend(_selectedLines),
                                           QString(),
                                           [&modelItems](auto res, auto row) {
                                               return std::move(res).append(
                                                   '\n' + modelItems.at(row)->text);
                                           });
    _clipboard->setText(std::move(selectedText));
}

void Terminal::unselectAllLines()
{
    auto &modelItems = App::instance()->notifyModel()->m_items;
    std::for_each(std::cbegin(_selectedLines), std::cend(_selectedLines), [&modelItems](auto row) {
        modelItems.at(row)->selected = false;
    });
    _selectedLines.clear();
    emit selectionChanged();
}

void Terminal::selectLine(int row)
{
    App::instance()->notifyModel()->updateItem(row, true, AppNotifyListModel::SelectedRole);
    _selectedLines.insert(row);
}

QMap<QString, QJSValue> Terminal::_get_js_properties(QString scope, QString flt)
{
    QMap<QString, QJSValue> map;

    QJSValue v = App::instance()->engine()->jsGetProperty(scope);
    if (v.isError()) {
        qWarning() << v.errorType() << v.toString();
        return map;
    }

    QRegExp re("^" + flt);
    QJSValueIterator it(v);
    while (it.hasNext()) {
        it.next();
        if (!flt.isEmpty() && !it.name().contains(re))
            continue;
        map.insert(it.name(), it.value());
    }

    //qDebug() << scope << v.isError() << v.toString() << st;
    return map;
}

void Terminal::enter(QString line)
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
