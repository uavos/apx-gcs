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
#ifndef BlackboxFile_H
#define BlackboxFile_H
//=============================================================================
#include "BlackboxItem.h"
#include <Fact/Fact.h>
//=============================================================================
class BlackboxFile : public BlackboxItem
{
    Q_OBJECT

public:
    explicit BlackboxFile(Fact *parent);

private:
    QFileInfo fi;
    QFile file;

protected:
    void updateStats();

private slots:
    void readNext();

public slots:
    virtual void getStats();
    virtual void download();
    virtual void stop();
};
//=============================================================================
#endif
