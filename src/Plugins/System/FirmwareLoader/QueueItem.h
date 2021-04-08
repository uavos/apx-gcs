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

#include <Fact/Fact.h>

class PFirmware;

class QueueItem : public Fact
{
    Q_OBJECT

public:
    explicit QueueItem(Fact *parent, QString uid, QString name, QString hw, QString type);

    bool match(const QString &uid) const;

    auto uid() const { return _uid; }
    auto type() const { return _type; }
    void setType(QString v);

    void finish(bool success);

protected:
    QString _uid;
    QString _name;
    QString _hw;
    QString _type;

    QByteArray _data;
    quint32 _offset;

    bool loadFirmware(QString hw);
    PFirmware *protocol() const;

protected slots:
    virtual void upload();
    void upgradeFinished(QString uid, bool success);

public slots:
    void start();

signals:
    void finished(QueueItem *item, bool success);
};
