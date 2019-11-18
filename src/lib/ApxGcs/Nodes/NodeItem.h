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
#ifndef NodeItem_H
#define NodeItem_H
//=============================================================================
#include "NodeField.h"
#include "NodeItemData.h"
#include "NodeTools.h"
#include <Protocols/ProtocolServiceNode.h>
#include <QDomDocument>
#include <QtCore>
#include <QtSql>
class Nodes;
//=============================================================================
class NodeItem : public NodeItemData
{
    Q_OBJECT

    Q_PROPERTY(QString version READ version WRITE setVersion NOTIFY versionChanged)
    Q_PROPERTY(QString hardware READ hardware WRITE setHardware NOTIFY hardwareChanged)

    Q_PROPERTY(bool infoValid READ infoValid WRITE setInfoValid NOTIFY infoValidChanged)
    Q_PROPERTY(bool offline READ offline NOTIFY offlineChanged)

public:
    explicit NodeItem(Nodes *parent, QString sn, ProtocolServiceNode *protocol);

    QString conf_hash;
    qint64 lastSeenTime;

    //db sync
    QVariantMap dictInfo;
    quint64 nconfID;

    QList<NodeField *> allFields;
    QMap<QString, NodeField *> allFieldsByName;

    //override
    QVariant data(int col, int role) const;
    void hashData(QCryptographicHash *h) const;

    void clear();

    Nodes *nodes;
    NodeItemBase *group;

    NodeTools *tools;

    void execCommand(quint16 cmd, const QString &name, const QString &descr);

    void setProtocol(ProtocolServiceNode *protocol);

    int loadConfigValues(QVariantMap values);
    bool loadConfigValue(const QString &name, const QString &value);

protected:
    QStringList sortNames;

    void groupFields(void);
    void groupNodes(void);
    void groupArrays(NodesBase *group);

    void saveTelemetryUploadEvent();
    void saveTelemetryConf(NodeField *f);

    QTimer nstatTimer;

private:
    ProtocolServiceNode *protocol;

    NodeField *dictCreateField(const DictNode::Field &f, NodeField *parentField);
    void requestFieldValues();

    NodeItem *subNode() const;

private slots:

    void validateDict();
    void validateData();
    void validateInfo();

    void updateReconf();

    void nodeNotify();

    void updateArrayRowDescr(Fact *fRow);

public slots:
    void upload();
    void updateDescr();

    void upgradeFirmware();
    void upgradeLoader();
    void upgradeRadio();

    //db sync
    void setDictInfo(QVariantMap dictInfo);
    void setNconfID(quint64 nconfID);

    void message(QString msg);

    //protocols:
public slots:
    void dictReceived(const DictNode::Dict &dict);

private slots:
    void messageReceived(const QString &msg);
    void infoReceived(const DictNode::Info &info);
    void dictInfoReceived(const DictNode::DictInfo &conf);
    void valuesReceived(quint16 id, const QVariantList &values);
    void valueModifiedExternally(quint16 id);
    void valueUploaded(quint16 id);
    void valuesSaved();
    void nstatReceived(const DictNode::Stats &nstat);

signals:
    void requestInfo();
    void requestDict();
    void requestValues(quint16 id);
    void uploadValue(quint16 id, const QVariant &v);
    void saveValues();
    void requestNstat();
    void requestUser(quint16 id, QByteArray data, int timeout_ms);

    //protocol forward for user commands (i.e. blackbox read)
    void acknowledgeRequest(quint16 cmd, QByteArray data = QByteArray());
    void serviceDataReceived(quint16 cmd, QByteArray data);
    void requestTimeout(quint16 cmd, QByteArray data);

    //---------------------------------------
    // PROPERTIES
public:
    QString version() const;
    void setVersion(const QString &v);
    QString hardware() const;
    void setHardware(const QString &v);
    bool infoValid() const;
    void setInfoValid(const bool &v);
    bool offline() const;

protected:
    QString m_version;
    QString m_hardware;
    bool m_infoValid;

signals:
    void versionChanged();
    void hardwareChanged();
    void infoValidChanged();
    void offlineChanged();
};
//=============================================================================
#endif
