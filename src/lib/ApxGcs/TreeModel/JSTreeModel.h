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
#pragma once

#include <QJSEngine>
#include <QtCore>

class JSTreeItem : public QObject
{
    Q_OBJECT
public:
    explicit JSTreeItem(JSTreeItem *parent, const QString &name, const QJSValue &value);

    QPointer<JSTreeItem> parentItem;

    QString name;
    QString descr;

    bool isFact;
    bool isQObj;
    bool isFunc;
    bool isMap;
    bool isCmd;

    QJSValue value() const;
    bool setValue(const QJSValue &v);

    int num() const;
    int size() const;
    JSTreeItem *child(int i);
    QList<JSTreeItem *> pathList(bool includeThis = true) const;
    QString path(const QString &sep = QString('.'));

    QList<JSTreeItem *> items;
    void sync();

    static bool lessThan(const JSTreeItem *i1, const JSTreeItem *i2);
    bool showThis(const QRegularExpression &regexp);

private:
    QString m_path;
    QJSValue m_value;
    int m_size;
    void updateSize();
};

class JSTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit JSTreeModel(QJSEngine *e);

    QHash<int, QByteArray> roleNames() const;

    //data model
    enum {
        JS_MODEL_COLUMN_NAME = 0,
        JS_MODEL_COLUMN_VALUE,
        JS_MODEL_COLUMN_DESCR,

        JS_MODEL_COLUMN_CNT,
    };
    enum FactModelRoles {
        ModelDataRole = Qt::UserRole + 1,
        NameRole,
        ValueRole,
        DescrRole,
        TextRole,
    };

    JSTreeItem *root;

    JSTreeItem *item(const QModelIndex &index) const;
    QModelIndex itemIndex(JSTreeItem *item, int column = 0) const;

private:
    QJSEngine *e;

protected:
    //override
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
};
