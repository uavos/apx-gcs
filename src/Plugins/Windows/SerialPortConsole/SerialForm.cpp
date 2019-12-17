#include "SerialForm.h"
#include "ui_SerialForm.h"

#include <App/AppDirs.h>
#include <App/AppLog.h>

#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryRecorder.h>
#include <Vehicles/Vehicles.h>

//==============================================================================
SerialForm::SerialForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SerialForm)
{
    ui->setupUi(this);

    //TODO
    ui->btnForward->setVisible(false);
    ui->eForward->setVisible(false);

    connect(ui->btnReset, SIGNAL(pressed()), this, SLOT(btnReset()));
    connect(ui->btnSend, SIGNAL(pressed()), this, SLOT(btnSend()));
    connect(ui->btnForward, SIGNAL(pressed()), this, SLOT(btnForward()));
    connect(ui->eTxText, SIGNAL(returnPressed()), this, SLOT(btnSend()));

    //restoreGeometry(QSettings().value(objectName()).toByteArray());
    ui->ePortID->setValue(QSettings().value(objectName() + "_port").toInt());

    ui->eForward->setText(QSettings().value(objectName() + "_fwdDev").toString());

    connect(Vehicles::instance(),
            &Vehicles::currentSerialDataReceived,
            this,
            &SerialForm::serialData);
}
//==============================================================================
SerialForm::~SerialForm()
{
    delete ui;
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
//=============================================================================
//==============================================================================
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
        ba.append(s);
        break;
    case 1: //HEX
        ba = QByteArray::fromHex(s.trimmed().toUtf8());
        break;
    case 2: //List
        foreach (QString si, s.trimmed().split(',')) {
            si = si.trimmed();
            if (si.contains('"')) {
                si.remove('"');
                if (si.size())
                    ba.append(si);
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
    Vehicles::instance()->current()->sendSerial(static_cast<quint8>(ui->ePortID->value()), ba);
}
//==============================================================================
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
        TelemetryRecorder *rec = Vehicles::instance()->current()->f_telemetry->f_recorder;
        if (rec)
            sTimestamp.append(QString("%1").arg(rec->currentTimstamp()));
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
        s = s.trimmed();
        break;
    case 1: //HEX
        for (int i = 0; i < ba.size(); i++)
            s += QString("%1 ").arg(static_cast<uint>(ba.at(i)), 2, 16, QChar('0'));
        s = s.trimmed();

        if (dumpFile.isOpen()) {
            if (!sTimestamp.isEmpty()) {
                dumpFile.write(sTimestamp.toUtf8().append(':'));
            }
            dumpFile.write(s.toUtf8().append('\n'));
        }
        break;
    }
    if (s.isEmpty())
        return;
    ui->textEdit->appendPlainText(s);
}
//==============================================================================
//==============================================================================
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
//==============================================================================
void SerialForm::uartRead()
{
    /*if (!uart.isOpen())
        return;
    QByteArray ba;
    ba.resize(64);
    ba.resize(uart.read((uint8_t *) ba.data(), ba.size()));
    if (ba.size() <= 0)
        return;
    Vehicles::instance()->current()->sendSerial(ui->ePortID->value(), ba);
    QTimer::singleShot(100, this, SLOT(uartRead()));*/
}
//=============================================================================
//==============================================================================
//==============================================================================
