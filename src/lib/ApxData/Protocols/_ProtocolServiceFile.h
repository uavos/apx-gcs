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
#ifndef ProtocolServiceFile_H
#define ProtocolServiceFile_H
#include "ProtocolBase.h"
#include <QtCore>
class ProtocolServiceNode;
class ProtocolServiceRequest;
//=============================================================================
class ProtocolServiceFile : public ProtocolBase
{
    Q_OBJECT
public:
    explicit ProtocolServiceFile(ProtocolServiceNode *node, quint16 cmdBase);

private:
    ProtocolServiceNode *node;
    quint16 cmd_file;
    quint16 cmd_read;
    quint16 cmd_write;

    enum FileCmd { _cmd_file = 0, _cmd_read, _cmd_write };

    QPointer<ProtocolServiceRequest> reqFileRead;
    QPointer<ProtocolServiceRequest> reqFileWrite;
    QPointer<ProtocolServiceRequest> reqRead;
    QPointer<ProtocolServiceRequest> reqWrite;

    QByteArray rdata, wdata; //all file data

    quint32 dataAddr;
    quint32 dataSize;
    quint8 dataCRC;

    bool request_download(void); //true if not done
    bool request_upload(void);   //true if not done

private slots:
    void serviceData(quint16 cmd, QByteArray data);

    void fileReadReply(QByteArray data);
    void readReply(QByteArray data);

    void fileWriteReply();
    void writeReply(QByteArray data);

    //export signals and slots
public slots:
    void download();
    void upload(QByteArray data);

signals:
    void fileReceived(QByteArray data);
    void fileUploaded();
};
//=============================================================================
#endif
