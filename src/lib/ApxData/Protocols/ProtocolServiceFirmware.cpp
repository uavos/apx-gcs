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
#include "ProtocolServiceFirmware.h"
#include "ProtocolService.h"
#include "ProtocolServiceNode.h"
#include "ProtocolServiceRequest.h"

#include <App/AppLog.h>

#include <Xbus/XbusNode.h>
#include <Xbus/XbusNodeFile.h>
//=============================================================================
ProtocolServiceFirmware::ProtocolServiceFirmware(ProtocolService *service)
    : ProtocolBase(service)
    , service(service)
    , reqInit(nullptr)
    , reqFileWrite(nullptr)
    , reqWrite(nullptr)
    , dataAddr(0)
    , dataSize(0)
{
    connect(service,
            &ProtocolService::loaderServiceData,
            this,
            &ProtocolServiceFirmware::loaderServiceData);
}
//=============================================================================
void ProtocolServiceFirmware::upgradeFirmware(QString sn, QByteArray data, quint32 startAddr)
{
    if (data.isEmpty()) {
        qWarning() << "empty data" << sn;
        return;
    }
    this->sn = sn;
    this->wdata = data;
    this->startAddr = startAddr;
    this->dataSize = static_cast<quint32>(data.size());

    /*QFile f("/Users/uavinda/Documents/fwtest.bin");
  if(f.open(QFile::WriteOnly)){
    f.write(data);
    f.close();
  }*/

    //qDebug()<<data.toHex().toUpper();

    setProgress(0);
    setStatus(tr("Initializing").append("..."));
    service->nodeUpgrading(sn);
    emit started(sn);
    ncmd = xbus::node::apc_loader;
    reqLoaderRetry = 50;
    writeRetry = 5;
    requestLoaderReboot();
}
//=============================================================================
void ProtocolServiceFirmware::upgradeLoader(QString sn, QByteArray data, quint32 startAddr)
{
    if (data.isEmpty()) {
        qWarning() << "empty data" << sn;
        return;
    }
    this->sn = sn;
    this->wdata = data;
    this->startAddr = startAddr;
    this->dataSize = static_cast<quint32>(data.size());

    setProgress(0);
    setStatus(tr("Initializing").append("..."));
    service->nodeUpgrading(sn);
    emit started(sn);
    ncmd = xbus::node::apc_loader;
    writeRetry = 5;
    requestLoaderInit();
}
//=============================================================================
void ProtocolServiceFirmware::stop()
{
    apxMsgW() << tr("Upgrade interrupted");
    finish(false);
}
//=============================================================================
//=============================================================================
void ProtocolServiceFirmware::finish(bool success)
{
    if (success) {
        setStatus(tr("Firmware upgraded"));
        apxMsg() << status();
    } else {
        setStatus(tr("Firmware upgrade error"));
        apxMsgW() << status();
    }
    setProgress(-1);
    service->nodeUpgradingFinished(sn);
    if (reqInit)
        reqInit->finish();
    if (reqFileWrite)
        reqFileWrite->finish();
    if (reqWrite)
        reqWrite->finish();
    reqInit = nullptr;
    reqFileWrite = nullptr;
    reqWrite = nullptr;
    emit finished(sn, success);
}
void ProtocolServiceFirmware::error()
{
    finish(false);
}
void ProtocolServiceFirmware::restart()
{
    if (writeRetry <= 0)
        return;
    writeRetry--;
    requestFileWrite();
}
//=============================================================================
//=============================================================================
void ProtocolServiceFirmware::loaderServiceData(QString sn, quint16 cmd, QByteArray data)
{
    //qDebug()<<sn<<cmd<<data.toHex().toUpper();
    if (sn != this->sn)
        return;
    switch (cmd) {
    case xbus::node::ldc_init: {
        ProtocolServiceRequest *r = reqInit;
        reqInit = nullptr;
        if (!r)
            break;
        if (data.size() != xbus::node::file::file_t::psize()) {
            qWarning() << "Wrong ldc_init response" << data.size();
            break;
        }
        initReply(data);
        r->finish();
    } break;
    case xbus::node::ldc_file: {
        ProtocolServiceRequest *r = reqFileWrite;
        reqFileWrite = nullptr;
        if (!r)
            break;
        if (data.size() != xbus::node::file::file_t::psize()) {
            qWarning() << "Wrong ldc_file response" << data.size();
            break;
        }
        if (data != r->data.mid(1, data.size())) {
            qWarning() << "Wrong ldc_file response data";
            break;
        }
        fileWriteReply(data);
        r->finish();
    } break;
    case xbus::node::ldc_write: {
        ProtocolServiceRequest *r = reqWrite;
        reqWrite = nullptr;
        if (!r)
            break;
        if (data.size() != xbus::node::file::file_data_hdr_t::psize()) {
            qWarning() << "Wrong ldc_write response" << data.size();
            break;
        }
        if (data != r->data.mid(1, data.size())) {
            qWarning() << "Wrong ldc_write sequence";
            break;
        }
        writeReply(data);
        r->finish();
    } break;
    }
}
//=============================================================================
//=============================================================================
ProtocolServiceRequest *ProtocolServiceFirmware::request(quint16 cmd,
                                                         const QByteArray &data,
                                                         int timeout_ms,
                                                         int retry)
{
    //qDebug()<<sn<<cmd<<data.toHex().toUpper();
    return service->request(sn, cmd, data, timeout_ms, true, retry);
}
ProtocolServiceRequest *ProtocolServiceFirmware::ldr_req(quint16 cmd,
                                                         const QByteArray &data,
                                                         int timeout_ms,
                                                         int retry)
{
    QByteArray ba;
    ba.append(static_cast<char>(cmd));
    ba.append(data);
    return request(ncmd, ba, timeout_ms, retry);
}
//=============================================================================
void ProtocolServiceFirmware::retrying(int retry, int cnt)
{
    setStatus(QString("%1... %2/%3").arg(tr("Retrying")).arg(retry).arg(cnt));
    qWarning() << status();
}
//=============================================================================
//=============================================================================
void ProtocolServiceFirmware::requestLoaderReboot()
{
    //qDebug()<<reqLoaderRetry;
    if (reqLoaderRetry > 0) {
        reqLoaderRetry--;
        ProtocolServiceRequest *req = request(xbus::node::apc_loader, QByteArray(), 100, 0);
        connect(req,
                &ProtocolServiceRequest::finished,
                this,
                &ProtocolServiceFirmware::requestLoaderRebootCheck);

        //connect(req, &ProtocolServiceRequest::timeout, this, [this]() {});
        return;
    }
    setStatus(tr("Can't initialize loader"));
    apxMsgW() << status();
    error();
}
void ProtocolServiceFirmware::requestLoaderRebootCheck()
{
    reqInit = ldr_req(xbus::node::ldc_init, QByteArray(), 200, 1);
    connect(reqInit,
            &ProtocolServiceRequest::timeout,
            this,
            &ProtocolServiceFirmware::requestLoaderReboot);
}
//=============================================================================
void ProtocolServiceFirmware::requestLoaderInit()
{
    qDebug() << reqLoaderRetry;
    reqInit = ldr_req(xbus::node::ldc_init, QByteArray(), 200, 50);
    connect(reqInit, &ProtocolServiceRequest::timeout, this, [this]() {
        setStatus(tr("Can't initialize upgrade"));
        apxMsgW() << status();
        error();
    });
}
//=============================================================================
void ProtocolServiceFirmware::initReply(QByteArray data)
{
    xbus::node::file::file_t f;
    XbusStreamReader stream(reinterpret_cast<const uint8_t *>(data.data()),
                            static_cast<uint16_t>(data.size()));
    f.read(&stream);

    if (f.start_address != startAddr) {
        setStatus(tr("Invalid start address"));
        apxMsgW() << QString("%1 0x%2 (avail: 0x%3)")
                         .arg(status())
                         .arg(static_cast<qulonglong>(startAddr), 8, 16, QChar('0'))
                         .arg(static_cast<qulonglong>(f.start_address), 8, 16, QChar('0'));
        error();
        return;
    }
    if (f.size < dataSize) {
        setStatus(tr("File too long"));
        apxMsgW() << QString("%1 (%2)").arg(status()).arg(f.size - dataSize);
        error();
        return;
    }
    //start file upload
    requestFileWrite();
}
//=============================================================================
//=============================================================================
void ProtocolServiceFirmware::requestFileWrite()
{
    //qDebug()<<node->name();
    xbus::node::file::file_t f;
    memset(&f, 0, sizeof(xbus::node::file::file_t));
    f.start_address = startAddr;
    f.size = dataSize;
    quint8 xor_crc = 0;
    for (int i = 0; i < wdata.size(); ++i)
        xor_crc ^= static_cast<quint8>(wdata.at(i));
    f.xor_crc = xor_crc;

    QByteArray ba(f.psize(), '\0');
    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(ba.data()));
    f.write(&stream);

    reqFileWrite = ldr_req(xbus::node::ldc_file, ba, 500, 3);
    connect(reqFileWrite,
            &ProtocolServiceRequest::retrying,
            this,
            &ProtocolServiceFirmware::retrying);
    connect(reqFileWrite, &ProtocolServiceRequest::timeout, this, [this]() {
        setStatus(tr("Can't initialize file upload"));
        apxMsgW() << status();
        error();
    });
}
void ProtocolServiceFirmware::fileWriteReply(QByteArray data)
{
    Q_UNUSED(data)
    //qDebug()<<node->info.name<<data.size();
    dataAddr = 0;
    if (requestWrite())
        return;
    finish(true);
}
//=============================================================================
//=============================================================================
bool ProtocolServiceFirmware::requestWrite(void)
{
    if (dataAddr >= dataSize)
        return false;
    quint16 cnt = xbus::node::file::size_block;
    uint rcnt = dataSize - dataAddr;
    xbus::node::file::file_data_hdr_t hdr;
    hdr.start_address = startAddr + dataAddr;
    hdr.data_size = cnt < rcnt ? cnt : static_cast<quint16>(rcnt);
    QByteArray blockData(wdata.mid(static_cast<int>(dataAddr), hdr.data_size));
    //fill FF in data block
    while (blockData.size() < xbus::node::file::size_block)
        blockData.append(static_cast<char>(0xFF));

    QByteArray ba(hdr.psize(), '\0');
    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(ba.data()));
    hdr.write(&stream);

    //make request
    /*qDebug() << QString("%1")
                    .arg(static_cast<qulonglong>(hdr.start_address), 8, 16, QChar('0'))
                    .toUpper()
             << hdr.data_size;*/
    reqWrite = ldr_req(xbus::node::ldc_write, ba.append(blockData), 5000, 3);
    connect(reqWrite, &ProtocolServiceRequest::retrying, this, &ProtocolServiceFirmware::retrying);
    connect(reqWrite, &ProtocolServiceRequest::timeout, this, &ProtocolServiceFirmware::restart);
    setProgress(static_cast<int>(dataAddr * 100 / dataSize));
    setStatus(QString("%1kB/%2kB").arg(dataAddr / 1024).arg(dataSize / 1024.0, 0, 'f', 1));
    return true;
}
void ProtocolServiceFirmware::writeReply(QByteArray data)
{
    //qDebug() << data.size();
    xbus::node::file::file_data_hdr_t hdr;
    XbusStreamReader stream(reinterpret_cast<const uint8_t *>(data.data()),
                            static_cast<uint16_t>(data.size()));
    hdr.read(&stream);

    if (hdr.start_address != (dataAddr + startAddr)) {
        qDebug() << "Wrong addr" << hdr.start_address << (dataAddr + startAddr);
        return;
    }
    dataAddr += hdr.data_size;
    if (requestWrite())
        return;
    finish(true);
}
//=============================================================================
//=============================================================================
//=============================================================================
