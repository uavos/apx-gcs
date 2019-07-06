#include "SerialForm.h"
#include "ui_SerialForm.h"
#include <Vehicles/Vehicles.h>
#include <ApxLog.h>

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

    restoreGeometry(QSettings().value(objectName()).toByteArray());
    ui->ePortID->setValue(QSettings().value(objectName() + "_port").toUInt());

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
    QSettings().setValue(objectName(), saveGeometry());
    QSettings().setValue(objectName() + "_port", ui->ePortID->value());
    emit finished();
}
//=============================================================================
//==============================================================================
void SerialForm::btnReset()
{
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
                ba.append((char) si.toInt());
        }
        break;
    }
    if (ba.isEmpty())
        return;
    if (ui->cbCR->isChecked())
        ba.append('\r');
    if (ui->cbLF->isChecked())
        ba.append('\n');
    Vehicles::instance()->current()->sendSerial(ui->ePortID->value(), ba);
}
//==============================================================================
void SerialForm::serialData(uint portNo, QByteArray ba)
{
    if ((int) portNo != ui->ePortID->value())
        return;
    /*if (uart.isOpen()) {
        uart.write((uint8_t *) ba.data(), ba.size());
    }*/
    if (!ui->cbRead->isChecked())
        return;
    if (ba.size() <= 0)
        return;
    //qDebug("%u",ba.size());
    QString s;
    switch (ui->cbRxFormat->currentIndex()) {
    case 0: //ASCII
        s = QString(ba);
        s.remove('\r');
        s = s.trimmed();
        break;
    case 1: //HEX
        for (int i = 0; i < ba.size(); i++)
            s += QString().sprintf("%.2X ", (unsigned char) ba.at(i));
        s = s.trimmed();
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
