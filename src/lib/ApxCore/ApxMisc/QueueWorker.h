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
#ifndef QueueWorker_H
#define QueueWorker_H
//=============================================================================
#include <Fact/Fact.h>
#include <QtCore>
//=============================================================================
class QueueWorker : public QThread
{
    Q_OBJECT

public:
    explicit QueueWorker();
    ~QueueWorker();

    virtual void exec(Fact *f);

protected:
    Fact *fact;
    QVariantMap result;

    virtual void run();

private slots:
    void threadFinished();

signals:
    void progress(Fact *f, int v);
    void workFinished(Fact *f, QVariantMap result);
    void stopRequested();

public slots:
    void stop();
};
//=============================================================================
#endif
