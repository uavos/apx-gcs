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
#include "DatabaseWorker.h"
#include "DatabaseSession.h"
#include <ApxLog.h>
//=============================================================================
DatabaseWorker::DatabaseWorker(DatabaseSession *db, QObject *parent)
    : QThread(parent)
    , db(db)
{
    setObjectName(db->sessionName.append("_worker"));

    rcnt = m_rate = 0;
    infoUpdateTime.start();
    start();
}
DatabaseWorker::~DatabaseWorker()
{
    requestInterruption();
    waitCondition.notify_all();
    qDebug() << "DatabaseWorker finishing..." << queueSize();
    wait();
}
void DatabaseWorker::run()
{
    while (!isInterruptionRequested()) {
        //wait for requests and update info every 1 sec
        if (queueSize() == 0) {
            queueMutex.lockForRead();
            waitCondition.wait(&queueMutex, 1000);
            bool empty = queue.empty();
            queueMutex.unlock();
            if (empty) {
                infoUpdate(true);
                continue;
            }
        }
        //don't check interruption and wait condition every iteration if we have a batch of requests
        int size = queueSize();
        for (int i = 0; i < size; i++) {
            DatabaseRequest *req = queue[i];
            rcnt++;
            infoUpdate(false);
            if (req->discarded()) {
                req->finish(false);
                if (!req->isSynchronous)
                    req->deleteLater();
                continue;
            }
            //process request
            QSqlQuery query(db->sql);
            query.setForwardOnly(true);
            if (!db->inTransaction)
                db->transaction(query); //begin if not already
            bool rv = req->run(query);
            if (!rv) {
                apxConsoleW() << "query error:" << query.lastError().text() << query.lastQuery()
                              << req;
                db->commit(query, true);
            } else if (req->discarded()) {
                db->rollback(query);
            }
            req->finish(!rv);
            if (!req->isSynchronous)
                req->deleteLater();
            if (i == size - 1 || size % 200 == 0)
                db->commit(query);
        }
        eraseRequests(size);
        if (queueSize() == 0)
            emit empty();
    }
    QWriteLocker locker(&queueMutex);
    for (auto req : queue) {
        req->discard();
        req->finish(false);
        if (!req->isSynchronous)
            req->deleteLater();
    }
    queue.clear();
}
//=============================================================================
void DatabaseWorker::request(DatabaseRequest *req)
{
    if (req && !isInterruptionRequested()) {
        enqueue(req);
        //qDebug()<<sz;
        waitCondition.wakeAll();
        //wait for long queues
        while (queueSize() >= 100000) {
            usleep(1000);
        }
    }
}
//=============================================================================
int DatabaseWorker::queueSize()
{
    QReadLocker lock(&queueMutex);
    return queue.size();
}
int DatabaseWorker::rate()
{
    return m_rate;
}
//=============================================================================
bool DatabaseWorker::infoUpdate(bool force)
{
    int dt = infoUpdateTime.elapsed();
    if (force == false && dt < 1000)
        return false;
    infoUpdateTime.start();

    //calc rate
    double vr = rcnt / (dt / 1000.0);
    rcnt = 0;
    m_rate = std::round(vr);

    emit infoChanged();
    return true;
}
//=============================================================================
void DatabaseWorker::enqueue(DatabaseRequest *req)
{
    QWriteLocker lock(&queueMutex);
    queue.push_back(req);
}
//=============================================================================
void DatabaseWorker::eraseRequests(int count)
{
    QWriteLocker lock(&queueMutex);
    queue.erase(queue.begin(), queue.begin() + count);
}
