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
#include "QueueJob.h"
//=============================================================================
QueueJob::QueueJob(Fact *parent,
                   const QString &name,
                   const QString &title,
                   const QString &descr,
                   QueueWorker *worker)
    : Fact(parent, name, title, descr, Group | Count)
    , worker(worker)
{
    worker->setParent(this);
    connect(worker, &QueueWorker::progress, this, &QueueJob::itemProgress, Qt::QueuedConnection);
    connect(worker, &QueueWorker::workFinished, this, &QueueJob::itemFinished, Qt::QueuedConnection);
    connect(this, &Fact::sizeChanged, this, &QueueJob::next);
}
//==============================================================================
void QueueJob::next()
{
    if (worker->isRunning())
        return;
    Fact *f = child(0);
    if (!f) {
        setProgress(-1);
        return;
    }
    worker->exec(f);
}
void QueueJob::itemProgress(Fact *f, int v)
{
    if (!hasChild(f))
        return;
    f->setProgress(v);
    if (v >= 0)
        setProgress(v);
}
void QueueJob::itemFinished(Fact *f, QVariantMap result)
{
    setProgress(0);
    if (hasChild(f)) {
        emit finished(f, result);
        f->deleteFact();
    } else
        f = nullptr;
    if (size() > 0) {
        QTimer::singleShot(100, this, &QueueJob::next);
    } else {
        setProgress(-1);
        if (f) {
            emit jobDone(result);
        }
    }
}
//=============================================================================
void QueueJob::stop()
{
    deleteChildren();
    worker->stop();
    setProgress(-1);
}
//=============================================================================
//=============================================================================
