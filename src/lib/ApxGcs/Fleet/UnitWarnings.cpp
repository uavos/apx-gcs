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
#include <App/AppPrefs.h>

static constexpr const char *kw_prefs_name = "keywords";
static constexpr const char *kw_prefs_group = "warnings";
static constexpr int bubble_max_items = 3;

QList<UnitWarnings *> UnitWarnings::_instances;

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

    // preferences (same approach as MapPrefs in MissionPlanner)
    f_prefs = new Fact(this,
                       "prefs",
                       tr("Preferences"),
                       tr("Warnings panel settings"),
                       Action | IconOnly,
                       "wrench");
    f_keywords = new Fact(f_prefs,
                          "keywords",
                          tr("Bubble keywords"),
                          tr("Comma separated keywords to show message in bubble"),
                          Text,
                          "message-alert");
    // keywords are shared between all units - persistent value with global settings group
    f_keywords->setValue(AppPrefs::instance()->loadValue(kw_prefs_name, kw_prefs_group, ""));
    connect(f_keywords, &Fact::valueChanged, this, &UnitWarnings::keywordsChanged);
    _instances.append(this);

    showTimer.setSingleShot(true);
    showTimer.setInterval(5000);
    connect(&showTimer, &QTimer::timeout, this, &UnitWarnings::showTimerTimeout);
}

UnitWarnings::~UnitWarnings()
{
    _instances.removeAll(this);
}

void UnitWarnings::keywordsChanged()
{
    const auto v = f_keywords->value().toString();
    if (v.trimmed().isEmpty())
        AppPrefs::instance()->removeValue(kw_prefs_name, kw_prefs_group);
    else
        AppPrefs::instance()->saveValue(kw_prefs_name, v, kw_prefs_group);

    // sync other units (setValue is filtered by value, no recursion)
    for (auto i : _instances) {
        if (i != this)
            i->f_keywords->setValue(v);
    }
}

QStringList UnitWarnings::keywords() const
{
    QStringList list;
    for (auto s : f_keywords->value().toString().split(',')) {
        s = s.trimmed();
        if (!s.isEmpty())
            list.append(s);
    }
    return list;
}

bool UnitWarnings::matchKeywords(const QString &msg) const
{
    for (const auto &kw : keywords()) {
        if (msg.contains(kw, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

QStringList UnitWarnings::bubbleItems() const
{
    QStringList list;
    for (auto f : m_bubbleItems)
        list.append(f->title());
    return list;
}

void UnitWarnings::addBubbleItem(Fact *fact)
{
    // newest on top, limited number of items
    m_bubbleItems.removeAll(fact);
    m_bubbleItems.prepend(fact);
    while (m_bubbleItems.size() > bubble_max_items)
        m_bubbleItems.removeLast();
    emit bubbleItemsChanged();
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
            if (m_bubbleItems.removeAll(fact) > 0)
                emit bubbleItemsChanged();
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
    if (matchKeywords(msg))
        addBubbleItem(fact);
    showList.insert(showNum > showList.size() ? showList.size() : showNum, fact);
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
