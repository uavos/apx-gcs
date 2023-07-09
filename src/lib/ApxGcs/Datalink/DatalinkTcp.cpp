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
#include "DatalinkTcp.h"
#include "Datalink.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <crc.h>

DatalinkTcp::DatalinkTcp(Fact *parent, QTcpSocket *socket, quint16 rxNetwork, quint16 txNetwork)
    : DatalinkConnection(parent, "tcp#", "", "", rxNetwork, txNetwork)
    , socket(socket)
    , serverName(App::username())
{
    setEncoder(&_encoder);
    setDecoder(&_decoder);

    _serverClient = socket->isOpen();

    hostAddress = socket->peerAddress();
    hostPort = socket->peerPort();

    resetDataStream();

    socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

    connect(socket,
            &QTcpSocket::disconnected,
            this,
            &DatalinkTcp::socketDisconnected,
            Qt::QueuedConnection);
    connect(socket,
            static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(
                &QAbstractSocket::errorOccurred),
            this,
            &DatalinkTcp::socketError);
    connect(socket,
            static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(
                &QAbstractSocket::errorOccurred),
            this,
            &DatalinkTcp::close,
            Qt::QueuedConnection);
    connect(socket, &QTcpSocket::stateChanged, this, &DatalinkTcp::socketStateChanged);

    if (_serverClient) {
        setUrl(socket->peerAddress().toString());
        setStatus("Waiting request");
        connect(socket, &QTcpSocket::readyRead, this, &DatalinkTcp::readyReadHeader);
    } else {
        connect(socket, &QTcpSocket::connected, this, &DatalinkTcp::requestDatalinkHeader);
    }
}

void DatalinkTcp::resetDataStream()
{
    DatalinkConnection::resetDataStream();
    data.datalink = false;
}

void DatalinkTcp::connectToHost(QHostAddress host, quint16 port)
{
    if (_serverClient)
        return;
    hostAddress = host;
    hostPort = port;
    if (socket->isOpen())
        socket->abort();
    connect(socket, &QTcpSocket::readyRead, this, &DatalinkTcp::readyReadHeader);
    socket->connectToHost(host, port);
}

void DatalinkTcp::close()
{
    //qDebug()<<active();
    //if(active())socket->disconnectFromHost();
    socket->abort();
    resetDataStream();
}

void DatalinkTcp::socketDisconnected()
{
    //qDebug()<<_serverClient;
    resetDataStream();
    disconnect(socket, &QTcpSocket::readyRead, this, &DatalinkTcp::readyReadHeader);
    disconnect(socket, &QTcpSocket::readyRead, this, &DatalinkTcp::readDataAvailable);
    closed();
    emit disconnected();
    if (_serverClient) {
        disconnect(socket, nullptr, this, nullptr);
        socket->deleteLater();
        //parentItem()->removeItem(this,false);
        //deleteLater();
        deleteFact();
    }
}

void DatalinkTcp::socketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    if (data.datalink) {
        apxMsg() << QString("#%1 (%2:%3)")
                        .arg(socket->errorString())
                        .arg(socket->peerAddress().toString())
                        .arg(socket->peerPort());
    }
    setStatus("Error");

    emit error();
}

void DatalinkTcp::readyReadHeader()
{
    if (!readHeader())
        return;
    if (!checkHeader())
        return;
    if (!data.datalink)
        return;
    setStatus("Datalink");
    opened();
    disconnect(socket, &QTcpSocket::readyRead, this, &DatalinkTcp::readyReadHeader);
    connect(socket, &QTcpSocket::readyRead, this, &DatalinkTcp::readDataAvailable);
    readDataAvailable();
}

bool DatalinkTcp::checkHeader()
{
    if (_serverClient)
        return checkServerRequestHeader();
    else
        return checkDatalinkResponseHeader();
}

bool DatalinkTcp::readHeader()
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

QByteArray DatalinkTcp::read()
{
    if (!data.datalink)
        return {};
    if (!(socket && socket->isOpen()))
        return {};
    return socket->read(xbus::size_packet_max * 2);
}

void DatalinkTcp::write(const QByteArray &packet)
{
    if (!data.datalink)
        return;
    if (!(socket && socket->isOpen()))
        return;
    socket->write(packet);
}

void DatalinkTcp::requestDatalinkHeader()
{
    setStatus("Requesting");
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

bool DatalinkTcp::checkServerRequestHeader()
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
                /*if(connections.size()>1)sname.append(QString(" [%1]").arg(connections.size()));
                apxMsg("#%s: %s",tr("client").toUtf8().data(),sname.toUtf8().data());
                if(!extctrEnabled()){
                if(data.local)apxMsg("%s",tr("Local client controls enabled").toUtf8().data());
                else apxMsgW("%s",tr("External client controls disabled").toUtf8().data());
                }*/
                apxMsg() << QString("#%1: %2").arg(tr("client")).arg(sname);
                setUrl(sname);
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
                              .arg("https://docs.uavos.com/");
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

bool DatalinkTcp::checkDatalinkResponseHeader()
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
        resetDataStream();
        apxMsg() << QString("#%1: %2").arg(tr("server connected")).arg(sname);
        return true;
    }
    //error
    socket->close();
    return false;
}

bool DatalinkTcp::isLocalHost(const QHostAddress address)
{
    //qDebug()<<QNetworkInterface::allAddresses();
    if (address.isLoopback())
        return true;
    foreach (const QHostAddress &a, QNetworkInterface::allAddresses())
        if (address.isEqual(a))
            return true;
    return false;
}

void DatalinkTcp::socketStateChanged(QAbstractSocket::SocketState socketState)
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
