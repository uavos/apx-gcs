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
#include "UnitWarnings.h"
#include "Unit.h"
#include <App/App.h>

UnitWarnings::UnitWarnings(Unit *parent)
    : Fact(parent,
           "warnings",
           tr("Warnings"),
           tr("Malfunctions and warnings list"),
           Group | Count | FlatModel,
           "alert")
    , showNum(0)
{
    f_clear = new Fact(this,
                       "clear",
                       tr("Clear"),
                       tr("Remove all messages from list"),
                       Action,
                       "notification-clear-all");
    f_clear->setEnabled(false);
    connect(f_clear, &Fact::triggered, this, &Fact::deleteChildren);

    connect(this, &Fact::sizeChanged, this, [=]() { f_clear->setEnabled(size() > 0); });

    showTimer.setSingleShot(true);
    showTimer.setInterval(5000);
    connect(&showTimer, &QTimer::timeout, this, &UnitWarnings::showTimerTimeout);
}

void UnitWarnings::warning(const QString &msg)
{
    createItem(msg, WARNING);
    App::sound("warning");
}
void UnitWarnings::error(const QString &msg)
{
    createItem(msg, ERROR);
    App::sound("error");
}

Fact *UnitWarnings::createItem(const QString &msg, MsgType kind)
{
    Fact *fact = nullptr;
    if (size() > 0) {
        fact = child(0);
        if (fact->title() != msg || fact->property("kind").toInt() != kind)
            fact = nullptr;
    }
    if (!fact) {
        fact = new Fact(this, "item#", msg, "");
        fact->move(0);
        if (size() > 100)
            child(size() - 1)->deleteFact();
        fact->setValue(1);
        fact->setProperty("kind", kind);
        switch (kind) {
        case INFO:
            fact->setIcon("information");
            break;
        case WARNING:
            fact->setIcon("alert-circle");
            break;
        case ERROR:
            fact->setIcon("alert-octagon");
            break;
        }
        fact->setDescr(QDateTime::currentDateTime().toString());
        connect(fact, &Fact::destroyed, this, [=]() {
            showMap.remove(fact);
            showList.removeAll(fact);
        });
    } else {
        fact->setValue(fact->value().toUInt() + 1);
    }
    for (auto f : showList) {
        if (f->title() != fact->title())
            continue;
        showList.removeAll(f);
        showMap.remove(f);
        break;
    }
    emit show(fact->title(), kind);
    showList.insert(showNum, fact);
    showMap.insert(fact, 0);
    showNum = showList.indexOf(fact);
    showTimer.stop();
    showTimerTimeout();
    return fact;
}

void UnitWarnings::showTimerTimeout()
{
    if (showList.isEmpty()) {
        return;
    }
    //continuously show all facts
    if (showNum >= showList.size())
        showNum = 0;
    Fact *fact = showList.at(showNum);
    emit showMore(fact->title(), (MsgType) fact->property("kind").toInt());
    //qDebug()<<fact->title();

    //next item
    if (++showMap[fact] >= 3) {
        showList.removeAll(fact);
        showMap.remove(fact);
    } else
        showNum++;
    if (showNum >= showList.size())
        showNum = 0;
    if (!showList.isEmpty()) {
        showTimer.start();
    }
}
