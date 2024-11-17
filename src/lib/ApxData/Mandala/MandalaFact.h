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

#include <MandalaMeta.h>

#include <Fact/Fact.h>

class Mandala;
class MandalaConverter;

class MandalaFact : public Fact
{
    Q_OBJECT
    Q_PROPERTY(uint uid READ uid CONSTANT)

public:
    explicit MandalaFact(Mandala *tree, Fact *parent, const mandala::meta_s &meta);

    void addConverter(MandalaConverter *c) { _converters.append(c); }
    void removeConverter(MandalaConverter *c) { _converters.removeAll(c); }

    // send value to uplink when set
    bool setValue(const QVariant &v) override;

    Q_INVOKABLE mandala::uid_t uid() const;
    Q_INVOKABLE void request();
    Q_INVOKABLE void send();
    Q_INVOKABLE void sendValue(QVariant v);

    Q_INVOKABLE Fact *classFact() const;
    Q_INVOKABLE QString mpath() const;

    Q_INVOKABLE mandala::uid_t offset() const;

    // units conversions
    void setValueFromStream(const QVariant &v);

    bool setRawValueLocal(QVariant v);

    void increment_rx_cnt();
    auto rx_cnt() const { return _rx_cnt; }
    auto everReceived() const { return _everReceived; }

    void increment_tx_cnt();
    auto tx_cnt() const { return _tx_cnt; }
    auto everSent() const { return _everSent; }

    inline const mandala::meta_s &meta() const { return m_meta; }
    inline const mandala::fmt_s &fmt() const { return m_fmt; }

public:
    bool isSystem() const;
    bool isGroup() const;

protected:
    //Fact override
    virtual QVariant data(int col, int role) override;
    virtual bool showThis(QRegularExpression re) const override; //filter helper

private:
    Mandala *m_tree;
    const mandala::meta_s &m_meta;
    const mandala::fmt_s &m_fmt;

    QList<MandalaConverter *> _converters;

    QElapsedTimer sendTime;
    QTimer sendTimer;

    int getPrecision();
    QColor getColor();

    size_t _rx_cnt{};
    bool _everReceived{};

    size_t _tx_cnt{};
    bool _everSent{};

    void updateCounters();

    QVariant convertFromStream(QVariant v) const;
    QVariant convertForStream(QVariant v) const;
};
