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
#include "ProtocolServiceRequest.h"
#include "ProtocolService.h"
#include "ProtocolServiceNode.h"

#include <App/AppLog.h>
//=============================================================================
ProtocolServiceRequest::ProtocolServiceRequest(ProtocolService *service,
                                               const QString &sn,
                                               quint16 cmd,
                                               const QByteArray &data,
                                               int timeout_ms,
                                               bool highprio,
                                               int retry)
    : QObject(service)
    , sn(sn)
    , cmd(cmd)
    , data(data)
    , timeout_ms(timeout_ms)
    , retryCnt(retry < 0 ? (timeout_ms > 0 ? 4 : 0) : retry)
    , active(false)
    , highprio(highprio)
    , acknowledged(false)
    , service(service)
    , retryNum(0)
{
    connect(this,
            &ProtocolServiceRequest::sendServiceRequest,
            service,
            &ProtocolService::sendServiceRequest);

    connect(&timer, &QTimer::timeout, this, &ProtocolServiceRequest::triggerTimeout);
    //qDebug()<<cmd<<sn<<data.size()<<data.toHex().toUpper();
    //t.start();
}
//=============================================================================
bool ProtocolServiceRequest::equals(QString sn, uint cmd, QByteArray data)
{
    return this->cmd == cmd && this->sn == sn && this->data == data
           && this->data.size() == data.size();
}
bool ProtocolServiceRequest::confirms(QString sn, uint cmd, QByteArray data)
{
    return this->cmd == cmd && this->sn == sn && this->data.startsWith(data);
}
//=============================================================================
void ProtocolServiceRequest::trigger()
{
    active = true;
    //qDebug()<<cmd<<sn<<data.size()<<data.toHex().toUpper();
    emit sendServiceRequest(sn, cmd, data);
    if (timeout_ms && (!this->sn.isEmpty())) {
        timer.start(timeout_ms);
    } else {
        finish();
    }
}
//=============================================================================
void ProtocolServiceRequest::finish(bool acknowledged)
{
    //qDebug()<<cmd<<sn<<data.size();//<<t.elapsed();//data.toHex().toUpper();
    this->acknowledged = acknowledged;
    timer.stop();
    active = false;
    emit finished(this);
}
//=============================================================================
void ProtocolServiceRequest::triggerTimeout()
{
    timer.stop();
    //request timeout
    if (retryNum < retryCnt) {
        retryNum++;
        emit retrying(retryNum, retryCnt);
        trigger();
        return;
    }
    active = false;
    if (retryCnt > 0) {
        ProtocolServiceNode *node = service->getNode(sn, false);
        apxConsoleW() << tr("Service timeout")
                      << QString("(%1): apc_%2 (%3)")
                             .arg(node ? node->name() : "?")
                             .arg(cmd)
                             .arg(QString(data.toHex().toUpper()));
        emit timeout();
        service->requestTimeout(this, node);
    }
    finish();
}
//=============================================================================
