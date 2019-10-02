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
#ifndef TerminalListModel_H
#define TerminalListModel_H
//=============================================================================
#include <ApxApp.h>
#include <QtCore>
//=============================================================================
class TerminalListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    explicit TerminalListModel(QObject *parent = nullptr);
    ~TerminalListModel();

    enum TerminalListModelRoles {
        ModelDataRole = Qt::UserRole + 1,
        TextRole,
        SubsystemRole,

        SourceRole,
        TypeRole,
        OptionsRole,

        FactRole,

        TimestampRole,
    };
    QHash<int, QByteArray> roleNames() const;

private:
    struct TerminalListItem
    {
        qint64 timestamp;
        QString text;
        QString subsystem;
        ApxApp::NotifyFlags flags;
        QPointer<Fact> fact;
    };
    QList<TerminalListItem *> _items;
    int _enterIndex;

protected:
    //ListModel override
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

private slots:
    void notification(QString msg, QString subsystem, ApxApp::NotifyFlags flags, Fact *fact);

public slots:
    void enter(const QString &line);
    void enterResult(bool ok);

    //-----------------------------------------
    //PROPERTIES
public:
protected:
signals:
    void countChanged();
};
//=============================================================================
#endif
