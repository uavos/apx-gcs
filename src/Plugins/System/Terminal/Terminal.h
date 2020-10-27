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
#ifndef Terminal_H
#define Terminal_H
//=============================================================================
#include <Fact/Fact.h>
#include <QtCore>
//=============================================================================
class Terminal : public Fact
{
    Q_OBJECT

public:
    explicit Terminal(Fact *parent = nullptr);

    Q_INVOKABLE void exec(const QString &cmd);

    Q_INVOKABLE void historyReset();
    Q_INVOKABLE QString historyNext(const QString &cmd);
    Q_INVOKABLE QString historyPrev(const QString &cmd);

    Q_INVOKABLE QString autocomplete(const QString &cmd);

private:
    int _enterIndex;
    QStringList _history;
    int _historyIndex;
    QString _replacedHistory;

public slots:
    void enter(const QString &line);
    void enterResult(bool ok);

signals:
    void newMessage(QtMsgType type, QString category, QString text);
};
//=============================================================================
#endif
