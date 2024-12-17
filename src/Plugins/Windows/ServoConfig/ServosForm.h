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

#include <QWidget>
#include <QtCore>

namespace Ui {
class ServosForm;
}

class Unit;
class ServosForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServosForm(QWidget *parent = 0);

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::ServosForm *ui;

    QList<QMetaObject::Connection> clist;

    uint counter;
    void sendVolz(uint cmd, uint id, uint arg);
    void sendFutabaAddr(uint servoID, uint newAddr);

private slots:
    void unitSelected(Unit *unit);

    void btnFind();
    void btnMove();
    void btnSetAdr();

    void do_find(void);
    void serialData(quint16 portNo, QByteArray ba);
signals:
    void finished();
};
