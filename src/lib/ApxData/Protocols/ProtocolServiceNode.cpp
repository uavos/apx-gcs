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
#include "ProtocolServiceNode.h"
#include "ProtocolService.h"

#include <Xbus/XbusNode.h>
#include <Xbus/XbusNodeConf.h>

#include <crc/crc.h>
//=============================================================================
ProtocolServiceNode::ProtocolServiceNode(ProtocolService *service, const QString &sn)
    : ProtocolBase(service)
    , sn(sn)
    , service(service)
{
    connect(this, &ProtocolServiceNode::valuesReceived, this, &ProtocolServiceNode::updateProgress);
    connect(this, &ProtocolServiceNode::valueUploaded, this, &ProtocolServiceNode::updateProgress);
    connect(this, &ProtocolServiceNode::valuesSaved, this, &ProtocolServiceNode::updateProgress);

    connect(service, &ProtocolService::activeChanged, this, [this]() {
        if (!this->service->active())
            setProgress(-1);
    });
    connect(service,
            &ProtocolService::requestTimeout,
            this,
            [this](ProtocolServiceRequest *request, ProtocolServiceNode *node) {
                if (node == this)
                    emit requestTimeout(request->cmd, request->data);
            });
}
//=============================================================================
QString ProtocolServiceNode::name() const
{
    return d.info.name;
}
bool ProtocolServiceNode::isSubNode() const
{
    return name().contains('.');
}
//=============================================================================
void ProtocolServiceNode::updateProgress()
{
    int v = 0;
    if (d.dict.commandsValid && d.dict.fieldsValid && d.dict.dataValid) {
        v = -1;
    } else if (d.dictInfo.valid) {
        int ncnt = 0;
        int pcnt = (d.dict.fieldsValid && d.dict.cached) ? 1 : 2;
        for (int i = 0; i < d.dict.fields.size(); ++i) {
            if (pcnt == 2 && d.dict.fields.at(i).valid)
                ncnt++;
            if (d.dict.fields.at(i).value.isValid())
                ncnt++;
        }
        if (ncnt)
            v = (ncnt * 100) / d.dictInfo.paramsCount / pcnt;
    }
    setProgress(v);
}
void ProtocolServiceNode::updateDataValid()
{
    int vcnt = 0;
    for (int i = 0; i < d.dict.fields.size(); ++i) {
        if (!d.dict.fields.at(i).valid)
            continue;
        if (!d.dict.fields.at(i).value.isValid())
            continue;
        vcnt++;
    }
    if (vcnt == d.dictInfo.paramsCount) {
        d.dict.dataValid = true;
    }
}
//=============================================================================
ProtocolServiceRequest *ProtocolServiceNode::request(quint16 cmd,
                                                     const QByteArray &data,
                                                     int timeout_ms,
                                                     bool highprio)
{
    return service->request(sn, cmd, data, timeout_ms, highprio);
}
QByteArray ProtocolServiceNode::fieldRequest(quint16 fid, QByteArray data) const
{
    QByteArray packet(sizeof(xbus::node::conf::fid_t), 0);
    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(packet.data()));
    stream.write<xbus::node::conf::fid_t>(fid);
    packet.append(data);
    return packet;
}
//=============================================================================
void ProtocolServiceNode::serviceData(quint16 cmd, QByteArray data)
{
    if (data.isEmpty() && cmd != xbus::node::apc_search && cmd != xbus::node::apc_script_file)
        return; //filter request

    uint16_t psize = static_cast<uint16_t>(data.size());
    uint8_t *pdata = reinterpret_cast<uint8_t *>(data.data());

    switch (cmd) {
    default:
        emit unknownServiceData(cmd, data);
        //qDebug() << cmd << data.size();
        return;
    case xbus::node::apc_search: { //response to search
        //qDebug() << "apc_search" << sn;
        requestInfo();
    } break;
    case xbus::node::apc_msg: { //message from vehicle
        QString s = QString(data).trimmed();
        if (s.isEmpty())
            break;
        emit messageReceived(s);
    } break;
    case xbus::node::apc_ack: { //user command acknowledge
        if (data.size() != 1)
            break;
        acknowledgeRequest(static_cast<quint8>(data.at(0)));
    } break;

    case xbus::node::apc_info: {
        //qDebug() << "apc_info" << sn << data.size();
        //fill available nodes
        if (xbus::node::Ident::psize() != psize)
            break;

        xbus::node::Ident dIdent;
        XbusStreamReader stream(pdata, psize);
        dIdent.read(&stream);

        //populate info
        d.info.name = QString(QByteArray(dIdent.name.data(), dIdent.name.size()));
        d.info.version = QString(QByteArray(dIdent.version.data(), dIdent.version.size()));
        d.info.hardware = QString(QByteArray(dIdent.hardware.data(), dIdent.hardware.size()));
        d.info.reconf = dIdent.flags.bits.conf_reset;
        d.info.fwSupport = dIdent.flags.bits.loader_support;
        d.info.fwUpdating = dIdent.flags.bits.in_loader;
        d.info.addressing = dIdent.flags.bits.addressing;
        d.info.rebooting = dIdent.flags.bits.reboot;
        d.info.busy = dIdent.flags.bits.busy;
        d.info.valid = true;
        infoReceived(d.info);
        if (!d.info.fwUpdating)
            requestDictInfo();
        acknowledgeRequest(cmd);
    } break;

    case xbus::node::apc_conf_inf: {
        if (xbus::node::IdentConf::psize() != psize)
            break;

        xbus::node::IdentConf dIdentConf;
        XbusStreamReader stream(pdata, psize);
        dIdentConf.read(&stream);
        //qDebug()<<"apc_conf_inf"<<sn<<data.size();

        //populate info
        d.dictInfo.chash = data.toHex().toUpper();
        d.dictInfo.paramsCount = dIdentConf.pcnt;
        d.dictInfo.valid = true;
        if (d.dictInfo.paramsCount != d.dict.fields.size() || d.dict.chash != d.dictInfo.chash) {
            //qDebug()<<"dict reset";
            d.dict.reset(d.dictInfo.chash, d.dictInfo.paramsCount);
        }
        dictInfoReceived(d.dictInfo);
        acknowledgeRequest(cmd);
    } break;

    case xbus::node::apc_nstat: {
        if (xbus::node::Status::psize() != psize)
            break;

        xbus::node::Status dStatus;
        XbusStreamReader stream(pdata, psize);
        dStatus.read(&stream);

        DictNode::Stats nstat;
        nstat.vbat = static_cast<double>(dStatus.vbat);
        nstat.ibat = static_cast<double>(dStatus.ibat);
        nstat.errCnt = dStatus.err_cnt;
        nstat.canRxc = dStatus.can_rxc;
        nstat.canAdr = dStatus.can_adr;
        nstat.canErr = dStatus.can_err;
        nstat.cpuLoad = dStatus.load * 100u / 255u;
        nstat.dump = QByteArray(reinterpret_cast<char *>(dStatus.dump.data()), dStatus.dump.size());
        nstatReceived(nstat);
        acknowledgeRequest(cmd);
    } break;

    case xbus::node::apc_conf_cmds: {
        if (!d.dictInfo.valid)
            break;
        QList<DictNode::Command> list;
        const char *str = data.data();
        int cnt = data.size();
        int sz;
        while (cnt > 0) {
            DictNode::Command c;
            c.cmd = static_cast<unsigned char>(*str++);
            cnt--;
            sz = static_cast<int>(strlen(str)) + 1;
            if (sz > cnt)
                break;
            c.name = QString(QByteArray(str, sz - 1));
            str += sz;
            cnt -= sz;
            sz = static_cast<int>(strlen(str)) + 1;
            if (sz > cnt)
                break;
            c.descr = QString(QByteArray(str, sz - 1));
            str += sz;
            cnt -= sz;
            list.append(c);
        }
        if (cnt != 0) {
            qDebug() << "Error node_conf commands received" << QString("cnt: %1)").arg(cnt);
            break;
        } else {
            d.dict.commands = list;
            d.dict.commandsValid = true;
            requestDict();
        }
        acknowledgeRequest(cmd);
    } break;

    case xbus::node::apc_conf_dsc: {
        if (!d.dictInfo.valid)
            break;

        if (psize <= sizeof(xbus::node::conf::fid_t))
            break;

        XbusStreamReader stream(pdata, psize);
        xbus::node::conf::fid_t fid;
        stream >> fid;

        //qDebug()<<"apc_conf_dsc"<<data.size()<<data.toHex().toUpper();
        fieldDictData(fid, data.mid(stream.position()));
        acknowledgeRequest(cmd, data.left(stream.position()));
    } break;

    case xbus::node::apc_conf_read: {
        if (!d.dict.fieldsValid)
            break;
        if (!d.dict.fieldsValid)
            break; //drop request

        if (psize <= sizeof(xbus::node::conf::fid_t))
            break;

        XbusStreamReader stream(pdata, psize);
        xbus::node::conf::fid_t fid;
        stream >> fid;

        fieldValuesData(fid, data.mid(stream.position()));
        acknowledgeRequest(cmd, data.left(stream.position()));
    } break;

    case xbus::node::apc_conf_write: {
        //qDebug() << data.toHex().toUpper();
        if (!d.dict.fieldsValid)
            break;

        if (psize < sizeof(xbus::node::conf::fid_t))
            break;

        XbusStreamReader stream(pdata, psize);
        xbus::node::conf::fid_t fid;
        stream >> fid;

        if (stream.tail() == 0) {
            //request fid or response of write eeprom
            acknowledgeRequest(cmd, data.left(stream.position()));
            //write field confirm
            if (fid >= 0xFF)
                valuesSaved();
            else
                emit valueUploaded(fid);
        } else { //field modified by other gcu
            const DictNode::Field &f = d.dict.fields.value(fid);
            if (!f.valid)
                break;
            fieldValuesData(fid, data.mid(stream.position()));
            emit valueModifiedExternally(fid);
        }
    } break;
    }
}
//=============================================================================
void ProtocolServiceNode::acknowledgeRequest(quint16 cmd, QByteArray data)
{
    service->acknowledgeRequest(sn, cmd, data);
}
//=============================================================================
void ProtocolServiceNode::requestInfo()
{
    request(xbus::node::apc_info, QByteArray(), 500, true);
}
void ProtocolServiceNode::requestDictInfo()
{
    request(xbus::node::apc_conf_inf, QByteArray(), 500, true);
}
void ProtocolServiceNode::requestDict()
{
    //qDebug()<<"req";
    if (!(d.info.valid && d.dictInfo.valid)) {
        qDebug() << "invalid protocol state";
        return;
    }
    if (isSubNode()) {
        //sub-nodes don't have commands
        d.dict.commandsValid = true;
    }
    if (!d.dict.commandsValid) {
        //qDebug()<<"apc_conf_cmds req";
        request(xbus::node::apc_conf_cmds, QByteArray(), 500, false);
    } else if (!d.dict.fieldsValid) {
        int cnt = 0;
        for (int i = 0; i < d.dict.fields.size(); ++i) {
            const DictNode::Field &f = d.dict.fields.at(i);
            if (f.valid)
                continue;
            cnt++;
            request(xbus::node::apc_conf_dsc,
                    fieldRequest(static_cast<xbus::node::conf::fid_t>(i), QByteArray()),
                    500,
                    false);
            //qDebug()<<"apc_conf_dsc req"<<i;
            break; //only once
        }
        if (cnt == 0) {
            //processDictFields();
            for (int i = 0; i < d.dict.fields.size(); ++i) {
                updateFieldDataType(d.dict.fields[i]);
            }
            d.dict.fieldsValid = true;
            emit dictReceived(d.dict);
        }
    }
    updateProgress();
}
//=============================================================================
//=============================================================================
void ProtocolServiceNode::updateFieldDataType(DictNode::Field &f)
{
    f.type = DictNode::Void;
    switch (f.ftype) {
    case xbus::node::conf::ft_option:
        f.type = DictNode::Option;
        break;
    case xbus::node::conf::ft_string:
        f.type = DictNode::String;
        break;
    case xbus::node::conf::ft_lstr:
        f.type = DictNode::StringL;
        break;
    case xbus::node::conf::ft_float:
        f.type = DictNode::Float;
        break;
    case xbus::node::conf::ft_byte:
        f.type = DictNode::Byte;
        break;
    case xbus::node::conf::ft_uint:
        f.type = DictNode::UInt;
        break;
    case xbus::node::conf::ft_varmsk:
        f.type = DictNode::MandalaID;
        break;
    case xbus::node::conf::ft_vec:
        for (int i = 0; i < 3; ++i) {
            QString s = f.opts.value(i, QString::number(i + 1));
            createSubField(f, s, f.descr + " (" + s + ")", f.units, xbus::node::conf::ft_float);
        }
        f.opts.clear();
        f.type = DictNode::Vector;
        break;
    case xbus::node::conf::ft_script:
        f.type = DictNode::Script;
        break;
    case xbus::node::conf::ft_regPID:
        createSubField(f, "Kp", tr("Proportional"), "K", xbus::node::conf::ft_float);
        createSubField(f, "Lp", tr("Proportional limit"), "%", xbus::node::conf::ft_byte);
        createSubField(f, "Kd", tr("Derivative"), "K", xbus::node::conf::ft_float);
        createSubField(f, "Ld", tr("Derivative limit"), "%", xbus::node::conf::ft_byte);
        createSubField(f, "Ki", tr("Integral"), "K", xbus::node::conf::ft_float);
        createSubField(f, "Li", tr("Integral limit"), "%", xbus::node::conf::ft_byte);
        createSubField(f, "Lo", tr("Output limit"), "%", xbus::node::conf::ft_byte);
        f.descr = "PID";
        f.type = DictNode::Hash;
        break;
    case xbus::node::conf::ft_regPI:
        createSubField(f, "Kp", tr("Proportional"), "K", xbus::node::conf::ft_float);
        createSubField(f, "Lp", tr("Proportional limit"), "%", xbus::node::conf::ft_byte);
        createSubField(f, "Ki", tr("Integral"), "K", xbus::node::conf::ft_float);
        createSubField(f, "Li", tr("Integral limit"), "%", xbus::node::conf::ft_byte);
        createSubField(f, "Lo", tr("Output limit"), "%", xbus::node::conf::ft_byte);
        f.descr = "PI";
        f.type = DictNode::Hash;
        break;
    case xbus::node::conf::ft_regP:
        createSubField(f, "Kp", tr("Proportional"), "K", xbus::node::conf::ft_float);
        createSubField(f, "Lo", tr("Output limit"), "%", xbus::node::conf::ft_byte);
        f.descr = "P";
        f.type = DictNode::Hash;
        break;
    case xbus::node::conf::ft_regPPI:
        createSubField(f, "Kpp", tr("Error to speed"), "K", xbus::node::conf::ft_float);
        createSubField(f, "Lpp", tr("Speed limit"), "%", xbus::node::conf::ft_byte);
        createSubField(f, "Kp", tr("Proportional"), "K", xbus::node::conf::ft_float);
        createSubField(f, "Lp", tr("Proportional limit"), "%", xbus::node::conf::ft_byte);
        createSubField(f, "Ki", tr("Integral"), "K", xbus::node::conf::ft_float);
        createSubField(f, "Li", tr("Integral limit"), "%", xbus::node::conf::ft_byte);
        createSubField(f, "Lo", tr("Output limit"), "%", xbus::node::conf::ft_byte);
        f.descr = "PPI";
        f.type = DictNode::Hash;
        break;
    }

    //arrays
    if (f.array > 1) {
        //fix complex ports config
        QStringList stNames;
        if (f.name == "ctr_ch") {
            QStringList st;
            //find size of array for ports channel
            int sz = 0;
            for (int i = 0; i < d.dict.fields.size(); ++i) {
                DictNode::Field &f1 = d.dict.fields[i];
                if (!f1.name.startsWith("ctr_ch_"))
                    continue;
                sz = f1.array;
                break;
            }
            for (int i = 0; i < sz; i++)
                st << QString("CH%1").arg(i + 1);
            f.opts = st;
            f.type = DictNode::Option;
        } else if (f.opts.size() == f.array && f.ftype != xbus::node::conf::ft_option) {
            stNames = f.opts;
        }
        //create array items
        for (int i = 0; i < f.array; ++i) {
            const QString stitle = stNames.value(i, QString::number(i + 1));
            DictNode::Field &f1 = createSubField(f,
                                                 QString::number(i + 1),
                                                 QString("%1/%2").arg(f.title).arg(stitle),
                                                 f.units,
                                                 f.ftype);
            f1.title = stitle;
            if (stNames.isEmpty() && (!f.opts.isEmpty())) {
                f1.opts = f.opts;
                f1.type = DictNode::Option;
            }
        }
        f.opts.clear();
        f.type = DictNode::Array;
    }
}
//=============================================================================
DictNode::Field &ProtocolServiceNode::createSubField(
    DictNode::Field &f, QString name, QString descr, QString units, int ftype)
{
    DictNode::Field f1;
    f1.id = static_cast<quint16>(f.subFields.size());
    f1.name = QString("%1_%2").arg(f.name).arg(name);
    f1.title = name;
    f1.descr = descr;
    f1.units = units;
    f1.ftype = ftype;
    updateFieldDataType(f1);
    f.subFields.append(f1);
    return f.subFields[f.subFields.size() - 1];
}
//=============================================================================
//=============================================================================
void ProtocolServiceNode::requestValues(quint16 id)
{
    updateProgress();
    const DictNode::Field &f = d.dict.fields.value(id);
    if (f.type == DictNode::Script) {
        requestImageField(f);
        return;
    }
    request(xbus::node::apc_conf_read, fieldRequest(id), 500, false);
}
//=============================================================================
//=============================================================================
void ProtocolServiceNode::fieldDictData(quint16 id, QByteArray data)
{
    DictNode::Field &f = d.dict.fields[id];
    if (f.valid)
        return; //duplicate response

    DictNode::Field r;
    r.id = id;
    int cnt = data.size();
    const char *str = data.data();
    int sz;
    r.ftype = static_cast<xbus::node::conf::ftype_t>(*str++);
    cnt--;
    sz = static_cast<int>(strlen(str)) + 1;
    if (sz > cnt)
        return;
    r.name = QString(QByteArray(str, sz - 1));
    str += sz;
    cnt -= sz;
    sz = static_cast<int>(strlen(str)) + 1;
    if (sz > cnt)
        return;
    r.descr = QString(QByteArray(str, sz - 1));
    str += sz;
    cnt -= sz;
    sz = static_cast<int>(strlen(str)) + 1;
    if (sz > cnt)
        return;
    //qDebug() << id << cnt << r.name << QString(QByteArray(str, sz - 1));
    r.opts = QString(QByteArray(str, sz - 1)).split(',', QString::SkipEmptyParts);
    str += sz;
    cnt -= sz;
    //qDebug()<<id<<cnt<<r.name<<r.descr;
    if (cnt != 0) {
        qDebug() << "Error node_conf descriptor received" << QString("cnt: %1)").arg(cnt) << r.name
                 << r.descr << r.opts;
        return;
    }

    if (r.ftype == xbus::node::conf::ft_option)
        r.type = DictNode::Option;

    r.expandStrings();
    //validate and publish
    //r.dictData=data;
    r.valid = true;
    /*if((!d.dict.cached) && f.valid){
    if(f.dictData==r.dictData)return;
    qDebug()<<"inconsistent"<<id<<info.name;
    //field already downloaded different data
    d.dict.reset();
    info.valid=false;
    requestInfo();
    return;
  }*/
    f = r;
    //continue requests
    requestDict();
}
//=============================================================================
void ProtocolServiceNode::fieldValuesData(quint16 id, QByteArray data)
{
    quint16 sid = id;
    QVariantList values;
    const uint16_t psize = static_cast<uint16_t>(data.size());
    const uint8_t *pdata = reinterpret_cast<uint8_t *>(data.data());
    XbusStreamReader stream(pdata, psize);

    //data may contain burst field values <data><fid><data><fid><data>...
    while (stream.position() < psize) {
        if (id >= d.dict.fields.size())
            break;
        DictNode::Field &f = d.dict.fields[id];
        if (!f.valid)
            break;

        unpackValue(f, &stream, values);

        if (stream.position() >= psize) {
            //all unpacked
            emit valuesReceived(sid, values);
            return;
        }

        id = stream.read<xbus::node::conf::fid_t, quint16>();
    }
    //error
    qDebug() << "unpack error" << sid << id << d.dict.fields[id].name << data.size();
}
//=============================================================================
void ProtocolServiceNode::unpackValue(DictNode::Field &f,
                                      XbusStreamReader *stream,
                                      QVariantList &values)
{
    if (!f.subFields.isEmpty()) {
        QVariantList svalues;
        for (int i = 0; i < f.subFields.size(); ++i) {
            DictNode::Field &fs = f.subFields[i];
            unpackValue(fs, stream, svalues);
        }
        f.value = QVariant(svalues);
        values.append(f.value);
        return;
    }
    QVariant v;
    switch (f.type) {
    default: {
        qDebug() << "unknown type" << DictNode::dataTypeToString(f.type);
    } break;
    case DictNode::UInt:
        v = QVariant::fromValue(stream->read<xbus::node::conf::ft_uint_t, int>());
        break;
    case DictNode::Float:
        v = QVariant::fromValue(stream->read<xbus::node::conf::ft_float_t, float>());
        break;
    case DictNode::Byte:
        v = QVariant::fromValue(stream->read<xbus::node::conf::ft_byte_t, int>());
        break;
    case DictNode::String: {
        xbus::node::conf::ft_string_t a;
        stream->read(a);
        v = QVariant::fromValue(QString(QByteArray(a.data(), a.size())));
    } break;
    case DictNode::StringL: {
        xbus::node::conf::ft_lstr_t a;
        stream->read(a);
        v = QVariant::fromValue(QString(QByteArray(a.data(), a.size())));
    } break;
    case DictNode::MandalaID:
        v = QVariant::fromValue(stream->read<xbus::node::conf::ft_varmsk_t, int>());
        break;
    case DictNode::Option:
        v = QVariant::fromValue(stream->read<xbus::node::conf::ft_option_t, int>());
        break;
    //complex types
    case DictNode::Script: {
        xbus::node::conf::ft_script_t a;
        a.read(stream);
        if (a.size == 0)
            v = QVariant::fromValue(QString());
        else {
            v = QVariant();
        }
    } break;
    }
    f.value = v;
    updateDataValid();
    values.append(v);
}
//=============================================================================
QByteArray ProtocolServiceNode::packValue(DictNode::Field f, const QVariant &v) const
{
    QByteArray data;
    if (!f.subFields.isEmpty()) {
        const QVariantList &values = v.value<QVariantList>();
        if (values.size() != f.subFields.size())
            return QByteArray();
        for (int i = 0; i < f.subFields.size(); ++i) {
            const DictNode::Field &fs = f.subFields.at(i);
            QByteArray sba = packValue(fs, values.at(i));
            if (sba.isEmpty())
                return QByteArray();
            data.append(sba);
        }
        return data;
    }
    data.fill('\0', 1024);
    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(data.data()));

    switch (f.type) {
    default: {
        qDebug() << "unknown type" << DictNode::dataTypeToString(f.type);
    } break;
    case DictNode::UInt:
        stream.write<xbus::node::conf::ft_uint_t, uint>(v.toUInt());
        break;
    case DictNode::Float:
        stream.write<xbus::node::conf::ft_float_t, float>(v.toFloat());
        break;
    case DictNode::Byte:
        stream.write<xbus::node::conf::ft_byte_t, uint>(v.toUInt());
        break;
    case DictNode::String: {
        xbus::node::conf::ft_string_t a;
        QByteArray src(v.toString().toUtf8());
        a.fill(0);
        std::copy(src.constBegin(), src.constEnd(), a.begin());
        a[a.size() - 1] = 0;
        stream << a;
    } break;
    case DictNode::StringL: {
        xbus::node::conf::ft_lstr_t a;
        QByteArray src(v.toString().toUtf8());
        a.fill(0);
        std::copy(src.constBegin(), src.constEnd(), a.begin());
        a[a.size() - 1] = 0;
        stream << a;
    } break;
    case DictNode::MandalaID:
        stream.write<xbus::node::conf::ft_varmsk_t, uint>(v.toUInt());
        break;
    case DictNode::Option:
        stream.write<xbus::node::conf::ft_option_t, uint>(v.toUInt());
        break;
    }
    data.resize(stream.position());
    return data;
}
//=============================================================================
void ProtocolServiceNode::requestImageField(DictNode::Field f)
{
    ProtocolServiceFile *p = createFile(xbus::node::apc_script_file);
    if (!p)
        return;
    connect(p, &ProtocolServiceFile::fileReceived, this, [this, f, p](const QByteArray &data) {
        imageFieldData(f.id, data);
        p->deleteLater();
        updateProgress();
    });
    p->download();
}
void ProtocolServiceNode::imageFieldData(quint16 id, QByteArray data)
{
    QVariantList pkg;
    while (data.size() > static_cast<int>(sizeof(xbus::node::conf::ft_script_t))) {
        xbus::node::conf::ft_script_t hdr;
        memcpy(&hdr, data.data(), sizeof(xbus::node::conf::ft_script_t));
        if (data.size() != (sizeof(xbus::node::conf::ft_script_t) + hdr.size)) {
            qDebug() << "wrong size" << data.size()
                     << (sizeof(xbus::node::conf::ft_script_t) + hdr.size);
            break;
        }
        if (hdr.code_size >= hdr.size) {
            qDebug() << "wrong code size" << hdr.code_size << hdr.size;
            break;
        }
        if (hdr.crc
            != CRC_16_IBM(reinterpret_cast<const uint8_t *>(data.data())
                              + sizeof(xbus::node::conf::ft_script_t),
                          hdr.size,
                          0xFFFF)) {
            qDebug() << "wrong CRC" << hdr.crc;
            break;
        }
        QByteArray bsource = data.mid(
            static_cast<int>(sizeof(xbus::node::conf::ft_script_t) + hdr.code_size));
        QString src = QString(qUncompress(bsource));
        if (src.isEmpty())
            break;
        pkg.append(src);
        QByteArray code = data.mid(static_cast<int>(sizeof(xbus::node::conf::ft_script_t)),
                                   static_cast<int>(hdr.code_size));
        pkg.append(code);
        //qDebug()<<src.size()<<code.size();
        break;
    }
    //qDebug()<<value;
    d.dict.fields[id].value = QVariant(pkg);
    updateDataValid();
    emit valuesReceived(id, QVariantList() << d.dict.fields.at(id).value);
}
void ProtocolServiceNode::uploadImageField(DictNode::Field f, QVariant v)
{
    QVariantList pkg = v.value<QVariantList>();
    QString src = pkg.value(0).toString();
    QByteArray code = pkg.value(1).toByteArray();
    QByteArray basrc = src.isEmpty() ? QByteArray() : qCompress(src.toUtf8(), 9);
    xbus::node::conf::ft_script_t hdr;
    memset(&hdr, 0, sizeof(xbus::node::conf::ft_script_t));
    hdr.size = static_cast<uint>(code.size() + basrc.size());
    hdr.code_size = static_cast<uint>(code.size());
    code.append(basrc);
    hdr.crc = CRC_16_IBM(reinterpret_cast<const quint8 *>(code.data()),
                         static_cast<uint>(code.size()),
                         0xFFFF);

    QByteArray data;
    data.append(reinterpret_cast<const char *>(&hdr), sizeof(hdr));
    data.append(code);

    ProtocolServiceFile *p = createFile(xbus::node::apc_script_file);
    if (!p)
        return;
    connect(p, &ProtocolServiceFile::fileUploaded, this, [=]() {
        emit valueUploaded(f.id);
        p->deleteLater();
    });
    service->setActive(true);
    p->upload(data);
}
//=============================================================================
void ProtocolServiceNode::uploadValue(quint16 id, QVariant v)
{
    const DictNode::Field &f = d.dict.fields[id];
    if (!f.valid) {
        qWarning() << "invalid dictionary" << id;
        return;
    }
    if (f.type == DictNode::Script) {
        uploadImageField(f, v);
        return;
    }
    QByteArray data = packValue(f, v);
    if (data.isEmpty()) {
        qWarning() << "can't pack value" << id << f.title;
        return;
    }
    service->setActive(true);
    request(xbus::node::apc_conf_write, fieldRequest(id, data), 1500, false);
}
void ProtocolServiceNode::saveValues()
{
    service->setActive(true);
    request(xbus::node::apc_conf_write, fieldRequest(0xFF), 1500, false);
}
//=============================================================================
//=============================================================================
void ProtocolServiceNode::requestNstat()
{
    if (isSubNode())
        return;
    service->setActive(true);
    request(xbus::node::apc_nstat, QByteArray(), 1000, false);
}
//=============================================================================
void ProtocolServiceNode::requestUser(quint16 id, QByteArray data, int timeout_ms)
{
    service->setActive(true);
    request(id, data, timeout_ms, false);
}
//=============================================================================
//=============================================================================
//=============================================================================
ProtocolServiceFile *ProtocolServiceNode::createFile(quint16 cmdBase)
{
    ProtocolServiceFile *file = files.value(cmdBase);
    if (file)
        return nullptr;
    //qDebug()<<"new file protocol";
    file = new ProtocolServiceFile(this, cmdBase);
    connect(file, &QObject::destroyed, this, [this, file]() {
        files.remove(files.key(file));
        //qDebug()<<"destroyed";
    });
    connect(file, &ProtocolServiceFile::fileReceived, file, [file]() { file->deleteLater(); });
    connect(service, &ProtocolService::stopRequested, file, [file]() { file->deleteLater(); });
    files.insert(cmdBase, file);
    return file;
}
//=============================================================================
//=============================================================================
void ProtocolServiceNode::loadCachedDict(DictNode::Dict dict)
{
    if (d.dict.chash != d.dictInfo.chash) {
        qDebug() << "hash mismatch";
        return;
    }
    d.dict = dict;
}
//=============================================================================
//=============================================================================
