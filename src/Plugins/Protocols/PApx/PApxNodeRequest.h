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
#include <Protocols/PNode.h>
#include <XbusNode.h>

class PApxNode;
class PApxNodes;

class PApxNodeRequest : public QObject
{
    Q_OBJECT
public:
    explicit PApxNodeRequest(PApxNode *node, mandala::uid_t uid, uint timeout_ms = 1000);
    virtual ~PApxNodeRequest();

    bool equals(PApxNodeRequest *req) { return uid() == req->uid() && cid() == req->cid(); }

    bool make_request(PApxRequest &req);
    bool check_response(PStreamReader &stream);
    void discard();

    uint timeout_ms() const { return _timeout_ms; }
    PApxNode *node() const { return _node; }
    mandala::uid_t uid() const { return _uid; }
    QString title() const;
    auto active() const { return _active; }

    virtual QString cid() const { return QString(); } // compare ID to check duplicates

    static constexpr uint retries = 5;

    bool silent{};

protected:
    PApxNode *_node;
    mandala::uid_t _uid;
    uint _timeout_ms;
    bool _active{};

    virtual bool request(PApxRequest &req) { return true; }
    virtual bool response(PStreamReader &stream) { return true; }

    PTrace *trace();

signals:
    void finished();
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
class PApxNodeRequestMod : public PApxNodeRequest
{
public:
    explicit PApxNodeRequestMod(PApxNode *node,
                                PNode::mod_cmd_e cmd,
                                QByteArray adr,
                                QStringList data)
        : PApxNodeRequest(node, mandala::cmd::env::nmt::mod::uid)
        , _cmd(cmd)
        , _adr(adr)
        , _data(data)
    {}

private:
    PNode::mod_cmd_e _cmd;
    QByteArray _adr;
    QStringList _data;

    xbus::node::mod::op_e _op;
    QString cid() const override { return QString(_adr.toHex()).append(_data.join('.')); }

    bool request(PApxRequest &req) override;
    bool response(PStreamReader &stream) override;
};
class PApxNodeRequestUsr : public PApxNodeRequest
{
public:
    explicit PApxNodeRequestUsr(PApxNode *node, quint8 cmd, QByteArray data)
        : PApxNodeRequest(node, mandala::cmd::env::nmt::usr::uid, 0)
        , _cmd(cmd)
        , _data(data)
    {}

private:
    quint8 _cmd;
    QByteArray _data;
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

class PApxNodeRequestFile : public PApxNodeRequest
{
    Q_OBJECT
public:
    explicit PApxNodeRequestFile(PApxNode *node,
                                 QString name,
                                 xbus::node::file::op_e op_init,
                                 xbus::node::file::op_e op_file = xbus::node::file::info)
        : PApxNodeRequest(node, mandala::cmd::env::nmt::file::uid)
        , _name(name)
        , _op_init(op_init)
        , _op_file(op_file)
        , _op(op_init)
    {}

protected:
    QString _name;
    xbus::node::file::op_e _op_init;
    xbus::node::file::op_e _op_file;
    xbus::node::file::op_e _op;

    xbus::node::file::info_s _info{};
    xbus::node::file::offset_t _offset{};
    xbus::node::file::size_t _tcnt{};
    xbus::node::hash_t _hash{};

    void reset();
    void next();

    virtual QString cid() const override { return _name; }
    bool request(PApxRequest &req) override final;
    bool response(PStreamReader &stream) override final;

    virtual void opened() {}

    virtual bool response_file(xbus::node::file::offset_t offset, PStreamReader &stream)
    {
        return true;
    }
    virtual bool request_file(PApxRequest &req) { return true; }

signals:
    void downladed();
    void uploaded();
    void progress(int percent);
};

class PApxNodeRequestFileRead : public PApxNodeRequestFile
{
public:
    // generates requests tree to download a file form node
    // data is collected by PApxNodeFile
    explicit PApxNodeRequestFileRead(PApxNode *node, QString name)
        : PApxNodeRequestFile(node, name, xbus::node::file::ropen, xbus::node::file::read)
    {
        qDebug() << "downloading:" << name;
    }

private:
    bool response_file(xbus::node::file::offset_t offset, PStreamReader &stream) override;
};

class PApxNodeRequestFileWrite : public PApxNodeRequestFile
{
public:
    // generates requests tree to upload a file form node
    explicit PApxNodeRequestFileWrite(PApxNode *node,
                                      QString name,
                                      QByteArray data,
                                      size_t offset = 0)
        : PApxNodeRequestFile(node, name, xbus::node::file::wopen, xbus::node::file::write)
        , _data(data)
        , _woffset(offset)
    {
        qDebug() << "uploading:" << name << data.size();
    }

private:
    QByteArray _data;
    size_t _woffset;

    void opened() override;
    bool request_file(PApxRequest &req) override;
    bool response_file(xbus::node::file::offset_t offset, PStreamReader &stream) override;
};
