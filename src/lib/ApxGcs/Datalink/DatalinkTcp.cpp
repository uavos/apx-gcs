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
    : DatalinkSocket(parent, socket, socket->peerAddress(), socket->peerPort(), rxNetwork, txNetwork)
    , _tcp(socket)
    , serverName(App::username())
{
    _connectionType = _tcp->isOpen() ? HTTP_RESPONSE : HTTP_CLIENT;

    _tcp->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

    if (_connectionType == HTTP_RESPONSE) {
        setUrl(_tcp->peerAddress().toString());
        setStatus("Waiting request");
        connect(_tcp, &QTcpSocket::readyRead, this, &DatalinkTcp::readyReadHeader);
    } else {
        connect(_tcp, &QTcpSocket::connected, this, &DatalinkTcp::requestDatalinkHeader);
    }
}

void DatalinkTcp::resetDataStream()
{
    DatalinkConnection::resetDataStream();
    data.datalink = false;
}

void DatalinkTcp::connectToHost(QHostAddress host, quint16 port)
{
    if (_connectionType == HTTP_CLIENT) {
        _hostAddress = host;
        _hostPort = port;
        if (_tcp->isOpen())
            _tcp->abort();
        connect(_tcp, &QTcpSocket::readyRead, this, &DatalinkTcp::readyReadHeader);
        _tcp->connectToHost(host, port);
    }
}

void DatalinkTcp::socketDisconnected()
{
    DatalinkSocket::socketDisconnected();

    //qDebug()<<_connectionType;
    disconnect(_tcp, &QTcpSocket::readyRead, this, &DatalinkTcp::readyReadHeader);
    disconnect(_tcp, &QTcpSocket::readyRead, this, &DatalinkTcp::readDataAvailable);

    if (_connectionType == HTTP_RESPONSE) {
        disconnect(_tcp, nullptr, this, nullptr);
        _tcp->deleteLater();
        deleteFact();
    }
}

void DatalinkTcp::readyReadHeader()
{
    // qDebug() << _tcp->bytesAvailable() << _tcp->canReadLine();

    if (!readHeader())
        return;
    if (!checkHeader())
        return;
    if (!data.datalink)
        return;
    setStatus("Datalink");
    opened();
    disconnect(_tcp, &QTcpSocket::readyRead, this, &DatalinkTcp::readyReadHeader);
    connect(_tcp, &QTcpSocket::readyRead, this, &DatalinkTcp::readDataAvailable);
    readDataAvailable();
}

bool DatalinkTcp::checkHeader()
{
    switch (_connectionType) {
    case HTTP_RESPONSE:
        return checkServerRequestHeader();
    case HTTP_CLIENT:
        return checkDatalinkResponseHeader();
    default:
        return true;
    }
}

bool DatalinkTcp::readHeader()
{
    if (data.datalink)
        return true;
    if (!_tcp->canReadLine())
        return false;
    bool reqDone = false;
    while (_tcp->canReadLine()) {
        QString line = _tcp->readLine();
        // qDebug() << "line:" << line.trimmed();
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
    for (const auto i : hdr) {
        if (i.contains(':'))
            data.hdr_hash.insert(i.left(i.indexOf(':')).trimmed().toLower(),
                                 i.mid(i.indexOf(':') + 1).trimmed());
    }
    // qDebug() << "hdr:" << hdr;
    if (!data.hdr.isEmpty())
        return true;
    //error
    _tcp->close();
    return false;
}

QByteArray DatalinkTcp::read()
{
    if (!data.datalink)
        return {};
    if (!(_tcp && _tcp->isOpen()))
        return {};
    return _tcp->read(xbus::size_packet_max * 2);
}

void DatalinkTcp::write(const QByteArray &packet)
{
    // qDebug() << "write:" << packet.size();
    if (!data.datalink)
        return;
    if (!(_tcp && _tcp->isOpen()))
        return;
    _tcp->write(packet);
}

void DatalinkTcp::requestDatalinkHeader()
{
    setStatus("Requesting");
    data.datalink = false;
    data.hdr.clear();
    QTextStream stream(_tcp);
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
            QTextStream stream(_tcp);
            stream.setAutoDetectUnicode(true);
            if (req == "/datalink") {
                stream << "HTTP/1.0 200 OK\r\n";
                stream << "Content-Type: application/octet-stream\r\n";
                stream << QString("Server: %1\r\n").arg(serverName);
                stream << "\r\n";
                stream.flush();
                QString sname;
                if (isLocalHost(_tcp->peerAddress()))
                    sname = "localhost";
                else
                    sname = _tcp->peerAddress().toString();
                //sname+=":"+QString::number(_tcp->peerPort());
                if (data.hdr_hash.contains("from"))
                    sname.prepend(data.hdr_hash.value("from") + "@");
                /*if(_tcp->peerAddress()==extSocket->peerAddress()){
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
            setStatus("HTTP");
            emit httpRequest(stream, req, _tcp);
            stream.flush();
            _tcp->close();
            return false;
        }
        //unknown request type
        break;
    }
    //error
    _tcp->close();
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
        QString sname = _tcp->peerAddress().toString() + ":" + QString::number(_tcp->peerPort());
        if (data.hdr_hash.contains("server"))
            sname.prepend(data.hdr_hash.value("server") + "@");
        resetDataStream();
        data.datalink = true;
        apxMsg() << QString("#%1: %2").arg(tr("server connected")).arg(sname);
        return true;
    }
    //error
    _tcp->close();
    return false;
}
