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

#include <Protocols/ProtocolNode.h>
#include <Protocols/ProtocolViewBase.h>

class Releases;

class QueueItem : public ProtocolViewBase<ProtocolNode>
{
    Q_OBJECT

public:
    explicit QueueItem(Fact *parent, ProtocolNode *protocol, QString type);

    bool match(const QString &sn) const;
    QString type() const;
    void setType(QString v);

    void finish(bool success);

    QString format_name;
    QString format_hw;

protected:
    QString m_type;

    QByteArray _data;
    quint32 _offset;

    ProtocolNodeFile *file_p{nullptr};
    ProtocolNodeFile *file(const QString &fname);

    bool loadFirmware(QString hw, QString ver = QString());

    QList<QMetaObject::Connection> _clist;

private slots:
    void cleanUploadConnections();

protected slots:
    virtual void upload();

public slots:
    void start();

signals:
    void finished(QueueItem *item, bool success);
};
