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
#pragma once

#include "ProtocolBase.h"
#include "ProtocolNodeFile.h"
#include "ProtocolNodeRequest.h"

#include <QtCore>

#include <Xbus/XbusNode.h>

class ProtocolNodes;

class ProtocolNode : public ProtocolBase
{
    Q_OBJECT

    Q_PROPERTY(QString sn READ sn CONSTANT)

    Q_PROPERTY(QString version READ version NOTIFY versionChanged)
    Q_PROPERTY(QString hardware READ hardware NOTIFY hardwareChanged)
    Q_PROPERTY(QStringList files READ files NOTIFY filesChanged)

    Q_PROPERTY(bool identValid READ identValid WRITE setIdentValid NOTIFY identValidChanged)
    Q_PROPERTY(bool dictValid READ dictValid WRITE setDictValid NOTIFY dictValidChanged)
    Q_PROPERTY(bool dataValid READ dataValid WRITE setDataValid NOTIFY dataValidChanged)

    Q_PROPERTY(bool upgrading READ upgrading WRITE setUpgrading NOTIFY upgradingChanged)

public:
    explicit ProtocolNode(ProtocolNodes *nodes, const QString &sn);

    // called by nodes
    void downlink(xbus::pid_t pid, ProtocolStreamReader &stream);

    ProtocolNodeRequest *request(xbus::pid_t pid, int timeout_ms = 500, int retry_cnt = -1);

    ProtocolNodeFile *file(const QString &fname);

    struct field_s : public xbus::node::dict::field_s
    {
        QString name;
        QString descr;
        QStringList units;
    };
    typedef QList<field_s> Dictionary;

private:
    ProtocolNodes *nodes;

    QMap<QString, ProtocolNodeFile *> _files_map;

    QElapsedTimer timeReqLoader;
private slots:
    void requestRebootLoaderNext();

    void resetFilesMap();
    void updateDescr();

    //export signals and slots
signals:
    void requestTimeout(quint16 cmd, QByteArray data);

    void identReceived();
    void dictReceived(const ProtocolNode::Dictionary &dict);
    void confReceived(const QVariantList &values);

    void messageReceived(xbus::node::msg::type_t type, QString msg);

    void loaderAvailable();
    void filesAvailable();

public slots:
    void requestReboot();
    void requestRebootLoader();

    void requestIdent();
    void requestDict();
    void requestConf();
    void requestStatus();

    //---------------------------------------
    // PROPERTIES
public:
    const xbus::node::ident::ident_s &ident() const;
    bool setIdent(const xbus::node::ident::ident_s &ident);

    QString sn() const;

    QString version() const;
    void setVersion(const QString &v);
    QString hardware() const;
    void setHardware(const QString &v);
    QStringList files() const;
    void setFiles(const QStringList &v);

    // additional props
    bool identValid() const;
    void setIdentValid(const bool &v);

    bool dictValid() const;
    void setDictValid(const bool &v);

    bool dataValid() const;
    void setDataValid(const bool &v);

    bool upgrading() const;
    void setUpgrading(const bool &v);

protected:
    xbus::node::ident::ident_s m_ident;

    QString m_sn;
    QString m_version;
    QString m_hardware;
    QStringList m_files;

    bool m_identValid{false};
    bool m_dictValid{false};
    bool m_dataValid{false};
    bool m_upgrading{false};

signals:
    void identChanged();
    void versionChanged();
    void hardwareChanged();
    void filesChanged();

    void identValidChanged();
    void dictValidChanged();
    void dataValidChanged();

    void upgradingChanged();
};
