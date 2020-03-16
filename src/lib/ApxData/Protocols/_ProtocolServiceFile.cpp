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
#include "ProtocolServiceFile.h"
#include "ProtocolServiceNode.h"
#include "ProtocolServiceRequest.h"

#include <Xbus/XbusNodeFile.h>
//=============================================================================
ProtocolServiceFile::ProtocolServiceFile(ProtocolServiceNode *node, quint16 cmdBase)
    : ProtocolBase(node)
    , node(node)
    , reqFileRead(nullptr)
    , reqFileWrite(nullptr)
    , reqRead(nullptr)
    , reqWrite(nullptr)
    , dataAddr(0)
    , dataSize(0)
{
    cmd_file = cmdBase + _cmd_file;
    cmd_read = cmdBase + _cmd_read;
    cmd_write = cmdBase + _cmd_write;
    connect(node, &ProtocolServiceNode::unknownServiceData, this, &ProtocolServiceFile::serviceData);
}
//=============================================================================
void ProtocolServiceFile::download()
{
    node->setProgress(0);
    qDebug() << node->name();
    reqFileRead = node->request(cmd_file, QByteArray(), 1000, false);
    //req->rxCntFilter=sizeof(_flash_file);
    //connect(req,&ProtocolServiceRequest::requestReplyData,this,&ProtocolServiceFile::fileReadReply);
}
//=============================================================================
void ProtocolServiceFile::upload(QByteArray data)
{
    qDebug() << node->name();
    node->setProgress(0);
    wdata = data;
    dataSize = static_cast<uint>(data.size());
    xbus::node::file::file_t f;
    memset(&f, 0, sizeof(xbus::node::file::file_t));
    f.start_address = 0;
    f.size = dataSize;
    quint8 xor_crc = 0;
    for (int i = 0; i < data.size(); i++)
        xor_crc ^= static_cast<quint8>(wdata.at(i));
    f.xor_crc = xor_crc;

    QByteArray ba(f.psize(), '\0');
    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(ba.data()));
    f.write(&stream);

    reqFileWrite = node->request(cmd_file, ba, 3000, false);
    //req->rxCntFilter=0;
    //connect(req,&ProtocolServiceRequest::requestReplyData,this,&ProtocolServiceFile::fileWriteReply);
}
//=============================================================================
//=============================================================================
void ProtocolServiceFile::fileReadReply(QByteArray data)
{
    //qDebug()<<node->info.name<<data.toHex().toUpper();
    xbus::node::file::file_t f;
    XbusStreamReader stream(reinterpret_cast<const uint8_t *>(data.data()),
                            static_cast<uint16_t>(data.size()));
    f.read(&stream);

    if (f.size == 0) {
        qDebug() << "empty file";
        emit fileReceived(QByteArray());
        return;
    }
    //start reading flash blocks
    dataAddr = 0;
    dataSize = f.size;
    dataCRC = f.xor_crc;
    if (request_download())
        return;
    qDebug() << "empty file (hdr)";
    emit fileReceived(QByteArray());
}
//=============================================================================
void ProtocolServiceFile::fileWriteReply()
{
    //qDebug()<<node->info.name<<data.size();
    dataAddr = 0;
    if (request_upload())
        return;
    emit fileUploaded();
}
//=============================================================================
bool ProtocolServiceFile::request_download(void)
{
    if (dataAddr >= dataSize)
        return false;
    quint16 cnt = xbus::node::file::size_block;
    uint rcnt = dataSize - dataAddr;
    xbus::node::file::file_data_hdr_t hdr;
    hdr.start_address = dataAddr;
    hdr.data_size = cnt < rcnt ? cnt : static_cast<quint16>(rcnt);

    QByteArray ba(hdr.psize(), '\0');
    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(ba.data()));
    hdr.write(&stream);

    reqRead = node->request(cmd_read, ba, 1000, false);
    //connect(req,&ProtocolServiceRequest::requestReplyData,this,&ProtocolServiceFile::readReply);
    //qDebug()<<hdr.start_address<<hdr.data_size;
    node->setProgress(dataAddr * 100 / dataSize);
    return true;
}
bool ProtocolServiceFile::request_upload(void)
{
    if (dataAddr >= dataSize)
        return false;
    quint16 cnt = xbus::node::file::size_block;
    uint rcnt = dataSize - dataAddr;
    xbus::node::file::file_data_hdr_t hdr;
    hdr.start_address = dataAddr;
    hdr.data_size = cnt < rcnt ? cnt : static_cast<quint16>(rcnt);
    QByteArray blockData(wdata.mid(static_cast<int>(dataAddr), hdr.data_size));

    QByteArray ba(hdr.psize(), '\0');
    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(ba.data()));
    hdr.write(&stream);

    reqWrite = node->request(cmd_write, ba.append(blockData), 1000, false);
    //req->rxCntFilter=sizeof(_flash_data_hdr);
    //connect(req,&ProtocolServiceRequest::requestReplyData,this,&ProtocolServiceFile::writeReply);
    //qDebug()<<hdr.start_address<<hdr.data_size;
    node->setProgress(dataAddr * 100 / dataSize);
    return true;
} //=============================================================================
void ProtocolServiceFile::readReply(QByteArray data)
{
    //qDebug()<<node->info.name<<data.size();
    if (data.size() <= static_cast<int>(xbus::node::file::file_data_hdr_t::psize())) {
        qDebug() << "Wrong reply size" << data.size();
        return;
    }
    xbus::node::file::file_data_hdr_t hdr;
    XbusStreamReader stream(reinterpret_cast<const uint8_t *>(data.data()),
                            static_cast<uint16_t>(data.size()));
    hdr.read(&stream);

    if (hdr.start_address != dataAddr) {
        qDebug() << "Wrong addr" << hdr.start_address << dataAddr;
        return;
    }
    uint sz = static_cast<uint>(data.size()) - xbus::node::file::file_data_hdr_t::psize();
    if (sz != hdr.data_size) {
        qDebug() << "Wrong data size" << hdr.data_size << sz;
        return;
    }
    rdata.append(data.mid(xbus::node::file::file_data_hdr_t::psize()));
    dataAddr += hdr.data_size;
    if (request_download())
        return;
    //downloaded, check consistency
    quint8 xor_crc = 0;
    for (int i = 0; i < rdata.size(); i++)
        xor_crc ^= static_cast<quint8>(rdata.at(i));
    if (xor_crc != dataCRC) {
        qDebug() << "Wrong script_block CRC";
        rdata.clear();
        return;
    }
    //qDebug()<<"done";
    emit fileReceived(rdata);
}
//=============================================================================
void ProtocolServiceFile::writeReply(QByteArray data)
{
    //qDebug()<<node->info.name<<data.size();
    xbus::node::file::file_data_hdr_t hdr;
    XbusStreamReader stream(reinterpret_cast<const uint8_t *>(data.data()),
                            static_cast<uint16_t>(data.size()));
    hdr.read(&stream);

    if (hdr.start_address != dataAddr) {
        qDebug() << "Wrong addr" << hdr.start_address << dataAddr;
        return;
    }
    dataAddr += hdr.data_size;
    if (request_upload())
        return;
    //qDebug()<<"done";
    emit fileUploaded();
}
//=============================================================================
//=============================================================================
void ProtocolServiceFile::serviceData(quint16 cmd, QByteArray data)
{
    if (cmd == cmd_file) {
        if (reqFileRead) {
            if (data.size() != xbus::node::file::file_t::psize())
                return;
            ProtocolServiceRequest *r = reqFileRead;
            reqFileRead = nullptr;
            fileReadReply(data);
            r->finish();
        } else if (reqFileWrite) {
            if (data.size() != 0)
                return;
            ProtocolServiceRequest *r = reqFileWrite;
            reqFileWrite = nullptr;
            fileWriteReply();
            r->finish();
        }
    } else if (cmd == cmd_read) {
        if (reqRead) {
            if (data.size() <= static_cast<int>(xbus::node::file::file_data_hdr_t::psize()))
                return;
            if (reqRead->data != data.left(reqRead->data.size()))
                return;
            ProtocolServiceRequest *r = reqRead;
            reqRead = nullptr;
            readReply(data);
            r->finish();
        }
    } else if (cmd == cmd_write) {
        if (reqWrite) {
            if (data.size() != xbus::node::file::file_data_hdr_t::psize())
                return;
            if (data != reqWrite->data.left(data.size()))
                return;
            ProtocolServiceRequest *r = reqWrite;
            reqWrite = nullptr;
            writeReply(data);
            r->finish();
        }
    }
}
//=============================================================================
//=============================================================================
