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
#ifndef DatalinkInspector_H
#define DatalinkInspector_H
//=============================================================================
#include "DatalinkInspectorListModel.h"
#include <Fact/Fact.h>
#include <QtCore>
//=============================================================================
class DatalinkInspector : public Fact
{
    Q_OBJECT
    Q_PROPERTY(DatalinkInspectorListModel *outModel READ outModel CONSTANT)

public:
    explicit DatalinkInspector(Fact *parent = nullptr);

    void handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &message);

    Q_INVOKABLE void exec(const QString &cmd);

    Q_INVOKABLE void historyReset();
    Q_INVOKABLE QString historyNext(const QString &cmd);
    Q_INVOKABLE QString historyPrev(const QString &cmd);

    Q_INVOKABLE QString autocomplete(const QString &cmd);

private:
    DatalinkInspectorListModel *_model;
    QStringList _history;
    int _historyIndex;
    QString _replacedHistory;

public:
    //---------------------------------------
    DatalinkInspectorListModel *outModel() const;

signals:
    void newMessage(QtMsgType type, QString category, QString text);
};
//=============================================================================
#endif
