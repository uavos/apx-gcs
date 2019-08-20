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
#ifndef ProtocolServiceNode_H
#define ProtocolServiceNode_H
#include "ProtocolBase.h"
#include "ProtocolServiceFile.h"
#include <Dictionary/DictNode.h>
#include <QtCore>
class ProtocolService;
class ProtocolServiceRequest;
class XbusStreamReader;
//=============================================================================
class ProtocolServiceNode : public ProtocolBase
{
    Q_OBJECT
public:
    explicit ProtocolServiceNode(ProtocolService *service, const QString &sn);

    ProtocolServiceRequest *request(quint16 cmd,
                                    const QByteArray &data,
                                    int timeout_ms,
                                    bool highprio);

    //service protocol and requests management
    void serviceData(quint16 cmd, QByteArray data);

    //unpack helpers
    void fieldDictData(quint16 id, QByteArray data);
    void fieldValuesData(quint16 id, QByteArray data);

    QString name() const;
    bool isSubNode() const;

private:
    QString sn;
    ProtocolService *service;

    DictNode d;

    QHash<quint16, ProtocolServiceFile *> files;
    ProtocolServiceFile *createFile(quint16 cmdBase);

    void updateFieldDataType(DictNode::Field &f);
    DictNode::Field &createSubField(
        DictNode::Field &f, QString name, QString descr, QString units, int ftype);

    QByteArray packValue(DictNode::Field f, const QVariant &v) const;
    void unpackValue(DictNode::Field &f, XbusStreamReader *stream, QVariantList &values);

    QByteArray fieldRequest(quint16 fid, QByteArray data = QByteArray()) const;

    void requestImageField(DictNode::Field f);
    void imageFieldData(quint16 id, QByteArray data);
    void uploadImageField(DictNode::Field f, QVariant v);

    void updateDataValid();

private slots:
    void updateProgress();

    //export signals and slots
signals:
    void unknownServiceData(quint16 cmd, QByteArray data);
    void requestTimeout(quint16 cmd, QByteArray data);

    void searchReceived();
    void messageReceived(QString msg);
    void infoReceived(DictNode::Info info);
    void dictInfoReceived(DictNode::DictInfo conf);
    void dictReceived(DictNode::Dict dict);
    void valuesReceived(quint16 id, QVariantList values);
    void valueModifiedExternally(quint16 id);
    void valueUploaded(quint16 id);
    void valuesSaved();

    void nstatReceived(DictNode::Stats nstat);

public slots:
    void requestInfo();
    void requestDictInfo();
    void requestDict();
    void requestValues(quint16 id);
    void uploadValue(quint16 id, QVariant v);
    void saveValues();

    void requestNstat();
    void requestUser(quint16 id, QByteArray data, int timeout_ms);

    void loadCachedDict(DictNode::Dict dict);

    void acknowledgeRequest(quint16 cmd, QByteArray data = QByteArray());
};
//=============================================================================
#endif
