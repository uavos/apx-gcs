/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "DatalinkPort.h"
#include "Datalink.h"
#include "DatalinkPorts.h"
#include "DatalinkRemote.h"
#include "DatalinkSerial.h"

#include <App/App.h>
#include <App/AppLog.h>
//=============================================================================
DatalinkPort::DatalinkPort(DatalinkPorts *parent, Datalink *datalink, const DatalinkPort *port)
    : Fact(port ? parent->f_list : parent,
           port ? "port#" : tr("add"),
           port ? "" : tr("Add new port"),
           port ? "" : tr("Configure new port"),
           Group | (port ? Bool : NoFlags))
    , f_connection(nullptr)
    , ports(parent)
    , _new(port ? false : true)
    , _blockUpdateRoutingValue(false)
    , _blockUpdateRoutingFacts(false)
{
    f_comment = new Fact(this, "comment", tr("Description"), tr("Comment"), Text);

    f_enable = new Fact(this, "enable", tr("Enabled"), tr("Connect when available"), Bool);

    f_type = new Fact(this, "type", tr("Type"), tr("Link type"), Enum);
    f_type->setEnumStrings(QMetaEnum::fromType<PortType>());
    f_type->setEnabled(_new);

    f_url = new Fact(this, "url", tr("URL"), tr("Port device path or address"), Text);
    connect(f_type, &Fact::valueChanged, this, &DatalinkPort::syncUrlEnum);
    connect(datalink->f_remotes->f_servers,
            &Fact::sizeChanged,
            this,
            &DatalinkPort::syncUrlEnum,
            Qt::QueuedConnection);
    syncUrlEnum();

    f_baud = new Fact(this, "baud", tr("Baud rate"), tr("Serial port speed"), Text);
    f_baud->setEnumStrings(QStringList() << "460800"
                                         << "230400"
                                         << "115200");

    f_codec = new Fact(this, "codec", tr("Codec"), tr("Packet framing"), Enum);
    f_codec->setEnumStrings(QMetaEnum::fromType<DatalinkSerial::CodecType>());

    //network routing
    f_routing = new Fact(this,
                         "routing",
                         tr("Data routing"),
                         tr("Advanced network configuration"),
                         Group);
    const QMetaEnum &m = QMetaEnum::fromType<Datalink::NetworkMask>();
    for (int i = 0; i < m.keyCount(); ++i) {
        QString key = m.key(i);
        int v = m.value(i);
        QString s;
        switch (v) {
        case Datalink::LOCAL:
            s = tr("Process received data on this GCS");
            break;
        case Datalink::CLIENTS:
            s = tr("Forward to remote clients");
            break;
        case Datalink::SERVERS:
            s = tr("Forward to remote servers");
            break;
        case Datalink::PORTS:
            s = tr("Forward to local ports");
            break;
        case Datalink::AUX:
            s = tr("Forward to AUX ports");
            break;
        }
        Fact *f = new Fact(f_routing, "rx_" + key.toLower(), key, s, Bool);
        f->setSection(tr("Receive network"));
        connect(f, &Fact::valueChanged, this, &DatalinkPort::updateRoutingValue);
        f_rx.append(f);
    }
    for (int i = 0; i < m.keyCount(); ++i) {
        QString key = m.key(i);
        int v = m.value(i);
        QString s;
        switch (v) {
        case Datalink::LOCAL:
            s = tr("Transmit data from this GCS");
            break;
        case Datalink::CLIENTS:
            s = tr("Transmit data from remote clients");
            break;
        case Datalink::SERVERS:
            s = tr("Transmit data from remote servers");
            break;
        case Datalink::PORTS:
            s = tr("Transmit data from local ports");
            break;
        case Datalink::AUX:
            s = tr("Transmit data from AUX ports");
            break;
        }
        Fact *f = new Fact(f_routing, "tx_" + key.toLower(), key, s, Bool);
        f->setSection(tr("Transmit network"));
        connect(f, &Fact::valueChanged, this, &DatalinkPort::updateRoutingValue);
        f_tx.append(f);
    }
    connect(f_routing, &Fact::valueChanged, this, &DatalinkPort::updateRoutingFacts);
    connect(f_routing, &Fact::valueChanged, this, &DatalinkPort::updateRoutingStatus);
    updateRoutingValue();

    if (_new) {
        f_save = new Fact(this, "save", tr("Save"), "", Action | Apply | CloseOnTrigger);
        connect(f_save, &Fact::triggered, parent, &DatalinkPorts::addTriggered);
        connect(this, &Fact::triggered, this, &DatalinkPort::defaults);
        connect(f_type, &Fact::valueChanged, this, &DatalinkPort::defaultUrl);
        defaults();
    } else {
        copyValuesFrom(port);
        f_remove = new Fact(this, "remove", tr("Remove"), "", Action | Remove);
        connect(f_remove, &Fact::triggered, this, &DatalinkPort::removeTriggered);

        for (int i = 0; i < size(); ++i) {
            connect(child(i), &Fact::valueChanged, parent, &DatalinkPorts::save);
        }
        //f_connection
        switch (f_type->value().toInt()) {
        case SERIAL:
            f_connection = new DatalinkSerial(this, f_url->text(), f_baud->value().toUInt());
            connect(f_url, &Fact::valueChanged, f_connection, [this]() {
                qobject_cast<DatalinkSerial *>(f_connection)->setDevName(f_url->text());
            });
            connect(f_codec, &Fact::valueChanged, f_connection, [this]() {
                qobject_cast<DatalinkSerial *>(f_connection)
                    ->setCodec(f_codec->value().value<DatalinkSerial::CodecType>());
            });
            qobject_cast<DatalinkSerial *>(f_connection)
                ->setCodec(f_codec->value().value<DatalinkSerial::CodecType>());
            break;
        case TCP:
            f_connection = new DatalinkRemote(this, datalink, f_url->text());
            connect(f_url, &Fact::valueChanged, f_connection, [this]() {
                qobject_cast<DatalinkRemote *>(f_connection)->setRemoteUrl(f_url->text());
            });
            break;
        }
        if (f_connection) {
            f_connection->setEnabled(false);
            connect(this, &Fact::valueChanged, f_connection, [this]() {
                f_connection->setActivated(value().toBool());
            });
            connect(f_connection, &DatalinkConnection::activatedChanged, this, [this]() {
                setValue(f_connection->activated());
            });
            bind(f_connection, "active");
            connect(f_routing, &Fact::valueChanged, this, &DatalinkPort::updateConnectionNetwork);
            updateConnectionNetwork();

            connect(f_connection, &Fact::titleChanged, this, &DatalinkPort::updateStatus);

            datalink->addConnection(f_connection);
        }

        //enable switch
        bind(f_enable, "value");

        connect(App::instance(), &App::loadingFinished, this, [this]() {
            setValue(f_enable->value());
        });
    }

    for (int i = 0; i < size(); ++i) {
        connect(child(i), &Fact::valueChanged, this, &DatalinkPort::updateStatus);
    }
    updateStatus();
}
//=============================================================================
void DatalinkPort::defaults()
{
    f_enable->setValue(true);
    f_type->setValue(SERIAL);
    f_baud->setValue("460800");
    f_routing->setValue("LOCAL,CLIENTS:LOCAL,CLIENTS");
    syncUrlEnum();
}
void DatalinkPort::clear()
{
    f_enable->setValue(false);
    f_type->setValue(SERIAL);
    f_baud->setValue(QVariant());
    f_routing->setValue(QVariant());
    syncUrlEnum();
}
void DatalinkPort::defaultUrl()
{
    switch (f_type->value().toInt()) {
    case SERIAL:
        f_url->setValue("auto");
        break;
    case TCP:
        f_url->setValue("192.168.1.23");
        break;
    }
}
//=============================================================================
void DatalinkPort::removeTriggered()
{
    setParentFact(nullptr);
    ports->save();
    deleteLater();
}
//=============================================================================
void DatalinkPort::updateStatus()
{
    int type = f_type->value().toInt();
    bool bSerial = type == SERIAL;
    f_baud->setVisible(bSerial);
    f_codec->setVisible(bSerial);

    if (_new)
        return;
    QStringList st;
    if (!f_comment->text().isEmpty())
        st << f_comment->text();

    if (f_connection) {
        st << f_connection->title();
        setDescr(f_connection->descr());
    } else {
        st << f_url->text();
        setDescr(f_routing->value().toString());
    }
    setTitle(st.join(": "));
}
//=============================================================================
void DatalinkPort::updateRoutingValue()
{
    if (_blockUpdateRoutingValue)
        return;
    _blockUpdateRoutingFacts = true;
    QStringList rx, tx;
    for (int i = 0; i < f_rx.size(); ++i) {
        if (f_rx.at(i)->value().toBool())
            rx.append(f_rx.at(i)->title());
    }
    for (int i = 0; i < f_tx.size(); ++i) {
        if (f_tx.at(i)->value().toBool())
            tx.append(f_tx.at(i)->title());
    }
    f_routing->setValue(QString("%1:%2").arg(rx.join(',')).arg(tx.join(',')));
    _blockUpdateRoutingFacts = false;
}
void DatalinkPort::updateRoutingFacts()
{
    if (_blockUpdateRoutingFacts)
        return;
    _blockUpdateRoutingValue = true;
    QString v = f_routing->value().toString();
    QStringList rx = v.left(v.indexOf(':')).split(',', QString::SkipEmptyParts);
    for (int i = 0; i < f_rx.size(); ++i) {
        f_rx.at(i)->setValue(rx.contains(f_rx.at(i)->title()));
    }
    QStringList tx = v.mid(v.indexOf(':') + 1).split(',', QString::SkipEmptyParts);
    for (int i = 0; i < f_rx.size(); ++i) {
        f_tx.at(i)->setValue(tx.contains(f_tx.at(i)->title()));
    }
    _blockUpdateRoutingValue = false;
}
void DatalinkPort::updateRoutingStatus()
{
    QString v = f_routing->value().toString();
    QString rx = v.left(v.indexOf(':'));
    QString tx = v.mid(v.indexOf(':') + 1);
    if (rx.isEmpty())
        rx = "NO";
    if (tx.isEmpty())
        tx = "NO";
    f_routing->setDescr(QString("RX: %1 / TX: %2").arg(rx).arg(tx));
}
void DatalinkPort::updateConnectionNetwork()
{
    if (!f_connection)
        return;
    const QMetaEnum &m = QMetaEnum::fromType<Datalink::NetworkMask>();
    QString v = f_routing->value().toString();
    quint16 rxNetwork = 0;
    foreach (QString s, v.left(v.indexOf(':')).split(',', QString::SkipEmptyParts)) {
        rxNetwork |= m.keyToValue(s.toUtf8());
    }
    quint16 txNetwork = 0;
    foreach (QString s, v.mid(v.indexOf(':') + 1).split(',', QString::SkipEmptyParts)) {
        txNetwork |= m.keyToValue(s.toUtf8());
    }
    f_connection->setRxNetwork(rxNetwork);
    f_connection->setTxNetwork(txNetwork);
}
//=============================================================================
void DatalinkPort::syncUrlEnum()
{
    QStringList st;
    switch (f_type->value().toInt()) {
    case SERIAL: {
        st << "auto";
        foreach (QSerialPortInfo spi, QSerialPortInfo::availablePorts()) {
            if (st.contains(spi.portName()))
                continue;
            st.append(spi.portName());
        }
    } break;
    case TCP: {
        const Fact *fg = static_cast<const Fact *>(sender());
        for (int i = 0; i < fg->size(); ++i) {
            const Fact *f = fg->child(i);
            if (st.contains(f->title()))
                continue;
            st.append(f->title());
        }
    } break;
    }
    f_url->setEnumStrings(st);
    if (f_url->text().isEmpty() && (!st.isEmpty()))
        f_url->setValue(st.first());
}
//=============================================================================
