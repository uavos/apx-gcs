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
#ifndef SerialFORM_H
#define SerialFORM_H

#include <QWidget>
#include <QtCore>
//#include "comm.h"

namespace Ui {
class SerialForm;
}

class Vehicle;
class SerialForm : public QWidget
{
    Q_OBJECT

public:
    explicit SerialForm(QWidget *parent = 0);

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::SerialForm *ui;
    //Comm uart;
    QSocketNotifier *socketNotifier;

    QFile dumpFile;

    QList<QMetaObject::Connection> clist;

private slots:
    void vehicleSelected(Vehicle *vehicle);

    void btnReset();
    void btnSend();
    void btnForward();
    void serialData(uint portNo, QByteArray ba);

    void uartRead();

signals:
    void finished();
};

#endif // SerialFORM_H
