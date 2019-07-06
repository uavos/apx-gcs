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
#include "QueueWorker.h"
//=============================================================================
QueueWorker::QueueWorker()
    : QThread()
{
    connect(this, &QThread::finished, this, &QueueWorker::threadFinished);
}
QueueWorker::~QueueWorker()
{
    //qDebug()<<"DEL QueueWorker";
    if (isRunning()) {
        requestInterruption();
        int to = 5;
        while (isRunning() && (to--))
            wait(100);
        //terminate();
    }
}
//=============================================================================
void QueueWorker::exec(Fact *f)
{
    fact = f;
}
//=============================================================================
void QueueWorker::threadFinished()
{
    emit workFinished(fact, result);
}
//=============================================================================
void QueueWorker::run()
{
    emit progress(fact, 0);
}
//=============================================================================
void QueueWorker::stop()
{
    requestInterruption();
    emit stopRequested();
}
//=============================================================================
