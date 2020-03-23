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
#ifndef DatabaseLookupModel_H
#define DatabaseLookupModel_H
//=============================================================================
#include <QtCore>
//=============================================================================
class DatabaseLookupModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
    explicit DatabaseLookupModel(QObject *parent = nullptr);

    bool ordered;
    bool qmlMapSafe;

    typedef QList<QVariantMap> ItemsList;

    virtual QHash<int, QByteArray> roleNames() const;
    enum ItemRoles {
        ValuesRole = Qt::UserRole + 1,
    };

    int indexOf(const QString &name, const QVariant &value) const;

protected:
    ItemsList _items;
    QList<quint64> keys;

    //ListModel override
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

public:
    Q_INVOKABLE QVariantMap get(int i) const;
    Q_INVOKABLE bool set(int i, QVariantMap v);

public slots:
    void syncItems(ItemsList list);
    void resetFilter();

signals:
    void itemEdited(int i, QVariantMap v);
    void itemRemoved(int i, QVariantMap v);
    void synced();

    //-----------------------------------------
    //PROPERTIES
public:
    int count() const;
    QString filter() const;
    void setFilter(QString v);

private:
    QString m_filter;
signals:
    void countChanged();
    void filterChanged();
};
//=============================================================================
#endif
