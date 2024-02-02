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

#include <QtCore>

class PTrace : public QObject
{
    Q_OBJECT

public:
    explicit PTrace(QObject *parent = nullptr);

    bool enabled() const { return _enabled; }
    void enable(bool v);

    void reset();

    virtual void uplink();
    virtual void downlink(size_t sz = 0);
    virtual void end(size_t sz = 0);

    void block(QString block);
    void blocks(QStringList blocks);
    void data(QByteArray data);

    void tree() { block("+"); } // nested (wrapped) stream mark

    template<typename T>
    void raw(const T &r, QString name = QString())
    {
        if (!_enabled)
            return;
        if (!name.isEmpty())
            block(name.append(':'));
        data(QByteArray(reinterpret_cast<const char *>(&r), sizeof(r)));
    }

protected:
    bool _enabled{false};

private:
    QStringList _blocks;

signals:
    void packet(QStringList blocks);
};
