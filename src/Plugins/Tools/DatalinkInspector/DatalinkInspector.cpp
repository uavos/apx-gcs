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
#include "DatalinkInspector.h"

#include <App/AppGcs.h>
#include <Protocols/Protocols.h>

DatalinkInspector::DatalinkInspector(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("DatalinkInspector"),
           tr("System terminal"),
           Group,
           "teamviewer")
{
    qmlRegisterUncreatableType<DatalinkInspector>("APX.DatalinkInspector",
                                                  1,
                                                  0,
                                                  "DatalinkInspector",
                                                  "Reference only");

    m_packetsModel = new PTraceListModel(this);
    m_uidModel = new PTraceListModel(this);

    m_proxyModel = new PTraceFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_packetsModel);

    connect(AppGcs::instance()->f_datalink->f_protocols,
            &Protocols::trace_packet,
            m_packetsModel,
            &PTraceListModel::append);

    connect(AppGcs::instance()->f_datalink->f_protocols,
            &Protocols::trace_packet,
            this,
            &DatalinkInspector::append_uid);

    connect(this, &Fact::activeChanged, this, [this]() {
        AppGcs::instance()->f_datalink->f_protocols->setTraceEnabled(active());
    });

    connect(this, &Fact::activeChanged, this, &DatalinkInspector::clear);

    setOpt("page", "qrc:/" PLUGIN_NAME "/DatalinkInspector.qml");
    loadQml("qrc:/" PLUGIN_NAME "/DatalinkInspectorPlugin.qml");
}

void DatalinkInspector::clear()
{
    m_packetsModel->clear();
    m_uidModel->clear();
    m_proxyModel->clear_filter();
    _uid_cnt.clear();
}

void DatalinkInspector::append_uid(QStringList blocks)
{
    for (auto s : blocks) {
        if (s.startsWith('$'))
            append_uid_item(s.mid(1));
    }
}
void DatalinkInspector::append_uid_item(QString uid)
{
    auto items = m_uidModel->items();

    for (int i = 0; i < items.size(); ++i) {
        if (items.at(i).at(0) != uid)
            continue;
        uint cnt = ++_uid_cnt[i];
        m_uidModel->updateItem(i, QStringList() << uid << QString("[%1]").arg(cnt));
        return;
    }
    // create new
    _uid_cnt.append(1);
    m_uidModel->append(QStringList() << uid);
}

void DatalinkInspector::filter(QString uid, bool exclude)
{
    uid.prepend('$');
    if (exclude) {
        m_proxyModel->add_filter(uid);
    } else {
        m_proxyModel->remove_filter(uid);
    }
}
