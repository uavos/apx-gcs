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

#include <Xbus/XbusNodeConf.h>
#include <Math/crc.h>
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
//=============================================================================
void ProtocolServiceNode::serviceData(quint16 cmd, QByteArray data)
{
    if (data.isEmpty() && cmd != XbusNode::apc_search && cmd != XbusNode::apc_script_file)
        return; //filter request
    switch (cmd) {
    default: {
        emit unknownServiceData(cmd, data);
    }
        return;
    case XbusNode::apc_search: { //response to search
        //qDebug()<<"apc_search"<<sn;
        requestInfo();
    } break;
    case XbusNode::apc_msg: { //message from vehicle
        QString s = QString(data).trimmed();
        if (s.isEmpty())
            break;
        emit messageReceived(s);
    } break;
    case XbusNode::apc_ack: { //user command acknowledge
        if (data.size() != 1)
            break;
        service->acknowledgeRequest(sn, static_cast<quint8>(data.at(0)));
    } break;

    case XbusNode::apc_info: {
        //qDebug()<<"apc_info"<<sn<<data.size();
        //fill available nodes
        uint data_cnt = static_cast<uint>(data.size());
        if (data_cnt < sizeof(xbus::node_name_t))
            break; //filter request
        xbus::node_info_t ninfo;
        memset(&ninfo, 0, sizeof(xbus::node_info_t));
        if (data_cnt > sizeof(xbus::node_info_t)) {
            memcpy(&ninfo, data.data(), sizeof(xbus::node_info_t));
        } else {
            memcpy(&ninfo, data.data(), data_cnt);
        }
        ninfo.name[sizeof(xbus::node_name_t) - 1] = 0;
        //populate info
        d.info.name = QString(reinterpret_cast<const char *>(ninfo.name));
        d.info.version = QString(reinterpret_cast<const char *>(ninfo.version));
        d.info.hardware = QString(reinterpret_cast<const char *>(ninfo.hardware));
        d.info.reconf = ninfo.flags.conf_reset;
        d.info.fwSupport = ninfo.flags.loader_support;
        d.info.fwUpdating = ninfo.flags.in_loader;
        d.info.addressing = ninfo.flags.addressing;
        d.info.rebooting = ninfo.flags.reboot;
        d.info.busy = ninfo.flags.busy;
        d.info.valid = true;
        infoReceived(d.info);
        if (!d.info.fwUpdating)
            requestDictInfo();
        service->acknowledgeRequest(sn, cmd);
    } break;

    case XbusNode::apc_conf_inf: {
        if (!d.info.valid)
            break;
        //qDebug()<<"apc_conf_inf"<<sn<<data.size();
        uint data_cnt = static_cast<uint>(data.size());
        if (data_cnt == 0)
            break; //filter request
        if (data_cnt != sizeof(xbus::node_conf_info_t))
            break;
        xbus::node_conf_info_t conf_inf;
        memcpy(&conf_inf, data.data(), sizeof(xbus::node_conf_info_t));
        //populate info
        d.dictInfo.chash = data.toHex().toUpper();
        d.dictInfo.paramsCount = conf_inf.cnt;
        d.dictInfo.valid = true;
        if (d.dictInfo.paramsCount != d.dict.fields.size() || d.dict.chash != d.dictInfo.chash) {
            //qDebug()<<"dict reset";
            d.dict.reset(d.dictInfo.chash, d.dictInfo.paramsCount);
        }
        dictInfoReceived(d.dictInfo);
        service->acknowledgeRequest(sn, cmd);
    } break;

    case XbusNode::apc_nstat: {
        if (data.size() != (sizeof(xbus::node_name_t) + sizeof(xbus::node_status_t)))
            break;
        xbus::node_status_t nstatus;
        memcpy(&nstatus, data.data() + sizeof(xbus::node_name_t), sizeof(xbus::node_status_t));
        DictNode::Stats nstat;
        nstat.vbat = nstatus.power.VBAT / 1000.0;
        nstat.ibat = nstatus.power.IBAT / 1000.0;
        nstat.errCnt = nstatus.err_cnt;
        nstat.canRxc = nstatus.can_rxc;
        nstat.canAdr = nstatus.can_adr;
        nstat.canErr = nstatus.can_err;
        nstat.cpuLoad = static_cast<quint8>(static_cast<uint>(nstatus.load) * 100 / 255);
        nstat.dump = QByteArray(reinterpret_cast<const char *>(nstatus.dump), sizeof(nstatus.dump));
        nstatReceived(nstat);
        service->acknowledgeRequest(sn, cmd);
    } break;

    case XbusNode::apc_conf_cmds: {
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
        service->acknowledgeRequest(sn, cmd);
    } break;

    case XbusNode::apc_conf_dsc: {
        if (!d.dictInfo.valid)
            break;
        if (data.size() < 2)
            break; //filter request
        //if(!(data.size()>=2 && this->data.size()==1 && data.at(0)==this->data.at(0))) return;
        //if(d.dict.fieldsValid)break;
        quint16 id = static_cast<quint8>(data.at(0));
        //qDebug()<<"apc_conf_dsc"<<data.size()<<data.toHex().toUpper();
        fieldDictData(id, data.mid(1));
        service->acknowledgeRequest(sn, cmd, data.left(1));
    } break;

    case XbusNode::apc_conf_read: {
        if (!d.dict.fieldsValid)
            break;
        if (data.size() <= static_cast<int>(sizeof(xbus::node_fid_t)))
            break; //filter request
        //if(!(data.size()>=2 && this->data.size()==1 && data.at(0)==this->data.at(0))) return;
        if (!d.dict.fieldsValid)
            break; //drop request
        quint16 id = *reinterpret_cast<const xbus::node_fid_t *>(data.data());
        fieldValuesData(id, data.mid(sizeof(xbus::node_fid_t)));
        service->acknowledgeRequest(sn, cmd, data.left(1));
    } break;

    case XbusNode::apc_conf_write: {
        //qDebug() << data.toHex().toUpper();
        if (!d.dict.fieldsValid)
            break;
        if (data.size()
            == static_cast<int>(sizeof(
                xbus::node_fid_t))) { // && this->data.size()>1 && data.at(0)==this->data.at(0)){
            service->acknowledgeRequest(sn, cmd, data.left(sizeof(xbus::node_fid_t)));
            //write field confirm
            quint16 id = *reinterpret_cast<const xbus::node_fid_t *>(data.data());
            if (id == 0xFF)
                valuesSaved();
            else
                emit valueUploaded(id);
        } else if (data.size()
                   > static_cast<int>(sizeof(xbus::node_fid_t))) { //field modified by other gcu
            quint16 id = *reinterpret_cast<const xbus::node_fid_t *>(data.data());
            const DictNode::Field &f = d.dict.fields.value(id);
            if (!f.valid)
                break;
            fieldValuesData(id, data.mid(sizeof(xbus::node_fid_t)));
            emit valueModifiedExternally(id);
        }
    } break;
    }
}
//=============================================================================
void ProtocolServiceNode::requestInfo()
{
    request(XbusNode::apc_info, QByteArray(), 500, true);
}
void ProtocolServiceNode::requestDictInfo()
{
    request(XbusNode::apc_conf_inf, QByteArray(), 500, true);
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
        request(XbusNode::apc_conf_cmds, QByteArray(), 500, false);
    } else if (!d.dict.fieldsValid) {
        int cnt = 0;
        for (int i = 0; i < d.dict.fields.size(); ++i) {
            const DictNode::Field &f = d.dict.fields.at(i);
            if (f.valid)
                continue;
            cnt++;
            request(XbusNode::apc_conf_dsc, QByteArray().append(static_cast<char>(i)), 500, false);
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
    case xbus::ft_option:
        f.type = DictNode::Option;
        break;
    case xbus::ft_string:
        f.type = DictNode::String;
        break;
    case xbus::ft_lstr:
        f.type = DictNode::StringL;
        break;
    case xbus::ft_float:
        f.type = DictNode::Float;
        break;
    case xbus::ft_byte:
        f.type = DictNode::Byte;
        break;
    case xbus::ft_uint:
        f.type = DictNode::UInt;
        break;
    case xbus::ft_varmsk:
        f.type = DictNode::MandalaID;
        break;
    case xbus::ft_vec:
        for (int i = 0; i < 3; ++i) {
            QString s = f.opts.value(i, QString::number(i + 1));
            createSubField(f, s, f.descr + " (" + s + ")", f.units, xbus::ft_float);
        }
        f.opts.clear();
        f.type = DictNode::Vector;
        break;
    case xbus::ft_script:
        f.type = DictNode::Script;
        break;
    case xbus::ft_regPID:
        createSubField(f, "Kp", tr("Proportional"), "K", xbus::ft_float);
        createSubField(f, "Lp", tr("Proportional limit"), "%", xbus::ft_byte);
        createSubField(f, "Kd", tr("Derivative"), "K", xbus::ft_float);
        createSubField(f, "Ld", tr("Derivative limit"), "%", xbus::ft_byte);
        createSubField(f, "Ki", tr("Integral"), "K", xbus::ft_float);
        createSubField(f, "Li", tr("Integral limit"), "%", xbus::ft_byte);
        createSubField(f, "Lo", tr("Output limit"), "%", xbus::ft_byte);
        f.descr = "PID";
        f.type = DictNode::Hash;
        break;
    case xbus::ft_regPI:
        createSubField(f, "Kp", tr("Proportional"), "K", xbus::ft_float);
        createSubField(f, "Lp", tr("Proportional limit"), "%", xbus::ft_byte);
        createSubField(f, "Ki", tr("Integral"), "K", xbus::ft_float);
        createSubField(f, "Li", tr("Integral limit"), "%", xbus::ft_byte);
        createSubField(f, "Lo", tr("Output limit"), "%", xbus::ft_byte);
        f.descr = "PI";
        f.type = DictNode::Hash;
        break;
    case xbus::ft_regP:
        createSubField(f, "Kp", tr("Proportional"), "K", xbus::ft_float);
        createSubField(f, "Lo", tr("Output limit"), "%", xbus::ft_byte);
        f.descr = "P";
        f.type = DictNode::Hash;
        break;
    case xbus::ft_regPPI:
        createSubField(f, "Kpp", tr("Error to speed"), "K", xbus::ft_float);
        createSubField(f, "Lpp", tr("Speed limit"), "%", xbus::ft_byte);
        createSubField(f, "Kp", tr("Proportional"), "K", xbus::ft_float);
        createSubField(f, "Lp", tr("Proportional limit"), "%", xbus::ft_byte);
        createSubField(f, "Ki", tr("Integral"), "K", xbus::ft_float);
        createSubField(f, "Li", tr("Integral limit"), "%", xbus::ft_byte);
        createSubField(f, "Lo", tr("Output limit"), "%", xbus::ft_byte);
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
        } else if (f.opts.size() == f.array && f.ftype != xbus::ft_option) {
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
    postprocessField(f);
}
//=============================================================================
void ProtocolServiceNode::postprocessField(DictNode::Field &f)
{
    //find packed size
    int sz = 0;
    switch (f.type) {
    default:
        sz = 0;
        break;
    case DictNode::Option:
        sz = sizeof(xbus::ft_option_t);
        break;
    case DictNode::MandalaID:
        sz = sizeof(xbus::ft_varmsk_t);
        break;
    case DictNode::UInt:
        sz = sizeof(xbus::ft_uint_t);
        break;
    case DictNode::Float:
        sz = sizeof(xbus::ft_float_t);
        break;
    case DictNode::Vector:
        sz = sizeof(xbus::ft_vec_t);
        break;
    case DictNode::Byte:
        sz = sizeof(xbus::ft_byte_t);
        break;
    case DictNode::String:
        sz = sizeof(xbus::ft_string_t);
        break;
    case DictNode::StringL:
        sz = sizeof(xbus::ft_lstr_t);
        break;
    case DictNode::Script:
        sz = sizeof(xbus::ft_script_t);
        break;
    }
    f.packedSize = sz;
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
    request(XbusNode::apc_conf_read, QByteArray().append(static_cast<char>(id)), 500, false);
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
    r.ftype = static_cast<xbus::node_fieldtypes_t>(*str++);
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
    r.opts = QString(QByteArray(str, sz - 1)).split(',', QString::SkipEmptyParts);
    str += sz;
    cnt -= sz;
    //qDebug()<<id<<cnt<<r.name<<r.descr;
    if (cnt != 0) {
        qDebug() << "Error node_conf descriptor received" << QString("cnt: %1)").arg(cnt) << r.name
                 << r.descr << r.opts;
        return;
    }

    if (r.ftype == xbus::ft_option)
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
    while (!data.isEmpty()) {
        if (id >= d.dict.fields.size())
            break;
        DictNode::Field &f = d.dict.fields[id];
        if (!f.valid)
            break;

        int cnt = unpackValue(f, data, values);
        if (cnt <= 0)
            break;

        if (data.size() == cnt) {
            //all unpacked
            emit valuesReceived(sid, values);
            return;
        }

        id = static_cast<quint8>(data.at(cnt));
        data.remove(0, cnt + 1);
    }
    //error
    qDebug() << "unpack error" << sid << id << d.dict.fields[id].name << data.size();
}
//=============================================================================
int ProtocolServiceNode::unpackValue(DictNode::Field &f, QByteArray data, QVariantList &values)
{
    if (!f.subFields.isEmpty()) {
        int sz = 0;
        QVariantList svalues;
        for (int i = 0; i < f.subFields.size(); ++i) {
            DictNode::Field &fs = f.subFields[i];
            int cnt = unpackValue(fs, data, svalues);
            if (cnt <= 0)
                return 0;
            sz += cnt;
            data.remove(0, fs.packedSize);
        }
        f.value = QVariant(svalues);
        values.append(f.value);
        return sz;
    }
    if (data.size() < f.packedSize)
        return 0;
    const char *ptr = data.data();
    QVariant v;
    switch (f.type) {
    default: {
        qDebug() << "unknown type" << DictNode::dataTypeToString(f.type);
    } break;
    case DictNode::UInt:
        v = QVariant::fromValue(static_cast<int>(*reinterpret_cast<const xbus::ft_uint_t *>(ptr)));
        break;
    case DictNode::Float:
        v = QVariant::fromValue(
            static_cast<double>(*reinterpret_cast<const xbus::ft_float_t *>(ptr)));
        break;
    case DictNode::Byte:
        v = QVariant::fromValue(static_cast<int>(*reinterpret_cast<const xbus::ft_byte_t *>(ptr)));
        break;
    case DictNode::String:
        v = QVariant::fromValue(
            QString(QByteArray(reinterpret_cast<const char *>(
                                   *reinterpret_cast<const xbus::ft_string_t *>(ptr)),
                               sizeof(xbus::ft_string_t))));
        break;
    case DictNode::StringL:
        v = QVariant::fromValue(
            QString(QByteArray(reinterpret_cast<const char *>(
                                   *reinterpret_cast<const xbus::ft_lstr_t *>(ptr)),
                               sizeof(xbus::ft_lstr_t))));
        break;
    case DictNode::MandalaID:
        v = QVariant::fromValue(static_cast<int>(*reinterpret_cast<const xbus::ft_varmsk_t *>(ptr)));
        break;
    case DictNode::Option:
        v = QVariant::fromValue(static_cast<int>(*reinterpret_cast<const xbus::ft_option_t *>(ptr)));
        break;
    //complex types
    case DictNode::Script: {
        const xbus::ft_script_t *scr = reinterpret_cast<const xbus::ft_script_t *>(ptr);
        if (scr->size == 0)
            v = QVariant::fromValue(QString());
        else {
            v = QVariant();
        }
    } break;
    }
    f.value = v;
    updateDataValid();
    values.append(v);
    return f.packedSize;
}
//=============================================================================
QByteArray ProtocolServiceNode::packValue(DictNode::Field f, const QVariant &v) const
{
    QByteArray ba;
    if (!f.subFields.isEmpty()) {
        const QVariantList &values = v.value<QVariantList>();
        if (values.size() != f.subFields.size())
            return QByteArray();
        for (int i = 0; i < f.subFields.size(); ++i) {
            const DictNode::Field &fs = f.subFields.at(i);
            QByteArray sba = packValue(fs, values.at(i));
            if (sba.isEmpty())
                return QByteArray();
            ba.append(sba);
        }
        return ba;
    }
    if (f.packedSize <= 0) {
        qDebug() << "zero size field" << f.title;
    }
    ba.resize(f.packedSize);
    void *ptr = ba.data();
    switch (f.type) {
    default: {
        qDebug() << "unknown type" << DictNode::dataTypeToString(f.type);
    } break;
    case DictNode::UInt:
        *static_cast<xbus::ft_uint_t *>(ptr) = v.toUInt();
        break;
    case DictNode::Float:
        *static_cast<xbus::ft_float_t *>(ptr) = v.toFloat();
        break;
    case DictNode::Byte:
        *static_cast<xbus::ft_byte_t *>(ptr) = static_cast<xbus::ft_byte_t>(v.toUInt());
        break;
    case DictNode::String:
        strncpy(static_cast<char *>(ptr), v.toString().toUtf8().data(), sizeof(xbus::ft_string_t));
        break;
    case DictNode::StringL:
        strncpy(static_cast<char *>(ptr), v.toString().toUtf8().data(), sizeof(xbus::ft_lstr_t));
        break;
    case DictNode::MandalaID:
        *static_cast<xbus::ft_varmsk_t *>(ptr) = static_cast<xbus::ft_varmsk_t>(v.toUInt());
        break;
    case DictNode::Option:
        *static_cast<xbus::ft_option_t *>(ptr) = static_cast<xbus::ft_option_t>(v.toUInt());
        break;
    }
    return ba;
}
//=============================================================================
void ProtocolServiceNode::requestImageField(DictNode::Field f)
{
    ProtocolServiceFile *p = createFile(XbusNode::apc_script_file);
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
    while (data.size() > static_cast<int>(sizeof(xbus::ft_script_t))) {
        xbus::ft_script_t hdr;
        memcpy(&hdr, data.data(), sizeof(xbus::ft_script_t));
        if (data.size() != (sizeof(xbus::ft_script_t) + hdr.size)) {
            qDebug() << "wrong size" << data.size() << (sizeof(xbus::ft_script_t) + hdr.size);
            break;
        }
        if (hdr.code_size >= hdr.size) {
            qDebug() << "wrong code size" << hdr.code_size << hdr.size;
            break;
        }
        if (hdr.crc
            != CRC_16_IBM(reinterpret_cast<const uint8_t *>(data.data()) + sizeof(xbus::ft_script_t),
                          hdr.size,
                          0xFFFF)) {
            qDebug() << "wrong CRC" << hdr.crc;
            break;
        }
        QByteArray bsource = data.mid(static_cast<int>(sizeof(xbus::ft_script_t) + hdr.code_size));
        QString src = QString(qUncompress(bsource));
        if (src.isEmpty())
            break;
        pkg.append(src);
        QByteArray code = data.mid(static_cast<int>(sizeof(xbus::ft_script_t)),
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
    xbus::ft_script_t hdr;
    memset(&hdr, 0, sizeof(xbus::ft_script_t));
    hdr.size = static_cast<uint>(code.size() + basrc.size());
    hdr.code_size = static_cast<uint>(code.size());
    code.append(basrc);
    hdr.crc = CRC_16_IBM(reinterpret_cast<const quint8 *>(code.data()),
                         static_cast<uint>(code.size()),
                         0xFFFF);

    QByteArray data;
    data.append(reinterpret_cast<const char *>(&hdr), sizeof(hdr));
    data.append(code);

    ProtocolServiceFile *p = createFile(XbusNode::apc_script_file);
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
    request(XbusNode::apc_conf_write,
            QByteArray().append(static_cast<char>(id)).append(data),
            1500,
            false);
}
void ProtocolServiceNode::saveValues()
{
    service->setActive(true);
    request(XbusNode::apc_conf_write, QByteArray().append(static_cast<char>(0xFF)), 1500, false);
}
//=============================================================================
//=============================================================================
void ProtocolServiceNode::requestNstat()
{
    if (isSubNode())
        return;
    service->setActive(true);
    request(XbusNode::apc_nstat, QByteArray(), 1000, false);
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
    for (int i = 0; i < d.dict.fields.size(); ++i) {
        DictNode::Field &f = d.dict.fields[i];
        postprocessField(f);
        for (int j = 0; j < f.subFields.size(); ++j) {
            postprocessField(f.subFields[j]);
        }
    }
}
//=============================================================================
//=============================================================================
