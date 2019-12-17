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
#ifndef DatabaseWorker_H
#define DatabaseWorker_H
//=============================================================================
#include "DatabaseRequest.h"
#include <atomic>
#include <deque>
#include <QtCore>
#include <QtSql>
class DatabaseSession;
//=============================================================================
class DatabaseWorker : public QThread
{
    Q_OBJECT

public:
    explicit DatabaseWorker(DatabaseSession *db, QObject *parent = nullptr);
    ~DatabaseWorker() override;

    void request(DatabaseRequest *req);

    int queueSize();
    int rate();

protected:
    void run() override;

private:
    DatabaseSession *db;

    std::deque<DatabaseRequest *> queue;
    QReadWriteLock queueMutex;
    QWaitCondition waitCondition;

    //info
    int rcnt;
    std::atomic_int m_rate;
    QElapsedTimer infoUpdateTime;
    bool infoUpdate(bool force);
    void enqueue(DatabaseRequest *req);
    void eraseRequests(int count);

signals:
    void infoChanged();
};
//=============================================================================
#endif
