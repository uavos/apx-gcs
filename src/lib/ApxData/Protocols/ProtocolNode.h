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
#pragma once

#include "ProtocolBase.h"
#include "ProtocolNodeFile.h"
#include "ProtocolNodeRequest.h"

#include <QtCore>

#include <xbus/XbusNode.h>

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
    Q_PROPERTY(bool valid READ valid WRITE setValid NOTIFY validChanged)

public:
    explicit ProtocolNode(ProtocolNodes *nodes, const QString &sn);

    // called by nodes
    void downlink(const xbus::pid_s &pid, ProtocolStreamReader &stream);

    ProtocolNodeRequest *request(mandala::uid_t uid, size_t retry_cnt = 3);

    ProtocolNodeFile *file(const QString &fname);

    struct dict_field_s : public xbus::node::conf::field_s
    {
        QString name;
        QString title;
        QString units;
        QString path;
    };
    typedef QList<dict_field_s> Dict;

    ProtocolVehicle *vehicle() const;

    inline QVariantMap &values() { return _values; }

    inline quint64 lastSeenTime() const { return m_lastSeenTime; }
    inline QVariantMap dbDictInfo() const { return m_dbDictInfo; }
    inline quint64 dbConfigID() const { return m_dbConfigID; }
    inline QString identHash() const
    {
        return QString("%1").arg(ident().hash, 8, 16, QChar('0')).toUpper();
    }

    void setDict(const ProtocolNode::Dict &dict);

    QByteArray scriptFileData(const QVariant &value) const;

protected:
    QString toolTip() const override;
    void hashData(QCryptographicHash *h) const override;

private:
    ProtocolNodes *_nodes;

    QMap<QString, ProtocolNodeFile *> _files_map;
    Dict m_dict;
    QList<int> m_dict_fields;

    QVariantMap _values;

    QVariantMap m_dbDictInfo;
    quint64 m_dbConfigID{};

    QElapsedTimer timeReqLoader;

    quint64 m_lastSeenTime{};

    const dict_field_s *field(xbus::node::conf::fid_t fid) const;
    const dict_field_s *field(const QString &fpath) const;
    xbus::node::conf::fid_t fid(const QString &fpath) const;

    QVariant read_param(ProtocolStreamReader &stream, xbus::node::conf::fid_t fid);
    bool write_param(ProtocolStreamWriter &stream,
                     xbus::node::conf::fid_t fid,
                     const QVariant &value);

    void validate();

    // update fields batch
    QList<QObject *> _update_requests;

    // script
    QString _script_fpath;
    xbus::node::hash_t _script_hash;
    bool _parseScript(const QByteArray data);
    void _resetScript();

private slots:
    void requestRebootLoaderNext();

    void resetFilesMap();
    void updateDescr();

    void parseDictData(const xbus::node::file::info_s &info, const QByteArray data);
    void parseConfData(const xbus::node::file::info_s &info, const QByteArray data);
    void parseScriptData(const xbus::node::file::info_s &info, const QByteArray data);

    void updateRequestsCheckDone();

    //export signals and slots
signals:
    void requestTimeout(quint16 cmd, QByteArray data);

    void identReceived();
    void dictReceived(const ProtocolNode::Dict &dict);

    void confReceived(const QVariantMap &values);
    void confSaved();
    void confDefault();

    void messageReceived(xbus::node::msg::type_e type, QString msg);
    void statusReceived(const xbus::node::status::status_s &status);

    void loaderAvailable();
    void filesAvailable();

    // firmware upgrade forwarded signals
    void requestUpgrade(ProtocolNode *protocol, QString type);

public slots:
    void requestReboot();
    void requestRebootLoader();

    void requestIdent();
    void requestDict();
    void requestConf();
    void requestStatus();

    void requestUpdate(const QVariantMap &values);

    void requestMod(QStringList commands);
    void requestUsr(xbus::node::usr::cmd_t cmd, QByteArray data);

    // storage
    void dbDictInfoFound(QVariantMap dictInfo);
    void dbConfigIDFound(quint64 configID);

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

    bool valid() const;
    void setValid(const bool &v);

protected:
    xbus::node::ident::ident_s m_ident;

    QString m_sn;
    QString m_version;
    QString m_hardware;
    QStringList m_files;

    bool m_identValid{false};
    bool m_dictValid{false};
    bool m_valid{false};

signals:
    void identChanged();
    void versionChanged();
    void hardwareChanged();
    void filesChanged();

    void identValidChanged();
    void dictValidChanged();
    void validChanged();
};

Q_DECLARE_METATYPE(ProtocolNode::Dict);
