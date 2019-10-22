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
#include "AppInstances.h"
#include <App/AppLog.h>
#include <QLocalSocket>
const char *AppInstances::ack = "ack";
//=============================================================================
AppInstances::AppInstances(QObject *parent)
    : QObject(parent)
    , lockFile(QDir::temp().absoluteFilePath(QCoreApplication::applicationName() + ".lock"))
    , m_client(false)
{
    socketName = QCoreApplication::applicationName();
    lockFile.setStaleLockTime(0);
    if (lockFile.tryLock(0)) {
        QLocalServer::removeServer(socketName);
        localServer.listen(socketName);
    } else {
        //try to connect to server
        QLocalSocket socket;
        socket.connectToServer(socketName);
        if (socket.waitForConnected(1000)) {
            m_client = true;
        } else {
            //recover server from crash
            if (!lockFile.tryLock(0)) {
                lockFile.removeStaleLockFile();
                qWarning() << "lock file removed";
            }
            if (lockFile.tryLock(0)) {
                QLocalServer::removeServer(socketName);
                localServer.listen(socketName);
            } else {
                qWarning() << "lock file recovery failed";
            }
        }
    }
    if (m_client) {
        apxMsg() << tr("Client application mode");
    } else if (localServer.isListening()) {
        connect(&localServer, &QLocalServer::newConnection, this, &AppInstances::newConnection);
    } else {
        apxMsg() << tr("Local server error") << localServer.errorString();
    }
}
//=============================================================================
bool AppInstances::isClient()
{
    return m_client;
}
//=============================================================================
void AppInstances::newConnection()
{
    QLocalSocket *socket = localServer.nextPendingConnection();
    if (!socket)
        return;

    // Why doesn't Qt have a blocking stream that takes care of this shait???
    while (socket->bytesAvailable() < static_cast<int>(sizeof(quint32))
           && socket->state() == QLocalSocket::ConnectedState) {
        if (!socket->isValid()) // stale request
            return;
        socket->waitForReadyRead(1000);
    }
    QDataStream ds(socket);
    QByteArray uMsg;
    quint32 remaining;
    ds >> remaining;
    uMsg.resize(remaining);
    int got = 0;
    char *uMsgBuf = uMsg.data();
    //qDebug() << "RCV: remaining" << remaining;
    do {
        got = ds.readRawData(uMsgBuf, remaining);
        remaining -= got;
        uMsgBuf += got;
        //qDebug() << "RCV: got" << got << "remaining" << remaining;
    } while (remaining && got >= 0 && socket->waitForReadyRead(2000));
    //### error check: got<0
    if (got < 0) {
        qWarning() << "Message reception failed" << socket->errorString();
        delete socket;
        return;
    }
    // ### async this
    QString message = QString::fromUtf8(uMsg.constData(), uMsg.size());
    socket->write(ack, qstrlen(ack));
    socket->waitForBytesWritten(1000);
    emit messageReceived(message); // ##(might take a long time to return)
}
//=============================================================================
bool AppInstances::sendMessage(const QString &message, int timeout, bool block)
{
    if (!isClient())
        return false;

    QLocalSocket socket;
    bool connOk = false;
    for (int i = 0; i < 2; i++) {
        // Try twice, in case the other instance is just starting up
        socket.connectToServer(socketName);
        connOk = socket.waitForConnected(timeout / 2);
        if (connOk || i)
            break;
        int ms = 250;
#if defined(Q_OS_WIN)
        Sleep(DWORD(ms));
#else
        struct timespec ts = {ms / 1000, (ms % 1000) * 1000 * 1000};
        nanosleep(&ts, NULL);
#endif
    }
    if (!connOk)
        return false;

    QByteArray uMsg(message.toUtf8());
    QDataStream ds(&socket);
    ds.writeBytes(uMsg.constData(), uMsg.size());
    bool res = socket.waitForBytesWritten(timeout);
    res &= socket.waitForReadyRead(timeout); // wait for ack
    res &= (socket.read(qstrlen(ack)) == ack);
    if (block) // block until peer disconnects
        socket.waitForDisconnected(-1);
    return res;
}
//=============================================================================
