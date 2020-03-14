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

private:
    QString m_type;

    QByteArray _data;
    quint32 _offset;

    ProtocolNodeFile *file_p{nullptr};
    ProtocolNodeFile *file(const QString &fname);

    bool loadFirmware();

    QList<QMetaObject::Connection> _clist;

private slots:
    void cleanUploadConnections();

    void upload();

public slots:
    void start();

signals:
    void finished(QueueItem *item, bool success);
};
