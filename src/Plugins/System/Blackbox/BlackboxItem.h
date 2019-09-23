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
#ifndef BlackboxItem_H
#define BlackboxItem_H
//=============================================================================
#include <Fact/Fact.h>
class BlackboxReader;
//=============================================================================
class BlackboxItem : public Fact
{
    Q_OBJECT

public:
    explicit BlackboxItem(Fact *parent,
                          const QString &name,
                          const QString &title,
                          const QString &descr,
                          FactBase::Flags flags,
                          const QString &uid = QString());

    Fact *f_callsign;
    Fact *f_stats;

    Fact *f_start;
    Fact *f_stop;

protected:
    BlackboxReader *reader;

    QString uid;
    quint64 totalSize;

protected slots:
    virtual void updateActions();
    virtual void updateStats();

public slots:
    virtual void getStats();
    virtual void download();
    virtual void stop();
};
//=============================================================================
#endif
