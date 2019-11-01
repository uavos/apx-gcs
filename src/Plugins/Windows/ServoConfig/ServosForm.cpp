#include "ServosForm.h"
#include "ui_ServosForm.h"
#include <App/AppLog.h>
#include <Vehicles/Vehicles.h>

//==============================================================================
ServosForm::ServosForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ServosForm)
{
    ui->setupUi(this);

    connect(ui->btnFind, SIGNAL(pressed()), this, SLOT(btnFind()));
    connect(ui->btnMove, SIGNAL(pressed()), this, SLOT(btnMove()));
    connect(ui->btnSetAdr, SIGNAL(pressed()), this, SLOT(btnSetAdr()));

    connect(Vehicles::instance(),
            &Vehicles::currentSerialDataReceived,
            this,
            &ServosForm::serialData);
}
//=============================================================================
ServosForm::~ServosForm()
{
    delete ui;
}
void ServosForm::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    disconnect(this);
    emit finished();
}
//=============================================================================
//==============================================================================
void ServosForm::btnFind()
{
    ui->cbAdr->clear();
    counter = 1;
    apxMsg() << tr("Scanning for servos").append("...");
    do_find();
}
//==============================================================================
void ServosForm::btnMove()
{
    sendVolz(0xDD,
             ui->cbAdr->itemData(ui->cbAdr->currentIndex()).toInt(),
             0x1000 + (ui->eAdr->value() & 0x7F));
    apxMsg() << tr("Servo moved");
}
//==============================================================================
void ServosForm::btnSetAdr()
{
    switch (ui->tabWidget->currentIndex()) {
    case 0: //Volz
        sendVolz(0xAA,
                 ui->cbAdr->itemData(ui->cbAdr->currentIndex()).toInt(),
                 (ui->eAdr->value() << 8) | (ui->eAdr->value()));
        break;
    case 1: //Futaba
        sendFutabaAddr(ui->eFutabaNum1->value() << 16 | ui->eFutabaNum2->value(), ui->eAdr->value());
        break;
    }
    apxMsg() << tr("Address set");
}
//==============================================================================
void ServosForm::serialData(uint portNo, QByteArray ba)
{
    if ((int) portNo != ui->ePortID->value())
        return;
    //qDebug("%u",ba.size());
    if (ba.size() != 6)
        return;
    ui->cbAdr->addItem(QString("%1").arg((int) ba.at(1), 16), QVariant(ba.at(1)));
}
//==============================================================================
void ServosForm::do_find(void)
{
    sendVolz(0x92, counter++, 0x00);
    if (counter < 0x1F)
        QTimer::singleShot(200, this, SLOT(do_find()));
    else
        apxMsg() << tr("Scan finished");
}
//==============================================================================
void ServosForm::sendVolz(uint cmd, uint id, uint arg)
{
    QByteArray pack;
    pack[0] = cmd;
    pack[1] = id;
    pack[2] = (arg >> 8) & 0xFF;
    pack[3] = arg & 0xFF;

    uint16_t crc = 0xFFFF;

    for (int x = 0; x < 4; x++) {
        crc = ((pack.at(x) << 8) ^ crc);
        for (int y = 0; y < 8; y++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x8005;
            else
                crc = crc << 1;
        }
    }
    pack[4] = (crc >> 8) & 0xFF;
    pack[5] = crc & 0xFF;
    Vehicles::instance()->current()->sendSerial(ui->ePortID->value(), pack);
}
//==============================================================================
void ServosForm::sendFutabaAddr(uint servoID, uint newAddr)
{
    qInfo() << QString("SBUS ADDR %1-%2: %3").arg(servoID >> 16).arg(servoID & 0xFFFF).arg(newAddr);
    QByteArray pack;
    pack[0] = 0xF9;
    pack[1] = newAddr - 1;
    pack[2] = servoID >> 16 & 0xFF;
    pack[3] = servoID >> 8 & 0xFF;
    pack[4] = servoID & 0xFF;
    pack[5] = 0x17;
    pack[6] = 0x72;
    pack[7] = 0xD0;
    pack[8] = 0x80;
    pack[9] = 0x80;
    pack[10] = 0x80;
    pack[11] = 0x00;
    pack[12] = 0x00;
    pack[13] = 0x34;
    pack[14] = 0x01;
    pack[15] = 0x2D;
    pack[16] = 0x00;
    //crc 2's complement sum
    uint crc = 0;
    for (uint i = 1; i < 16; i++)
        crc += pack[i];
    pack[16] = 0x100 - crc;
    //reverse bits
    for (int i = 0; i < pack.size(); i++) {
        uint v = pack[i];
        uint vx = 0;
        uint m2 = 0x80;
        for (uint m = 1; m < 0x0100; m <<= 1) {
            if (v & m)
                vx |= m2;
            m2 >>= 1;
        }
        pack[i] = vx;
    }
    Vehicles::instance()->current()->sendSerial(ui->ePortID->value(), pack);
}
//==============================================================================
