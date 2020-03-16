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
#ifndef ProtocolServiceFirmware_H
#define ProtocolServiceFirmware_H
#include "ProtocolBase.h"
#include <QtCore>
class ProtocolService;
class ProtocolServiceNode;
class ProtocolServiceRequest;
//=============================================================================
class ProtocolServiceFirmware : public ProtocolBase
{
    Q_OBJECT
public:
    explicit ProtocolServiceFirmware(ProtocolService *service);

private:
    ProtocolService *service;

    QPointer<ProtocolServiceRequest> reqInit;
    QPointer<ProtocolServiceRequest> reqFileWrite;
    QPointer<ProtocolServiceRequest> reqWrite;

    QString sn;
    quint32 startAddr;
    QByteArray wdata; //all file data

    quint32 dataAddr;
    quint32 dataSize;
    quint8 dataCRC;

    int reqLoaderRetry;
    int writeRetry;

    quint16 ncmd;
    ProtocolServiceRequest *request(quint16 cmd, const QByteArray &data, int timeout_ms, int retry);
    ProtocolServiceRequest *ldr_req(quint16 cmd, const QByteArray &data, int timeout_ms, int retry);

    void finish(bool success);

private slots:
    void loaderServiceData(QString sn, quint16 cmd, QByteArray data);

    void initReply(QByteArray data);
    void fileWriteReply(QByteArray data);
    void writeReply(QByteArray data);

    void retrying(int retry, int cnt);

    void requestLoaderReboot();
    void requestLoaderRebootCheck();
    void requestLoaderInit();
    void requestFileWrite();
    bool requestWrite(void); //true if not done

    void error();

    void restart();

    //export signals and slots
public slots:
    void stop();
    void upgradeFirmware(QString sn, QByteArray data, quint32 startAddr);
    void upgradeLoader(QString sn, QByteArray data, quint32 startAddr);

signals:
    void started(QString sn);
    void finished(QString sn, bool success);
};
//=============================================================================
#endif
