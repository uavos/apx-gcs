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

#include <mandala/MandalaMetaBase.h>

#include <Fact/Fact.h>

class Mandala;

class MandalaFact : public Fact
{
    Q_OBJECT

public:
    explicit MandalaFact(Mandala *tree, Fact *parent, const mandala::meta_s &meta);

    // send value to uplink when set
    bool setValue(const QVariant &v) override;

    // set value locally and do not send uplink
    bool setValueLocal(const QVariant &v);

    Q_INVOKABLE mandala::uid_t uid() const;
    Q_INVOKABLE void request();
    Q_INVOKABLE void send();

    Q_INVOKABLE Fact *classFact() const;
    Q_INVOKABLE QString mpath() const;

    void addAlias(const QString &a);
    Q_INVOKABLE QString alias() const;
    Q_INVOKABLE mandala::uid_t offset() const;

    //stream
    void setValueFromStream(const QVariant &v);
    QVariant getValueForStream() const;

    void count_rx();

    inline const mandala::meta_s &meta() const { return m_meta; }

private:
    Mandala *m_tree;
    const mandala::meta_s &m_meta;
    QString m_alias;

    QElapsedTimer sendTime;
    QTimer sendTimer;

    int getPrecision();
    QColor getColor();

    size_t _rx_cnt{};

protected:
    //Fact override
    virtual QVariant data(int col, int role) const override;
    virtual bool showThis(QRegExp re) const override; //filter helper

    //---------------------------------------
    // PROPERTIES
public:
    bool isSystem() const;
};
