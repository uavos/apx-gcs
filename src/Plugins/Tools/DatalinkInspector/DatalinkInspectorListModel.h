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
#ifndef DatalinkInspectorListModel_H
#define DatalinkInspectorListModel_H
//=============================================================================
#include <QtCore>
class Fact;
class FactBase;
//=============================================================================
class DatalinkInspectorListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_ENUMS(DatalinkInspectorLineType)
    Q_ENUMS(QtMsgType)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    explicit DatalinkInspectorListModel(QObject *parent = nullptr);
    ~DatalinkInspectorListModel();

    enum DatalinkInspectorListModelRoles {
        ModelDataRole = Qt::UserRole + 1,
        TextRole,
        CategoryRole,
        TypeRole,
        TimestampRole,
    };
    QHash<int, QByteArray> roleNames() const;

    enum DatalinkInspectorLineType {
        InputLine,
        InfoGrayLine,
        InfoVehicleLine,
        ErrorLine,
    };
    Q_ENUM(DatalinkInspectorLineType)

private:
    struct DatalinkInspectorListItem
    {
        QtMsgType type;
        QString category;
        QString text;
        quint64 timestamp;
    };
    QList<DatalinkInspectorListItem *> _items;
    int _enterIndex;

protected:
    //ListModel override
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

public slots:
    void append(QtMsgType type, QString category, QString text);
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
