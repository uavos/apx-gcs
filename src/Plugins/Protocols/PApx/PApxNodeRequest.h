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

    bool make_request(PApxRequest &req);
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

    virtual bool request(PApxRequest &req) { return true; }
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
    bool request(PApxRequest &req) override;
};

class PApxNodeRequestIdent : public PApxNodeRequest
{
public:
    explicit PApxNodeRequestIdent(PApxNode *node)
        : PApxNodeRequest(node, mandala::cmd::env::nmt::ident::uid)
    {}
    bool response(PStreamReader &stream) override;
};

class PApxNodeRequestFile : public PApxNodeRequest
{
public:
    explicit PApxNodeRequestFile(PApxNode *node, QString name, xbus::node::file::op_e op)
        : PApxNodeRequest(node, mandala::cmd::env::nmt::file::uid)
        , _name(name)
        , _op(op)
    {}

protected:
    QString _name;
    xbus::node::file::op_e _op;

    virtual bool request(PApxRequest &req) override;
};

class PApxNodeRequestFileRead : public PApxNodeRequestFile
{
public:
    // generates requests tree to download a file form node
    // data is collected by PApxNodeFile
    explicit PApxNodeRequestFileRead(PApxNode *node, QString name)
        : PApxNodeRequestFile(node, name, xbus::node::file::ropen)
    {}

private:
    xbus::node::file::info_s _info{};
    xbus::node::file::offset_t _offset{};
    xbus::node::file::size_t _tcnt{};
    xbus::node::hash_t _hash{};

    void reset();
    void read_next();

    bool request(PApxRequest &req) override;
    bool response(PStreamReader &stream) override;
};

class PApxNodeRequestUpdate : public PApxNodeRequest
{
public:
    explicit PApxNodeRequestUpdate(PApxNode *node, QVariantMap values)
        : PApxNodeRequest(node, mandala::cmd::env::nmt::upd::uid)
        , _values(values)
    {}

private:
    QVariantMap _values;
    size_t _index{};
    xbus::node::conf::fid_t _fid{};

    bool request(PApxRequest &req) override;
    bool response(PStreamReader &stream) override;
};
