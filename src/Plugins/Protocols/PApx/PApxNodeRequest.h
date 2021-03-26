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

#include "PApxRequest.h"

#include <Mandala/Mandala.h>
#include <xbus/XbusNode.h>

class PApxNode;
class PApxNodes;

class PApxNodeRequest
{
public:
    explicit PApxNodeRequest(PApxNode *node, mandala::uid_t uid, uint timeout_ms = 1000);
    virtual ~PApxNodeRequest() {}

    void make_request(PApxRequest &req);
    bool check_response(PStreamReader &stream) { return response(stream); }
    void discard();

    uint timeout_ms() const { return _timeout_ms; }
    PApxNode *node() const { return _node; }
    mandala::uid_t uid() const { return _uid; }

    static constexpr uint retries = 5;

protected:
    PApxNode *_node;
    mandala::uid_t _uid;
    uint _timeout_ms;

    virtual void request(PApxRequest &req) {}
    virtual bool response(PStreamReader &stream) { return true; }

    PTrace *trace() const;
};

class PApxNodeRequestReboot : public PApxNodeRequest
{
public:
    explicit PApxNodeRequestReboot(PApxNode *node,
                                   xbus::node::reboot::type_e type = xbus::node::reboot::firmware)
        : PApxNodeRequest(node, mandala::cmd::env::nmt::reboot::uid)
        , _type(type)
    {}

private:
    xbus::node::reboot::type_e _type;
    void request(PApxRequest &req) override { req << _type; }
};

class PApxNodeRequestIdent : public PApxNodeRequest
{
public:
    explicit PApxNodeRequestIdent(PApxNode *node)
        : PApxNodeRequest(node, mandala::cmd::env::nmt::ident::uid)
    {}
    bool response(PStreamReader &stream) override;
};
