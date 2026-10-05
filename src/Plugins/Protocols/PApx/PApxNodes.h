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

#include "PApxUnit.h"

class PApxUnit;
class PApxNode;
class PApxNodeRequest;

class PApxNodes : public PNodes
{
    Q_OBJECT

public:
    explicit PApxNodes(PApxUnit *parent);

    bool process_incoming_data(const xbus::pid_s &pid, PStreamReader &stream, bool is_remote_uplink);

    // data from another GCS instance
    void process_gcs_data(const xbus::pid_s &pid, PStreamReader &stream);

    auto nodes() const { return _nodes.values(); }
    auto local() const { return _local; }

    // true while waiting for nodes data from another GCS instance
    auto gcsWait() const { return _gcs_wait; }

    void cancel_requests(PApxNode *node);

    PApxNode *getNode(QString uid, bool createNew = true);

private:
    PApxRequest _req;
    QHash<QString, PApxNode *> _nodes;
    bool _local;

    QTimer _reqTimeout;
    QTimer _reqNext;

    QList<PApxNodeRequest *> _requests;
    PApxNodeRequest *_request{};
    uint _retry{};

    // nodes data exchange between GCS instances
    bool _gcs_wait{};      // waiting for the data instead of downloading from hardware
    bool _gcs_done{};      // the data was requested once already
    bool _gcs_requested{}; // reply to another GCS is pending
    QTimer _gcsReply;
    uint32_t _gcs_rx_hash{};
    uint32_t _gcs_rx_size{};
    QByteArray _gcs_rx_data;
    void loadGcsNode(QJsonObject node);

protected:
    void requestSearch() override;

    void requestSync() override;

private slots:
    // reauests sequencer
    void request_scheduled(PApxNodeRequest *req);
    void request_finished(PApxNodeRequest *req);
    void request_extended(PApxNodeRequest *req, size_t time_ms);

    void request_timeout();
    void request_next();
    void request_current();

    void requestGcsNodes();
    void sendGcsNodes();

    void updateActive();
};
