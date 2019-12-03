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
#include "DatalinkTcpSocket.h"
#include "Datalink.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <crc/crc.h>
//=============================================================================
DatalinkTcpSocket::DatalinkTcpSocket(Fact *parent,
                                     QTcpSocket *socket,
                                     quint16 rxNetwork,
                                     quint16 txNetwork)
    : DatalinkConnection(parent, "socket#", "", "", rxNetwork, txNetwork)
    , socket(socket)
    , serverName(App::username())
{
    _serverClient = socket->isOpen();

    hostAddress = socket->peerAddress();
    hostPort = socket->peerPort();

    resetDataStream();

    socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

    connect(socket,
            &QTcpSocket::disconnected,
            this,
            &DatalinkTcpSocket::socketDisconnected,
            Qt::QueuedConnection);
    connect(socket,
            static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(
                &QAbstractSocket::error),
            this,
            &DatalinkTcpSocket::socketError);
    connect(socket,
            static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(
                &QAbstractSocket::error),
            this,
            &DatalinkTcpSocket::close,
            Qt::QueuedConnection);
    connect(socket, &QTcpSocket::stateChanged, this, &DatalinkTcpSocket::socketStateChanged);

    if (_serverClient) {
        setStatus("Waiting request");
        connect(socket, &QTcpSocket::readyRead, this, &DatalinkTcpSocket::readyReadHeader);
    } else {
        connect(socket, &QTcpSocket::connected, this, &DatalinkTcpSocket::requestDatalinkHeader);
    }
}
//=============================================================================
void DatalinkTcpSocket::resetDataStream()
{
    data.size = 0;
    data.crc16 = 0;
    data.datalink = false;
}
//=============================================================================
void DatalinkTcpSocket::connectToHost(QHostAddress host, quint16 port)
{
    if (_serverClient)
        return;
    hostAddress = host;
    hostPort = port;
    if (socket->isOpen())
        socket->abort();
    connect(socket, &QTcpSocket::readyRead, this, &DatalinkTcpSocket::readyReadHeader);
    socket->connectToHost(host, port);
}
//=============================================================================
void DatalinkTcpSocket::close()
{
    //qDebug()<<active();
    //if(active())socket->disconnectFromHost();
    socket->abort();
    resetDataStream();
}
//=============================================================================
void DatalinkTcpSocket::socketDisconnected()
{
    //qDebug()<<_serverClient;
    resetDataStream();
    disconnect(socket, &QTcpSocket::readyRead, this, &DatalinkTcpSocket::readyReadHeader);
    disconnect(socket, &QTcpSocket::readyRead, this, &DatalinkTcpSocket::readDataAvailable);
    closed();
    emit disconnected();
    if (_serverClient) {
        disconnect(socket, nullptr, this, nullptr);
        socket->deleteLater();
        //parentItem()->removeItem(this,false);
        //deleteLater();
        remove();
    }
}
//=============================================================================
void DatalinkTcpSocket::socketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    if (data.datalink) {
        apxMsg() << QString("#%1 (%2:%3)")
                        .arg(socket->errorString())
                        .arg(socket->peerAddress().toString())
                        .arg(socket->peerPort());
    }
    setStatus("Error");
    //close();
    emit error();
}
//=============================================================================
void DatalinkTcpSocket::readyReadHeader()
{
    if (!readHeader())
        return;
    if (!checkHeader())
        return;
    if (!data.datalink)
        return;
    setStatus("Datalink");
    opened();
    disconnect(socket, &QTcpSocket::readyRead, this, &DatalinkTcpSocket::readyReadHeader);
    connect(socket, &QTcpSocket::readyRead, this, &DatalinkTcpSocket::readDataAvailable);
    readDataAvailable();
}
//=============================================================================
bool DatalinkTcpSocket::checkHeader()
{
    if (_serverClient)
        return checkServerRequestHeader();
    else
        return checkDatalinkResponseHeader();
}
//=============================================================================
bool DatalinkTcpSocket::readHeader()
{
    if (data.datalink)
        return true;
    if (!socket->canReadLine())
        return false;
    bool reqDone = false;
    while (socket->canReadLine()) {
        QString line = socket->readLine();
        //qDebug()<<"line:"<<line.trimmed();
        //qDebug()<<"line:"<<QByteArray(line.toUtf8()).toHex().toUpper();
        if (!line.trimmed().isEmpty()) {
            data.hdr.append(line.trimmed());
        } else {
            reqDone = true;
            break;
        }
    }
    if (!reqDone)
        return false;
    //header received
    const QStringList &hdr = data.hdr;
    foreach (QString s, hdr) {
        if (s.contains(':'))
            data.hdr_hash.insert(s.left(s.indexOf(':')).trimmed().toLower(),
                                 s.mid(s.indexOf(':') + 1).trimmed());
    }
    //qDebug()<<"hdr:"<<hdr;
    if (!data.hdr.isEmpty())
        return true;
    //error
    socket->close();
    return false;
}
//=============================================================================
//=============================================================================
QByteArray DatalinkTcpSocket::read()
{
    qint64 cnt = socket->bytesAvailable();
    QByteArray packet;
    while (cnt > 0) {
        quint16 sz = data.size;
        quint16 crc16 = data.crc16;
        if (sz == 0) {
            if (cnt < static_cast<qint64>(sizeof(sz) + sizeof(crc16))) {
                cnt = 0;
                break;
            }
            socket->read(reinterpret_cast<char *>(&sz), sizeof(sz));
            socket->read(reinterpret_cast<char *>(&crc16), sizeof(crc16));
            if (sz > 2048) {
                apxConsoleW() << "tcp sz error:" << socket->peerAddress().toString()
                              << QString("(%1)").arg(sz);
                cnt = -1;
                break;
            }
            data.size = sz;
            data.crc16 = crc16;
        }
        if (cnt < static_cast<qint64>(sz)) {
            cnt = 0;
            break;
        }
        cnt -= sz;
        data.size = 0;
        packet = socket->read(sz);
        if (packet.size() != sz) {
            apxConsoleW() << "tcp read error:" << socket->peerAddress().toString()
                          << QString("(%1/%2)").arg(packet.size()).arg(sz);
            cnt = -1;
            break;
        }
        if (crc16
            != CRC_16_IBM(reinterpret_cast<const uint8_t *>(packet.data()),
                          static_cast<quint16>(packet.size()),
                          0xFFFF)) {
            apxConsoleW() << "tcp crc error:"
                          << QString("%1:%2")
                                 .arg(socket->peerAddress().toString())
                                 .arg(socket->peerPort());
            //apxConsole()<<sz<<packet.toHex().toUpper();
            cnt = -1;
            break;
        }
        break;
    }
    if (cnt < 0) {
        //error
        resetDataStream();
        socket->disconnectFromHost();
        return QByteArray();
    }
    if (cnt > 0)
        QTimer::singleShot(10, this, &DatalinkTcpSocket::readDataAvailable);
    return packet;
}
//=============================================================================
void DatalinkTcpSocket::write(const QByteArray &packet)
{
    if (!data.datalink)
        return;
    if (!(socket && socket->isOpen()))
        return;
    socket->write(makeTcpPacket(packet));
}
//=============================================================================
//=============================================================================
void DatalinkTcpSocket::requestDatalinkHeader()
{
    setStatus("Requesting");
    data.size = 0;
    data.datalink = false;
    data.hdr.clear();
    QTextStream stream(socket);
    stream << "GET /datalink HTTP/1.0\r\n";
    if (!requestHdrHostName.isEmpty())
        stream << QString("Host: %1\r\n").arg(requestHdrHostName);
    stream << QString("From: %1\r\n").arg(serverName);
    stream << "\r\n";
    stream.flush();
}
//=============================================================================
bool DatalinkTcpSocket::checkServerRequestHeader()
{
    if (data.datalink)
        return true;
    //request for service
    while (1) {
        if (data.hdr.isEmpty())
            break;
        QStringList rlist = data.hdr.at(0).simplified().split(' ');
        if (rlist.size() < 2)
            break;
        if (rlist.at(0) == "GET") {
            QString req = QUrl::fromPercentEncoding(rlist.at(1).toUtf8());
            QTextStream stream(socket);
            stream.setAutoDetectUnicode(true);
            if (req == "/datalink") {
                stream << "HTTP/1.0 200 OK\r\n";
                stream << "Content-Type: application/octet-stream\r\n";
                stream << QString("Server: %1\r\n").arg(serverName);
                stream << "\r\n";
                stream.flush();
                QString sname;
                if (isLocalHost(socket->peerAddress()))
                    sname = "localhost";
                else
                    sname = socket->peerAddress().toString();
                //sname+=":"+QString::number(socket->peerPort());
                if (data.hdr_hash.contains("from"))
                    sname.prepend(data.hdr_hash.value("from") + "@");
                /*if(socket->peerAddress()==extSocket->peerAddress()){
          apxConsoleW()<<tr("Client connection refused");
          break;
        }*/
                data.datalink = true;
                data.size = 0;
                data.crc16 = 0;
                /*if(connections.size()>1)sname.append(QString(" [%1]").arg(connections.size()));
        apxMsg("#%s: %s",tr("client").toUtf8().data(),sname.toUtf8().data());
        if(!extctrEnabled()){
          if(data.local)apxMsg("%s",tr("Local client controls enabled").toUtf8().data());
          else apxMsgW("%s",tr("External client controls disabled").toUtf8().data());
        }*/
                apxMsg() << QString("#%1: %2").arg(tr("client")).arg(sname);
                setTitle(sname);
                return true;
            }
            //generic request
            bool ok = false;
            setStatus("HTTP");
            emit httpRequest(stream, req, &ok);
            if (!ok) {
                stream << "HTTP/1.1 404 Not Found\r\n";
                stream << "Content-Type: text/html; charset=\"utf-8\"\r\n";
                stream << "\r\n";
                stream << QString("<b>GCS HTTP Server</b> (%1:%2)")
                              .arg(socket->localAddress().toString())
                              .arg(socket->localPort());
                if (req != "/")
                    stream << QString("<br>No service for '%1'").arg(req);
                stream << "<hr size=1>";
                stream << QString("<a href=%1>%1</a> - %2<br>").arg("/kml").arg("Google Earth KML");
                stream << QString("<a href=%1>%1</a> - %2<br>")
                              .arg("/datalink")
                              .arg("Datalink stream [uint16 packet size][CRC_16_IBM][packet data]");
                stream << QString("<a href=%1>%1</a> - %2<br>")
                              .arg("/mandala")
                              .arg("Mandala XML data and commands");
                stream << QString("<br>More info here: <a href=%1>%1</a>")
                              .arg("http://docs.uavos.com/sw/comm");
            }
            stream.flush();
            socket->close();
            return false;
        }
        //unknown request type
        break;
    }
    //error
    socket->close();
    return false;
}
//=============================================================================
bool DatalinkTcpSocket::checkDatalinkResponseHeader()
{
    if (data.datalink)
        return true;
    //response for GET /datalink
    while (1) {
        if (!data.hdr.at(0).contains("200 OK", Qt::CaseInsensitive)) {
            apxConsoleW() << data.hdr.at(0);
            break;
        }
        if (!data.hdr_hash.value("content-type")
                 .contains("application/octet-stream", Qt::CaseInsensitive)) {
            apxConsoleW() << data.hdr_hash.value("content-type");
            break;
        }
        QString sname = socket->peerAddress().toString() + ":"
                        + QString::number(socket->peerPort());
        if (data.hdr_hash.contains("server"))
            sname.prepend(data.hdr_hash.value("server") + "@");
        data.datalink = true;
        data.size = 0;
        data.crc16 = 0;
        apxMsg() << QString("#%1: %2").arg(tr("server connected")).arg(sname);
        return true;
    }
    //error
    socket->close();
    return false;
}
//=============================================================================
QByteArray DatalinkTcpSocket::makeTcpPacket(const QByteArray &ba) const
{
    quint16 sz = static_cast<quint16>(ba.size());
    quint16 crc16 = CRC_16_IBM(reinterpret_cast<const uint8_t *>(ba.data()), sz, 0xFFFF);
    QByteArray tcpData;
    tcpData.append(reinterpret_cast<const char *>(&sz), sizeof(sz));
    tcpData.append(reinterpret_cast<const char *>(&crc16), sizeof(crc16));
    tcpData.append(ba);
    return tcpData;
}
//=============================================================================
bool DatalinkTcpSocket::isLocalHost(const QHostAddress address)
{
    //qDebug()<<QNetworkInterface::allAddresses();
    if (address.isLoopback())
        return true;
    foreach (const QHostAddress &a, QNetworkInterface::allAddresses())
        if (address.isEqual(a))
            return true;
    return false;
}
//=============================================================================
void DatalinkTcpSocket::socketStateChanged(QAbstractSocket::SocketState socketState)
{
    QString s;
    switch (socketState) {
    //default:
    case QAbstractSocket::UnconnectedState:
        break;
    case QAbstractSocket::HostLookupState:
        s = "Lookup";
        break;
    case QAbstractSocket::ConnectingState:
        s = "Connecting";
        break;
    case QAbstractSocket::ConnectedState:
        s = "Connected";
        break;
    case QAbstractSocket::BoundState:
        s = "Bound";
        break;
    case QAbstractSocket::ClosingState:
        s = "Closing";
        break;
    case QAbstractSocket::ListeningState:
        s = "Listening";
        break;
    }
    setStatus(s);
}
//=============================================================================
//=============================================================================
