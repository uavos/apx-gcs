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
#ifndef MandalaTreeFact_H
#define MandalaTreeFact_H
//=============================================================================
#include <Fact/Fact.h>
#include <Mandala/MandalaMetaBase.h>
#include <Mandala/MandalaStream.h>
#include <QtCore>

#include "MandalaTreeStream.h"

class MandalaTree;

class MandalaTreeFact : public Fact
{
    Q_OBJECT
public:
    explicit MandalaTreeFact(MandalaTree *tree, Fact *parent, const mandala::meta_t &meta);

    bool setValue(const QVariant &v); //override
    bool setValueLocal(const QVariant &v);

    Q_INVOKABLE mandala::uid_t uid();
    Q_INVOKABLE void request();
    Q_INVOKABLE void send();

    size_t pack(void *buf) const;
    size_t unpack(const void *buf);

private:
    MandalaTree *m_tree;
    const mandala::meta_t &m_meta;

    QElapsedTimer sendTime;
    QTimer sendTimer;

    MandalaTreeStream *m_stream{nullptr};
    MandalaTreeStream *get_stream();

protected:
    //Fact override
    virtual QVariant data(int col, int role) const;
    virtual bool showThis(QRegExp re) const; //filter helper

private slots:
    void updateStatus();
    void updateDescr();

signals:
    void sendValueUpdate(quint16 id, double v);
    void sendValueRequest(quint16 id);
};
//=============================================================================
#endif
