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
    Q_PROPERTY(bool valid READ valid WRITE setValid NOTIFY validChanged)

    Q_PROPERTY(QString scriptTitle READ scriptTitle WRITE setScriptTitle NOTIFY scriptTitleChanged)

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

    // script support
    QByteArray scriptFileData(const QString source, const QByteArray code) const;
    inline QByteArray scriptCode() const { return _script_code; }

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
    QVariant read_param(ProtocolStreamReader &stream, xbus::node::conf::fid_t fid);
    bool write_param(ProtocolStreamWriter &stream,
                     xbus::node::conf::fid_t fid,
                     const QVariant &value);

    void validate();

    // script
    int _script_idx{-1};
    xbus::node::hash_t _script_hash;
    QByteArray _script_code;
    QByteArray _script_src;
    bool _parseScript(const QByteArray data);

private slots:
    void requestRebootLoaderNext();

    void resetFilesMap();
    void updateDescr();

    void parseDictData(const xbus::node::file::info_s &info, const QByteArray data);
    void parseConfData(const xbus::node::file::info_s &info, const QByteArray data);
    void parseScriptData(const xbus::node::file::info_s &info, const QByteArray data);

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

    void requestUpdate(xbus::node::conf::fid_t fid, QVariant value);
    void requestUpdateSave();

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

    QString scriptTitle() const;
    void setScriptTitle(const QString &v);

protected:
    xbus::node::ident::ident_s m_ident;

    QString m_sn;
    QString m_version;
    QString m_hardware;
    QStringList m_files;

    bool m_identValid{false};
    bool m_dictValid{false};
    bool m_valid{false};

    QString m_scriptTitle;

signals:
    void identChanged();
    void versionChanged();
    void hardwareChanged();
    void filesChanged();

    void identValidChanged();
    void dictValidChanged();
    void validChanged();

    void scriptTitleChanged();
};

Q_DECLARE_METATYPE(ProtocolNode::Dict);
