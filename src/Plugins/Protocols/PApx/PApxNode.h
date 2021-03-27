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
    void delete_request(mandala::uid_t uid);
    void clear_requests();
    void reschedule_request(PApxNodeRequest *req);

    void updateFiles(QStringList fnames);
    PApxNodeFile *file(QString name) const { return _files_map.value(name); }

private:
    PApxRequest _req;
    QHash<mandala::uid_t, PApxNodeRequest *> _requests;

    QMap<QString, PApxNodeFile *> _files_map;

    QJsonObject _ident;

    void updateProgress();

protected:
    void requestIdent() override { new PApxNodeRequestIdent(this); }
    void requestDict() override { new PApxNodeRequestFileRead(this, "dict"); }

    void requestReboot() override { new PApxNodeRequestReboot(this); }

private slots:
    void parseDictData(const xbus::node::file::info_s &info, const QByteArray data);
    void parseConfData(const xbus::node::file::info_s &info, const QByteArray data);
    void parseScriptData(const xbus::node::file::info_s &info, const QByteArray data);

signals:
    void request_scheduled(PApxNodeRequest *req);
    void request_finished(PApxNodeRequest *req);
};
