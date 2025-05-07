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
#include "SerialForm.h"

#include <App/AppDirs.h>
#include <App/AppLog.h>

#include <Fleet/Fleet.h>
#include <Telemetry/Telemetry.h>

#include "ui_SerialForm.h"

SerialForm::SerialForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SerialForm)
{
    ui->setupUi(this);

    QFont f("unexistent");
    f.setStyleHint(QFont::Monospace);
    ui->textEdit->setFont(f);

    ui->btnForward->setVisible(false);
    ui->eForward->setVisible(false);

    connect(ui->btnReset, SIGNAL(pressed()), this, SLOT(btnReset()));
    connect(ui->btnSend, SIGNAL(pressed()), this, SLOT(btnSend()));
    connect(ui->btnForward, SIGNAL(pressed()), this, SLOT(btnForward()));
    connect(ui->eTxText, SIGNAL(returnPressed()), this, SLOT(btnSend()));

    ui->ePortID->setValue(QSettings().value(objectName() + "_port").toInt());

    ui->eForward->setText(QSettings().value(objectName() + "_fwdDev").toString());

    connect(Fleet::instance(), &Fleet::unitSelected, this, &SerialForm::unitSelected);
    unitSelected(Fleet::instance()->current());
}

void SerialForm::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    disconnect(this);
    /*if (uart.isOpen()) {
        socketNotifier->setEnabled(false);
        uart.close();
        apxMsg() << tr("Serial port forwarding stopped");
    }*/
    //QSettings().setValue(objectName(), saveGeometry());
    QSettings().setValue(objectName() + "_port", ui->ePortID->value());
    emit finished();
}

void SerialForm::unitSelected(Unit *unit)
{
    for (auto c : clist)
        disconnect(c);
    auto protocol = unit->protocol();
    if (!protocol)
        return;
    clist.append(connect(protocol->data(), &PData::serialData, this, &SerialForm::serialData));
}

void SerialForm::btnReset()
{
    if (dumpFile.isOpen())
        dumpFile.close();
    ui->textEdit->clear();
}
void SerialForm::btnSend()
{
    ui->eTxText->selectAll();
    QString s = ui->eTxText->text();
    if (!s.size())
        return;
    QByteArray ba;
    switch (ui->cbTxFormat->currentIndex()) {
    case 0: //ASCII
        ba.append(s.toUtf8());
        break;
    case 1: //HEX
        ba = QByteArray::fromHex(s.trimmed().toUtf8());
        break;
    case 2: //List
        for (auto si : s.trimmed().split(',')) {
            si = si.trimmed();
            if (si.contains('"')) {
                si.remove('"');
                if (si.size())
                    ba.append(si.toUtf8());
            } else
                ba.append(static_cast<char>(si.toInt()));
        }
        break;
    }
    if (ba.isEmpty())
        return;
    if (ui->cbCR->isChecked())
        ba.append('\r');
    if (ui->cbLF->isChecked())
        ba.append('\n');

    auto protocol = Fleet::instance()->current()->protocol();
    if (protocol)
        protocol->data()->sendSerial(static_cast<quint8>(ui->ePortID->value()), ba);
}

void SerialForm::serialData(uint portNo, QByteArray ba)
{
    if (static_cast<int>(portNo) != ui->ePortID->value())
        return;

    //dump log
    if (ui->cbRec->isChecked()) {
        int fcnt = 0;
        while (!dumpFile.isOpen()) {
            QString fname = QDateTime::currentDateTimeUtc().toString("yyyy_MM_dd_hh_mm_ss_zzz");
            fname.append(QString("-%1").arg(portNo));
            if (!ui->eRec->text().trimmed().isEmpty()) {
                fname.append(QString("-%1").arg(ui->eRec->text().trimmed()));
            }
            if (fcnt > 0)
                fname.append(QString("-%1").arg(fcnt));
            fname.append(".log");
            QDir dir(AppDirs::logs().absoluteFilePath("serial"));
            dir.mkpath(".");
            dumpFile.setFileName(dir.absoluteFilePath(fname));
            if (dumpFile.exists()) {
                apxMsgW() << tr("File exists:") << fname;
                fcnt++;
                continue;
            }
            if (!dumpFile.open(QFile::WriteOnly)) {
                apxMsgW() << tr("File error:") << fname;
                ui->cbRec->setChecked(false);
            }
            break;
        }

    } else {
        if (dumpFile.isOpen())
            dumpFile.close();
    }

    /*if (uart.isOpen()) {
        uart.write((uint8_t *) ba.data(), ba.size());
    }*/

    QString sTimestamp;
    if (dumpFile.isOpen()) {
        sTimestamp.append(QString("%1").arg(QDateTime::currentDateTime().toMSecsSinceEpoch()));
    }

    if (!ui->cbRead->isChecked())
        return;
    if (ba.size() <= 0)
        return;
    //qDebug("%u",ba.size());
    QString s;
    switch (ui->cbRxFormat->currentIndex()) {
    case 0: //ASCII
        if (dumpFile.isOpen()) {
            if (!sTimestamp.isEmpty()) {
                dumpFile.write(sTimestamp.toUtf8().append(':'));
            }
            dumpFile.write(ba);
        }

        s = QString(ba);
        s.remove('\r');
        //s = s.trimmed();
        break;
    case 1: //HEX
        if (ui->cbInfo->isChecked()) {
            s = QString("size:%1 data:%2").arg(ba.size(), -3, 10).arg((QString) ba.toHex(' '));
        } else {
            s = (QString) ba.toHex(' ');
        }
        s = s.trimmed().append('\n');

        if (dumpFile.isOpen()) {
            if (!sTimestamp.isEmpty()) {
                dumpFile.write(sTimestamp.toUtf8().append(':'));
            }
            dumpFile.write(s.toUtf8());
        }
        break;
    }
    if (s.isEmpty())
        return;
    ui->textEdit->moveCursor(QTextCursor::End);
    ui->textEdit->insertPlainText(s);
    ui->textEdit->moveCursor(QTextCursor::End);
}

void SerialForm::btnForward()
{
    /*if (uart.isOpen()) {
        disconnect(socketNotifier, SIGNAL(activated(int)), this, SLOT(uartRead()));
        socketNotifier->setEnabled(false);
        socketNotifier->deleteLater();
        uart.close();
    }
    if (!uart.open(ui->eForward->text().toUtf8().data()))
        return;
    QSettings().setValue(objectName() + "_fwdDev", ui->eForward->text());
    socketNotifier = new QSocketNotifier(uart.handle(), QSocketNotifier::Read);
    connect(socketNotifier, SIGNAL(activated(int)), this, SLOT(uartRead()));
    apxMsg() << tr("Serial port forwarding started") + "...";*/
}

void SerialForm::uartRead()
{
    /*if (!uart.isOpen())
        return;
    QByteArray ba;
    ba.resize(64);
    ba.resize(uart.read((uint8_t *) ba.data(), ba.size()));
    if (ba.size() <= 0)
        return;
    Fleet::instance()->current()->sendSerial(ui->ePortID->value(), ba);
    QTimer::singleShot(100, this, SLOT(uartRead()));*/
}
