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

#include "QueueItem.h"

class ProtocolNodeFile;

class Loader : public QueueItem
{
    Q_OBJECT

public:
    explicit Loader(Fact *parent);

    void start(QueueItem *item, Releases *releases);
    void finish(bool success);

private:
    ProtocolNodeFile *file_p{nullptr};

    //files
    QByteArray fileData;
    quint32 startAddr;

    bool upgradeFirmware(QString node_sn, QByteArray data, quint32 offset);
    bool upgradeLoader(QString node_sn, QByteArray data, quint32 offset);

    ProtocolNodeFile *lock_file(const QString &node_sn, const QString &fname);

signals:
    void finished(bool success);
    void stop();
};
