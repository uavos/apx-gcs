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
#include "NodeItem.h"
#include "NodeField.h"
#include "NodeTools.h"
#include "Nodes.h"
#include <QtSql>

#include <App/App.h>
#include <Database/NodesDB.h>
#include <Vehicles/VehicleWarnings.h>
#include <Vehicles/Vehicles.h>
#include <QFontDatabase>
#include <QQmlEngine>
//=============================================================================
NodeItem::NodeItem(Nodes *parent, QString sn, ProtocolServiceNode *protocol)
    : NodeItemData(parent, sn)
    , lastSeenTime(0)
    , nconfID(0)
    , nodes(parent)
    , group(nullptr)
    , protocol(nullptr)
    , m_infoValid(false)
{
    qmlRegisterUncreatableType<NodeItem>("APX.Node", 1, 0, "Node", "Reference only");

    setIcon("sitemap");
    connect(this, &Fact::titleChanged, this, [=]() {
        if (title().endsWith("shiva"))
            setIcon("airplane");
    });

    sortNames << "shiva"
              << "nav"
              << "ifc"
              << "swc"
              << "cas"
              << "gps"
              << "mhx"
              << "servo"
              << "bldc";

    tools = new NodeTools(this, Action);

    connect(this, &NodeItem::versionChanged, this, &NodeItem::updateDescr);
    connect(this, &NodeItem::hardwareChanged, this, &NodeItem::updateDescr);
    connect(this, &NodeItem::upgradingChanged, this, &NodeItem::updateDescr);
    connect(this, &NodeItem::offlineChanged, this, &NodeItem::updateDescr);
    connect(this, &NodeItem::fwUpdatingChanged, this, &NodeItem::updateDescr);

    connect(this, &NodeItem::titleChanged, this, &NodeItem::nodeNotify);
    connect(this, &NodeItem::hardwareChanged, this, &NodeItem::nodeNotify);
    connect(this, &NodeItem::versionChanged, this, &NodeItem::nodeNotify);
    connect(this, &NodeItem::statusChanged, this, &NodeItem::nodeNotify);

    //validity
    connect(this, &NodeItem::dictValidChanged, this, &NodeItem::validateDict);
    connect(this, &NodeItem::dataValidChanged, this, &NodeItem::validateData);
    connect(this, &NodeItem::infoValidChanged, this, &NodeItem::validateInfo);

    connect(this, &NodeItem::reconfChanged, this, &NodeItem::updateReconf);

    nstatTimer.setSingleShot(true);
    nstatTimer.setInterval(10000);
    connect(&nstatTimer, &QTimer::timeout, this, &NodeItem::updateDescr);

    if (protocol) {
        nodes->storage->loadNodeInfo(this);
    }

    //protocol
    setProtocol(protocol);

    setFwSupport(!sn.startsWith("FF"));
    nodeNotify();

    emit requestInfo();
}
//=============================================================================
void NodeItem::setProtocol(ProtocolServiceNode *protocol)
{
    if (this->protocol == protocol)
        return;
    if (this->protocol) {
        disconnect(this->protocol);
    }
    this->protocol = protocol;
    connect(protocol, &QObject::destroyed, this, [this]() {
        this->protocol = nullptr;
        nodes->removeNode(sn());
    });
    connect(protocol, &ProtocolServiceNode::messageReceived, this, &NodeItem::messageReceived);
    connect(protocol, &ProtocolServiceNode::infoReceived, this, &NodeItem::infoReceived);
    connect(protocol, &ProtocolServiceNode::dictInfoReceived, this, &NodeItem::dictInfoReceived);
    connect(protocol, &ProtocolServiceNode::dictReceived, this, &NodeItem::dictReceived);
    connect(protocol, &ProtocolServiceNode::valuesReceived, this, &NodeItem::valuesReceived);
    connect(protocol,
            &ProtocolServiceNode::valueModifiedExternally,
            this,
            &NodeItem::valueModifiedExternally);
    connect(protocol, &ProtocolServiceNode::valueUploaded, this, &NodeItem::valueUploaded);
    connect(protocol, &ProtocolServiceNode::valuesSaved, this, &NodeItem::valuesSaved);
    connect(protocol, &ProtocolServiceNode::nstatReceived, this, &NodeItem::nstatReceived);
    connect(protocol,
            &ProtocolServiceNode::unknownServiceData,
            this,
            &NodeItem::serviceDataReceived);

    connect(protocol, &ProtocolServiceNode::requestTimeout, this, &NodeItem::requestTimeout);

    connect(protocol, &ProtocolServiceNode::progressChanged, this, [this, protocol]() {
        setProgress(protocol->progress());
    });

    if (!nodes->vehicle->isTemporary()) {
        connect(this, &NodeItem::requestInfo, protocol, &ProtocolServiceNode::requestInfo);
        connect(this, &NodeItem::requestDict, protocol, &ProtocolServiceNode::requestDict);
        connect(this, &NodeItem::requestValues, protocol, &ProtocolServiceNode::requestValues);
        connect(this, &NodeItem::uploadValue, protocol, &ProtocolServiceNode::uploadValue);
        connect(this, &NodeItem::saveValues, protocol, &ProtocolServiceNode::saveValues);
        connect(this, &NodeItem::requestNstat, protocol, &ProtocolServiceNode::requestNstat);
        connect(this, &NodeItem::requestUser, protocol, &ProtocolServiceNode::requestUser);
        connect(this,
                &NodeItem::acknowledgeRequest,
                protocol,
                &ProtocolServiceNode::acknowledgeRequest);
    }

    setDataValid(false);
    setDictValid(false);
    setInfoValid(false);
    emit offlineChanged();
    emit requestInfo();
}
//=============================================================================
void NodeItem::validateDict()
{
    if (!dictValid())
        return;
    groupFields();
    //qDebug()<path();

    //fields map
    for (int i = 0; i < allFields.size(); ++i) {
        NodeField *f = allFields.at(i);
        //expand complex numbers
        bool bComplex = f->size() > 0;
        if (bComplex) {
            for (int i2 = 0; i2 < f->size(); ++i2) {
                NodeField *f2 = f->child<NodeField>(i2);
                allFieldsByName.insert(f2->name(), f2);
            }
        } else
            allFieldsByName.insert(f->name(), f);
    }
}
void NodeItem::validateData()
{
    if (!dataValid())
        return;
    if (reconf()) {
        nodes->storage->restoreNodeConfig(this);
    } else if (!offline()) {
        setNconfID(0);
        nodes->storage->saveNodeConfig(this);
    }
    nodeNotify();
    //qDebug()<<"Node dataValid"<<path();
}
void NodeItem::validateInfo()
{
    if (!infoValid())
        return;
    groupNodes();
    if (upgrading()) {
        //setDictValid(true);
        return;
    }
    if (protocol) {
        nodes->storage->saveNodeInfo(this);
        nodes->storage->saveNodeUser(this);
    }
    //App::jsync(this);
    //qDebug()<<"Node infoValid"<<path();
}
void NodeItem::updateReconf()
{
    if (!reconf())
        setDataValid(false);
}
void NodeItem::setDictInfo(QVariantMap dictInfo)
{
    this->dictInfo = dictInfo;
    //qDebug()<<dictInfo;//.keys();
}
void NodeItem::setNconfID(quint64 nconfID)
{
    this->nconfID = nconfID;
    if (nconfID == 0)
        return;
    //qDebug() << nconfID;
    if (nodes->storage->loading)
        return;
    nodes->nconfSavedSync();
}
//=============================================================================
//=============================================================================
void NodeItem::clear()
{
    tools->clearCommands();
    allFields.clear();
    allFieldsByName.clear();
    removeAll();
    setDictValid(false);
    setDataValid(false);
    setModified(false);
    //clear subnode (.shiva)
    if (upgrading()) {
        NodeItem *n = subNode();
        if (n)
            nodes->removeNode(n->sn());
    }
}
//=============================================================================
void NodeItem::updateDescr()
{
    nstatTimer.stop();
    setActive(false);
    QStringList st;
    st.append(m_hardware);
    if (m_version != App::version())
        st.append(m_version);
    if (offline())
        st.append(tr("offline"));
    if (fwUpdating())
        st.append("LOADER");
    //st.append(sn());
    setDescr(st.join(' '));
}
//=============================================================================
void NodeItem::nodeNotify()
{
    if (offline())
        return;
    //if (m_version != App::version() && fwSupport()) {
    Vehicles::instance()->nodeNotify(this);
    //}
}
//=============================================================================
void NodeItem::upload()
{
    if (!(dictValid() && dataValid()))
        return;
    if (!modified())
        return;
    saveTelemetryUploadEvent();
    int cnt = 0;
    foreach (NodeField *f, allFields) {
        if (!f->modified())
            continue;
        emit uploadValue(f->id, f->uploadableValue());
        cnt++;
    }
    if (cnt > 0)
        emit saveValues();
}
//=============================================================================
void NodeItem::upgradeFirmware()
{
    if (!protocol)
        return;
    Vehicles::instance()->nodeUpgradeFW(this);
}
void NodeItem::upgradeLoader()
{
    if (!protocol)
        return;
    Vehicles::instance()->nodeUpgradeLD(this);
}
void NodeItem::upgradeRadio()
{
    if (!protocol)
        return;
    Vehicles::instance()->nodeUpgradeMHX(this);
}
//=============================================================================
//=============================================================================
void NodeItem::saveTelemetryUploadEvent()
{
    foreach (NodeField *f, allFields) {
        if (!f->modified())
            continue;
        if (f->size()) {
            for (int i = 0; i < f->size(); ++i) {
                NodeField *fx = f->child<NodeField>(i);
                if (!fx->modified())
                    continue;
                saveTelemetryConf(fx);
            }
        } else {
            saveTelemetryConf(f);
        }
    }
}
void NodeItem::saveTelemetryConf(NodeField *f)
{
    nodes->vehicle->recordConfigUpdate(title(), f->name(), f->toString(), sn());
}
//=============================================================================
int NodeItem::loadConfigValues(QVariantMap values)
{
    QStringList ignoredValues = values.keys();
    QStringList missingValues;
    int rcnt = 0;
    foreach (const QString &fieldName, allFieldsByName.keys()) {
        if (!values.contains(fieldName)) {
            missingValues.append(fieldName);
            continue;
        }
        if (loadConfigValue(fieldName, values.value(fieldName).toString())) {
            ignoredValues.removeOne(fieldName);
            rcnt++;
        }
    }
    //report
    QString sname = title();
    if (!status().isEmpty())
        sname.append(QString("-%1").arg(status()));
    int cnt = missingValues.size();
    if (cnt > 0) {
        QString s = QString("%1: %2").arg(sname).arg(cnt);
        if (cnt < 5)
            s.append(QString(" (%1)").arg(missingValues.join(',')));
        nodes->vehicle->message(tr("Missing values").append(": ").append(s), AppNotify::Warning);
        qWarning() << missingValues;
    }
    cnt = ignoredValues.size();
    if (cnt > 0) {
        QString s = QString("%1: %2").arg(sname).arg(cnt);
        if (cnt < 5)
            s.append(QString(" (%1)").arg(ignoredValues.join(',')));
        nodes->vehicle->message(tr("Ignored values").append(": ").append(s), AppNotify::Warning);
        qWarning() << ignoredValues;
    }
    return rcnt;
}
bool NodeItem::loadConfigValue(const QString &name, const QString &value)
{
    NodeField *f = allFieldsByName.value(name);
    if (!f)
        return false;
    if ((name == "comment" || name == "node_label") && value.isEmpty() && (!f->text().isEmpty()))
        return true;
    f->fromString(value);
    return true;
}
//=============================================================================
//=============================================================================
void NodeItem::message(QString msg)
{
    if (nodes->vehicle->isTemporary())
        return;

    QStringList st = msg.trimmed().split('\n', QString::SkipEmptyParts);
    for (auto s : st) {
        s = s.trimmed();
        if (s.isEmpty())
            continue;
        App::sound(s);
        nodes->vehicle->message(qApp->translate("msg", s.toUtf8().data()),
                                AppNotify::FromVehicle | AppNotify::Important,
                                title());

        //record
        QString nodeName = QString("%1/%2").arg(nodes->vehicle->callsign()).arg(title());
        nodes->vehicle->recordNodeMessage(nodeName, s, sn());
    }
}
//=============================================================================
void NodeItem::groupFields(void)
{
    //qDebug()<<"groupFields ----------- "<<allFields.size()<<conf_hash;
    for (auto f : allFields) {
        //grouping
        Fact *groupItem = nullptr;
        Fact *groupParent = this;
        foreach (QString group, f->groups) {
            groupItem = nullptr;
            QString gname = group.toLower();
            for (int i = 0; i < groupParent->size(); ++i) {
                Fact *f = groupParent->child(i);
                if (!(f->treeType() == Group && f->title().toLower() == gname))
                    continue;
                groupItem = f;
                break;
            }
            if (!groupItem) {
                groupItem = new NodesBase(groupParent, gname, group, "", Group);
                //qDebug()<<"GRP NEW"<<gname;
            }
            groupParent = groupItem;
            f->setParentFact(groupItem);
            f->setSection("");
        }
        if (!f->parentFact())
            f->setParentFact(this);
    }
    //hide grouped arrays (gpio, controls etc)
    QSet<NodesBase *> groups;
    for (auto f : allFields) {
        NodesBase *groupItem = qobject_cast<NodesBase *>(f->parentFact());
        if (!groupItem || groupItem == this)
            continue;
        //continue;
        if (groupItem && groupItem->size() >= 2) {
            //check if group members are arrays
            bool bArray = false;
            uint cnt = 0;
            for (int i = 0; i < groupItem->size(); ++i) {
                NodeField *f = groupItem->child<NodeField>(i);
                if (!f) {
                    //qDebug() << groupItem->path() << i << groupItem->child(i);
                    continue;
                }
                bArray = false;
                if (cnt < 2 && f->dtype != DictNode::Array)
                    break;
                //if(cnt<2 && f->size()<=1)break;
                //if (f->size() < 1)break;
                cnt++;
                bArray = true;
            }
            if (bArray)
                groups.insert(groupItem);
        }
    }
    for (auto group : groups) {
        //qDebug() << group->path();
        groupArrays(group);
    }
}
//=============================================================================
void NodeItem::groupArrays(NodesBase *group)
{
    //create action with underlaying table structure to edit arrays rows

    Fact *action = new Fact(group, group->name(), group->title(), group->descr(), Action);
    group->bind(action);
    group->setModel(action->model());

    connect(group->child(0), &Fact::statusChanged, group, [group]() {
        group->setStatus(group->child(0)->status());
    });
    group->setStatus(group->child(0)->status());

    //hide group members
    for (int i = 0; i < group->size(); ++i) {
        NodeField *f = group->child<NodeField>(i);
        if (!f)
            continue;
        f->setVisible(false);
    }

    Fact *f1 = group->child(0);
    int colCnt = group->size();
    for (int row = 0; row < f1->size(); ++row) {
        Fact *fi = f1->child(row);
        Fact *fRow = new Fact(action, fi->name(), fi->title(), "", Fact::Group);
        //fRow->setActionsModel(group->actionsModel());
        connect(group, &Fact::modifiedChanged, fRow, [fRow, group]() {
            fRow->setModified(group->modified());
        });
        connect(fi, &Fact::textChanged, fRow, [fRow, fi]() { fRow->setStatus(fi->text()); });
        connect(fi, &Fact::textChanged, this, [this, fRow]() { updateArrayRowDescr(fRow); });

        Fact *f_ch = nullptr;
        for (int i = 0; i < colCnt; ++i) {
            Fact *fArray = group->child(i);
            if (!fArray)
                continue;
            Fact *fp;
            bool bChParam = false;
            if (f_ch && fArray->name().startsWith("ctr_ch_")) {
                fp = fArray->child(f_ch->value().toInt());
                bChParam = true;
            } else {
                fp = fArray->child(row);
            }
            if (!fp)
                continue;
            Fact *f = new Fact(fRow,
                               fArray->name(),
                               fArray->title(),
                               fArray->descr(),
                               fp->treeType() | fp->dataType());
            f->bind(fp);
            connect(f, &Fact::textChanged, fRow, [this, fRow]() { updateArrayRowDescr(fRow); });
            if (bChParam) {
                connect(f_ch, &Fact::valueChanged, f, [f, fArray, f_ch]() {
                    f->bind(fArray->child(f_ch->value().toInt()));
                });
            }

            if (f->name() == "ctr_ch") {
                f_ch = f;
            }
        }
    }
}
void NodeItem::updateArrayRowDescr(Fact *fRow)
{
    QStringList st;
    if (!fRow->status().isEmpty()) {
        for (int i = 0; i < fRow->size(); ++i) {
            st.append(fRow->child(i)->text());
        }
    }
    fRow->setDescr(st.join(", "));
}
//=============================================================================
void NodeItem::groupNodes(void)
{
    //check node grouping
    if (group)
        return;
    NodeItemBase *ngroup = nullptr;
    //find same names with same parent
    QList<NodeItem *> nlist;
    QString stitle = title();
    if (stitle.endsWith(".shiva"))
        stitle.remove(0, stitle.lastIndexOf('.') + 1);
    QString gname = stitle;
    QStringList names;
    names.append(stitle);
    if (stitle == "bldc") {
        gname = "servo";
        names.append(gname);
    }
    foreach (NodeItem *i, nodes->nodes()) {
        QString s = i->title();
        if (s.endsWith(".shiva"))
            s.remove(0, s.lastIndexOf('.') + 1);
        if (names.contains(s))
            nlist.append(i);
    }
    foreach (NodeItemBase *g, nodes->nGroups) {
        if (g->size() > 0 && g->title() == gname.toUpper()) {
            ngroup = g;
            break;
        }
    }

    if (ngroup == nullptr && nlist.size() < 2) {
        //nodes->f_list->addChild(this);
        return;
    }
    //qDebug()<<"-append-";

    if (ngroup)
        group = ngroup;
    else {
        group = new NodeItemBase(nodes, gname, gname.toUpper(), Section);
        group->setSection(section());
        nodes->nGroups.append(group);
        //qDebug()<<"grp: "<<gname;
    }

    foreach (NodeItem *i, nlist) {
        if (i->parentFact() == group)
            continue;
        i->setParentFact(group);
        i->group = group;
        i->setName(i->name()); //update unique name
        i->setSection(group->title());
        //qDebug()<<gname<<"<<"<<i->name;
        //if(node->name.contains("shiva")) qDebug()<<node->name<<nlist.size()<<(group?group->name:"");
    }
    //update group descr
    QStringList gNames, gHW;
    for (int i = 0; i < group->size(); ++i) {
        NodeItem *n = group->child<NodeItem>(i);
        if (!gNames.contains(n->title()))
            gNames.append(n->title());
        if (!gHW.contains(n->hardware()))
            gHW.append(n->hardware());
    }
    QStringList sdescr;
    if (!gNames.isEmpty())
        sdescr.append(gNames.join(','));
    if (!gHW.isEmpty())
        sdescr.append("(" + gHW.join(',') + ")");
    if (!sdescr.isEmpty())
        group->setDescr(sdescr.join(' '));
    group->setStatus(QString("[%1]").arg(group->size()));
    //group->updateDictValid();
    //group->updateDataValid();
}
//=============================================================================
//=============================================================================
QString NodeItem::version() const
{
    return m_version;
}
void NodeItem::setVersion(const QString &v)
{
    if (m_version == v)
        return;
    m_version = v;
    emit versionChanged();
}
QString NodeItem::hardware() const
{
    return m_hardware;
}
void NodeItem::setHardware(const QString &v)
{
    if (m_hardware == v)
        return;
    m_hardware = v;
    emit hardwareChanged();
}
bool NodeItem::infoValid() const
{
    return m_infoValid;
}
void NodeItem::setInfoValid(const bool &v)
{
    if (m_infoValid == v)
        return;
    m_infoValid = v;
    emit infoValidChanged();
}
bool NodeItem::offline() const
{
    return !protocol;
}
//=============================================================================
//=============================================================================
QVariant NodeItem::data(int col, int role) const
{
    if (dictValid() && dataValid() && (!upgrading())) {
        switch (role) {
        case Qt::ForegroundRole:
            //if(isUpgrading())return QColor(Qt::white);
            //return col==FACT_MODEL_COLUMN_NAME?QColor(Qt::darkYellow):QColor(Qt::darkGray);
            //if(!statsShowTimer.isActive()) return isUpgradable()?QColor(Qt::red).lighter():Qt::darkGray;
            //nstats
            //return statsWarn?QColor(Qt::yellow):QColor(Qt::green);
            break;
        case Qt::BackgroundRole:
            //if(isUpgrading())return QColor(0x20,0x00,0x00);
            //if(isUpgradePending())return QColor(0x40,0x00,0x00);
            if (reconf())
                return QColor(Qt::darkGray).darker(200);
            return QColor(0x20, 0x40, 0x60);
        }
    }
    if (role == Qt::FontRole && col == Fact::FACT_MODEL_COLUMN_DESCR) {
#ifdef Q_OS_MAC
        return QFont("Menlo");
#else
        return QFont("FreeMono");
#endif
    }
    return NodeItemData::data(col, role);
}
//=============================================================================
void NodeItem::hashData(QCryptographicHash *h) const
{
    Fact::hashData(h);
    h->addData(version().toUtf8());
    h->addData(hardware().toUtf8());
    h->addData(conf_hash.toUtf8());
    h->addData(QString::number(fwSupport()).toUtf8());
}
//=============================================================================
NodeItem *NodeItem::subNode() const
{
    foreach (NodeItem *i, nodes->nodes()) {
        if (i->title().startsWith(title().append('.')))
            return i;
    }
    return nullptr;
}
//=============================================================================
//=============================================================================
void NodeItem::execCommand(quint16 cmd, const QString &name, const QString &descr)
{
    nodes->vehicle->message(descr + "...", AppNotify::Important, title());
    emit requestUser(cmd, QByteArray(), 1000);
    if (name.startsWith("conf") || name.contains("reconf")) {
        setDataValid(false);
        //invalidate subnode (.shiva)
        NodeItem *n = subNode();
        if (n)
            n->setDataValid(false);
        nodes->syncLater();
    }
}
//=============================================================================
//=============================================================================
// Protocols connection
//=============================================================================
void NodeItem::messageReceived(const QString &msg)
{
    message(msg);
    if ((reconf() || fwUpdating() || allFields.isEmpty()) && (!modified())) {
        if (msg.contains(title()) && msg.contains("init")) {
            nodes->syncLater(1000);
        }
    }
}
//=============================================================================
void NodeItem::infoReceived(const DictNode::Info &info)
{
    //qDebug()<<"ok";
    lastSeenTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    setName(info.name);
    setTitle(info.name);
    setVersion(info.version);
    setHardware(info.hardware);
    setReconf(info.reconf);
    setFwSupport(info.fwSupport || info.fwUpdating);
    setFwUpdating(info.fwUpdating);
    setAddressing(info.addressing);
    setRebooting(info.rebooting);
    setBusy(info.busy);
    setFailure(false);
    setInfoValid(true);

    nodeNotify();
    /*if(fwUpdating()){
    //setUpgrading(true);
    //setProgress(-1);
    clear();
    return;
  }*/
}
void NodeItem::dictInfoReceived(const DictNode::DictInfo &conf)
{
    //qDebug()<<"ok";
    updateDescr(); //reset nstat view
    //updateProgress();
    if (upgrading())
        return;
    if (conf.paramsCount != allFields.size() || conf.chash != conf_hash) {
        clear();
        conf_hash = conf.chash;
    }
    if (conf.paramsCount == 0) {
        setDictValid(true);
        setDataValid(true);
    }
    //nodes->vehicle->dbSaveVehicleNodes();
    if (!dictValid()) {
        if (nodes->skipCache.contains(sn())) {
            emit requestDict();
        } else {
            nodes->storage->loadDictCache(this);
        }
        return;
    }
    if (!dataValid()) {
        requestFieldValues();
    }
}
void NodeItem::dictReceived(const DictNode::Dict &dict)
{
    //qDebug()<<"dictReceived"<<name()<<dict.fields.size();
    if (dictValid()) {
        qDebug() << "NodeItem::dictReceived"
                 << "dictValid" << name() << dict.fields.size();
        requestFieldValues();
        return;
    }
    clear();
    conf_hash = dict.chash;
    foreach (const DictNode::Command &c, dict.commands) {
        tools->addCommand(c.name, c.descr, "", c.cmd);
    }
    for (int i = 0; i < dict.fields.size(); ++i) {
        dictCreateField(dict.fields.at(i), nullptr);
    }
    setDictValid(true);
    requestFieldValues();
    //App::jsync(this);

    nodes->skipCache.removeAll(sn());
    if (dictInfo.value("hash").toString().isEmpty()) {
        nodes->storage->saveDictCache(this, dict);
    }
    //provide cached dict to protocol
    if (lastSeenTime <= 0)
        return;
    qDebug() << title() << (dict.cached ? "cached" : "synced");
    if (protocol)
        protocol->loadCachedDict(dict);
}
NodeField *NodeItem::dictCreateField(const DictNode::Field &f, NodeField *parentField)
{
    NodeField *nf = new NodeField(
        this, f.id, f.type, f.name, f.title, f.descr, f.units, f.opts, f.groups, parentField);
    foreach (const DictNode::Field &fs, f.subFields) {
        dictCreateField(fs, nf);
    }
    return nf;
}
//=============================================================================
void NodeItem::requestFieldValues()
{
    if (dataValid())
        return;
    for (int i = 0; i < allFields.size(); ++i) {
        NodeField *f = allFields.at(i);
        if (f->dataValid())
            continue;
        emit requestValues(f->id);
        break;
    }
}
void NodeItem::valuesReceived(quint16 id, const QVariantList &values)
{
    //qDebug()<<"valuesReceived"<<name()<<id<<values.size();
    for (int i = 0; i < values.size(); ++i) {
        NodeField *field = allFields.value(id, nullptr);
        if (!field)
            return;
        QVariant v = values.at(i);
        if (v.isValid()) {
            field->setValue(v);
            field->setDataValid(true);
        }
        id++;
    }
    requestFieldValues();
}
void NodeItem::valueModifiedExternally(quint16 id)
{
    NodeField *field = allFields.value(id, nullptr);
    if (!field)
        return;
    message(QString("%1: %2=%3").arg(tr("Field modified")).arg(field->title()).arg(field->text()));
}
void NodeItem::valueUploaded(quint16 id)
{
    //qDebug()<<"NodeItem::valueUploaded"<<id;
    NodeField *field = allFields.value(id, nullptr);
    if (!field)
        return;
    field->backup();
}
void NodeItem::valuesSaved()
{
    //qDebug()<<"NodeItem::valuesSaved";
    if (reconf()) {
        nodes->syncLater(3000);
    } else {
        setNconfID(0);
        nodes->storage->saveNodeConfig(this);
        nodes->uploadedSync();
    }
}
//=============================================================================
void NodeItem::nstatReceived(const DictNode::Stats &nstat)
{
    setVbat(nstat.vbat);
    setIbat(nstat.ibat);
    setErrCnt(nstat.errCnt);
    setCanRxc(nstat.canRxc);
    setCanAdr(nstat.canAdr);
    setCanErr(nstat.canErr);
    setCpuLoad(nstat.cpuLoad);
    //print report
    QString snode;
    snode = QString("%1:%2 E:%3 C:%4 %5%")
                .arg(canAdr(), 2, 16, QChar('0'))
                .arg(canRxc(), 2, 16, QChar('0'))
                .arg(errCnt(), 2, 16, QChar('0'))
                .arg(canErr(), 2, 16, QChar('0'))
                .arg(cpuLoad())
                .toUpper();
    if (vbat() > 0)
        snode += QString("\t%1V %2mA").arg(vbat(), 0, 'f', 1).arg(static_cast<int>(ibat() * 1000.0));
    if (nodes->vehicle == Vehicles::instance()->current()) {
        apxMsg() << QString("#[%1]%2").arg(title()).arg(snode);
        if (nstat.dump.count('\0') != nstat.dump.size()) {
            QString s(nstat.dump.toHex().toUpper());
            for (int i = 2; i < s.size(); i += 3) {
                s.insert(i, ' ');
            }
            apxMsg() << QString("#%1").arg(s);
        }
    }
    setDescr(snode);
    setActive(true);
    nstatTimer.start();
}
//=============================================================================
//=============================================================================
