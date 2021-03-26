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

#include "PApxVehicle.h"

class PApxVehicle;
class PApxNode;
class PApxNodeRequest;

class PApxNodes : public PNodes
{
    Q_OBJECT

public:
    explicit PApxNodes(PApxVehicle *parent);

    bool process_downlink(const xbus::pid_s &pid, PStreamReader &stream);

private:
    PApxRequest _req;
    QHash<QString, PApxNode *> _nodes;

    PApxNode *getNode(QString uid, bool createNew = true);

    QTimer _reqTimer;
    QList<PApxNodeRequest *> _requests;
    PApxNodeRequest *_request{};
    uint _retry{};

protected:
    void requestSearch() override;

private slots:
    void request_scheduled(PApxNodeRequest *req);
    void request_finished(PApxNodeRequest *req);
    void request_timeout();
    void request_next();
    void request_current();
};
