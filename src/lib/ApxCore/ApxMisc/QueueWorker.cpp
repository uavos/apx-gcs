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
