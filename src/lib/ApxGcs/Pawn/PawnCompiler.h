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
#ifndef PawnCompiler_H
#define PawnCompiler_H
#include <Fact/Fact.h>
//=============================================================================
class PawnCompiler : public QObject
{
    Q_OBJECT
public:
    explicit PawnCompiler(Fact *fact);

    bool compile();

    QString getLog();
    bool error();

    const QByteArray &outData() const; //binary compiled data

private:
    Fact *fact;

    //compiler
    QProcess pawncc;
    QTemporaryFile tmpFile;
    QString outFileName;
    QString pawncc_log;
    QByteArray m_outData;
    bool m_error;

signals:
    void compiled();
};
//=============================================================================
#endif
