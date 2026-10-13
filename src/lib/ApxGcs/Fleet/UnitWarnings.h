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
#pragma once

#include <Fact/Fact.h>
#include <QtCore>
class Unit;

class UnitWarnings : public Fact
{
    Q_OBJECT
    Q_ENUMS(MsgType)

    // titles of warnings items matching keywords, shown in the bubble below the panel
    Q_PROPERTY(QStringList bubbleItems READ bubbleItems NOTIFY bubbleItemsChanged)

public:
    explicit UnitWarnings(Unit *parent);
    ~UnitWarnings() override;

    enum MsgType { INFO = 0, WARNING, ERROR };
    Q_ENUM(MsgType)

    Fact *f_clear;
    Fact *f_prefs;
    Fact *f_keywords; // comma separated keywords to show a message in the bubble

    QStringList bubbleItems() const;

    QStringList keywords() const;
    bool matchKeywords(const QString &msg) const;

private:
    QTimer showTimer;
    Fact *createItem(const QString &msg, MsgType kind);

    // bubble entries: warnings list items (removed together with them)
    // or plain info messages (removed by Clear)
    struct BubbleItem
    {
        QPointer<Fact> fact;
        QString text;
    };
    QList<BubbleItem> m_bubbleItems;
    void addBubbleItem(const QString &text, Fact *fact = nullptr);
    void clearBubble();

    // keywords are global for all units, stored in QSettings
    static QList<UnitWarnings *> _instances;
    void keywordsChanged();

    QHash<Fact *, int> showMap;
    FactList showList;
    int showNum;
private slots:
    void showTimerTimeout();
public slots:
    void warning(const QString &msg);
    void error(const QString &msg);
    void info(const QString &msg);
signals:
    void show(QString msg, MsgType msgType);
    void showMore(QString msg, MsgType msgType);
    void bubbleItemsChanged();
};
