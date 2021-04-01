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

#include "PApxNodeRequest.h"
#include "PApxVehicle.h"

class PApxVehicle;
class PApxNodes;
class PApxNodeFile;

class PApxNode : public PNode
{
    Q_OBJECT

public:
    explicit PApxNode(PApxNodes *parent, QString uid);
    ~PApxNode();

    void process_downlink(const xbus::pid_s &pid, PStreamReader &stream);

    void schedule_request(PApxNodeRequest *req);
    void delete_request(PApxNodeRequest *req);
    void reschedule_request(PApxNodeRequest *req);
    void extend_request(PApxNodeRequest *req, size_t time_ms);
    void request_deleted(PApxNodeRequest *req);

    void updateFiles(QStringList fnames);
    PApxNodeFile *file(QString name) const { return _files_map.value(name); }

    bool find_field(QString name,
                    xbus::node::conf::fid_t *fid,
                    xbus::node::conf::type_e *type) const;

    static QVariant read_param(PStreamReader &stream, xbus::node::conf::type_e type);
    static bool write_param(PStreamWriter &stream, xbus::node::conf::type_e type, QVariant value);

    void file_uploaded(QString name);

    static QByteArray pack_script(QVariant value);
    static QString hashToText(xbus::node::hash_t hash);

    QVariant optionToText(QVariant value, size_t fidx);
    QVariant textToOption(QVariant value, size_t fidx);

private:
    PApxNodes *_nodes;
    PApxRequest _req;
    QList<PApxNodeRequest *> _requests;

    QMap<QString, PApxNodeFile *> _files_map;

    // stored dict
    QString _dict_hash;
    QList<xbus::node::conf::type_e> _field_types;
    QList<size_t> _field_arrays;
    QStringList _field_names;
    QStringList _field_units;

    QVariantMap _values;
    xbus::node::conf::script_t _script_value{};
    QString _script_field;

    void updateProgress();

protected:
    void requestIdent() override { new PApxNodeRequestIdent(this); }
    void requestDict() override { new PApxNodeRequestFileRead(this, "dict"); }
    void requestConf() override { new PApxNodeRequestFileRead(this, "conf"); }
    void requestUpdate(QVariantMap values) override;

    void requestReboot() override { new PApxNodeRequestReboot(this); }
    void requestShell(QStringList commands) override { new PApxNodeRequestShell(this, commands); }
    void requestUsr(quint8 cmd, QByteArray data) override
    {
        new PApxNodeRequestUsr(this, cmd, data);
    }

private slots:
    void parseDictData(const xbus::node::file::info_s &info, const QByteArray data);
    void parseConfData(const xbus::node::file::info_s &info, const QByteArray data);
    void parseScriptData(const xbus::node::file::info_s &info, const QByteArray data);

signals:
    void request_scheduled(PApxNodeRequest *req);
    void request_finished(PApxNodeRequest *req);
    void request_extended(PApxNodeRequest *req, size_t time_ms);
};
