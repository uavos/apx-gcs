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
#include "JSTreeModel.h"
#include <Fact/Fact.h>
#include <QColor>
#include <QFontDatabase>
#include <QJSValueIterator>
//=============================================================================
JSTreeModel::JSTreeModel(QJSEngine *e)
    : QAbstractItemModel(e)
    , e(e)
{
    setObjectName("jsTreeModel");
    root = new JSTreeItem(nullptr, "root", e->globalObject());
    root->setParent(this);
}
//=============================================================================
QHash<int, QByteArray> JSTreeModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ModelDataRole] = "modelData";
    roles[NameRole] = "name";
    roles[ValueRole] = "value";
    roles[DescrRole] = "descr";
    roles[TextRole] = "text";
    return roles;
}
//=============================================================================
JSTreeItem *JSTreeModel::item(const QModelIndex &index) const
{
    return qobject_cast<JSTreeItem *>(static_cast<QObject *>(index.internalPointer()));
}
QModelIndex JSTreeModel::itemIndex(JSTreeItem *item, int column) const
{
    if (item == root)
        return QModelIndex();
    return createIndex(item->num(), column, item);
}
//=============================================================================
//=============================================================================
//=============================================================================
QVariant JSTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    JSTreeItem *i = item(index);
    if (!i)
        return QVariant();
    QJSValue value = i->value();
    int col = index.column();
    switch (role) {
    case ModelDataRole:
        return QVariant::fromValue(const_cast<JSTreeItem *>(i));
    case NameRole:
        return i->name;
    case ValueRole:
        return value.toVariant();
    case DescrRole:
        return i->descr;
    case TextRole:
        return value.toString();

    case Qt::ForegroundRole:
        if (col == JS_MODEL_COLUMN_NAME) {
            if (i->isFunc) {
                if (i->isCmd)
                    return QColor(Qt::yellow).lighter();
                return QColor(Qt::red).lighter();
            }
            if (i->isFact)
                return QColor(Qt::green).lighter();
            if (i->isQObj)
                return QColor(Qt::yellow).lighter();
            if (i->isMap)
                return QColor(Qt::cyan).lighter();
            return QColor(Qt::gray).lighter(); //QVariant();
        }
        if (col == JS_MODEL_COLUMN_VALUE) {
            //if(!enabled())return QColor(Qt::darkGray);
            //if(size()) return QColor(Qt::darkGray); //expandable
            //if(modified())return QColor(Qt::yellow);
            //if(isZero())return QColor(Qt::gray);
            return QColor(Qt::cyan).lighter(180);
        }
        return QColor(Qt::darkCyan);
    case Qt::BackgroundRole:
        return QVariant();
    case Qt::FontRole: {
        QFont font(QFontDatabase::systemFont(QFontDatabase::GeneralFont)); //qApp->font());
        if (col == JS_MODEL_COLUMN_DESCR)
            return QVariant();
        if (value.isQObject() && col == JS_MODEL_COLUMN_NAME)
            font.setBold(true);
        //if(ftype>=ft_regPID) return QFont("Monospace",-1,column==tc_field?QFont::Bold:QFont::Normal);
        //if(col==FACT_MODEL_COLUMN_NAME) return QFont("Monospace",-1,QFont::Normal,isModified());
        //if(ftype==ft_string) return QFont("",-1,QFont::Normal,true);
        return font;
    }
    }

    //value roles
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    switch (index.column()) {
    case JS_MODEL_COLUMN_NAME:
        return data(index, NameRole);
    case JS_MODEL_COLUMN_VALUE:
        return data(index, TextRole);
    case JS_MODEL_COLUMN_DESCR:
        return data(index, DescrRole);
    }
    return QVariant();
}
//=============================================================================
bool JSTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if ((!index.isValid()) || (role != Qt::EditRole) || index.column() != JS_MODEL_COLUMN_VALUE)
        return false;
    JSTreeItem *f = item(index);
    if (!f)
        return false;
    if (data(index, role) == value)
        return true;
    bool rv = f->setValue(e->toScriptValue(value));
    //updateTimerTimeout();
    //if(rv)emit dataChanged(index,index);//layoutChanged();
    return rv;
}
//=============================================================================
//=============================================================================
QModelIndex JSTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();
    JSTreeItem *parentTreeItem;
    if (!parent.isValid())
        parentTreeItem = root;
    else
        parentTreeItem = item(parent);
    if (!parentTreeItem)
        return QModelIndex();
    JSTreeItem *childTreeItem = parentTreeItem->child(row);
    if (!childTreeItem)
        return QModelIndex();
    return itemIndex(childTreeItem, column);
}
//=============================================================================
QModelIndex JSTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();
    JSTreeItem *i = item(index);
    if (!i)
        return QModelIndex();
    //checkConnections(i);
    if (!i->parentItem)
        return QModelIndex();
    JSTreeItem *parentTreeItem = i->parentItem;
    if (!parentTreeItem || parentTreeItem == root)
        return QModelIndex();
    return itemIndex(parentTreeItem);
}
//=============================================================================
int JSTreeModel::rowCount(const QModelIndex &parent) const
{
    JSTreeItem *parentTreeItem;
    if (parent.column() > 0)
        return 0;
    if (!parent.isValid())
        parentTreeItem = root;
    else
        parentTreeItem = item(parent);
    return parentTreeItem->size();
}
//=============================================================================
int JSTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return JS_MODEL_COLUMN_CNT;
}
//=============================================================================
QVariant JSTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation)
    if (role == Qt::DisplayRole) {
        switch (section) {
        case JS_MODEL_COLUMN_NAME:
            return tr("Name");
        case JS_MODEL_COLUMN_VALUE:
            return tr("Value");
        case JS_MODEL_COLUMN_DESCR:
            return tr("Description");
        }
    }
    return QVariant();
}
//=============================================================================
Qt::ItemFlags JSTreeModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags fx = Qt::NoItemFlags;
    JSTreeItem *i = item(index);
    if (!i)
        return fx;
    fx |= Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.column() != JS_MODEL_COLUMN_VALUE)
        return fx;
    if (i->isFact || i->isFunc || i->isMap || i->isQObj)
        return fx;
    return fx | Qt::ItemIsEditable;
}
//=============================================================================
//=============================================================================
//=============================================================================
JSTreeItem::JSTreeItem(JSTreeItem *parent, const QString &name, const QJSValue &value)
    : QObject(parent)
    , parentItem(parent)
    , name(name)
    , descr(value.toVariant().typeName())
    , isFact(value.toVariant().value<Fact *>())
    , isQObj(value.isQObject())
    , isFunc(value.isCallable())
    , isMap(QString(value.toVariant().typeName()) == "QVariantMap")
    , isCmd(value.hasProperty("info"))
    , m_value(value)
    , m_size(-1)
{
    if (isFact) {
        Fact *f = value.toVariant().value<Fact *>();
        descr = "Fact: " + f->title();
        if (!f->descr().isEmpty())
            descr.append(" - " + f->descr());
    } else {
        if (isFunc)
            descr = "func";
        if (value.hasProperty("info")) {
            if (!descr.isEmpty())
                descr.append(": ");
            descr.append(value.property("info").toString());
        }
    }
}
//=============================================================================
QJSValue JSTreeItem::value() const
{
    if (!parentItem)
        return m_value;
    return parentItem->value().property(name);
}
bool JSTreeItem::setValue(const QJSValue &v)
{
    if (!parentItem)
        return false;
    parentItem->value().setProperty(name, v);
    return value().equals(v);
}
int JSTreeItem::num() const
{
    if (!parentItem)
        return 0;
    int i = parentItem->items.indexOf(const_cast<JSTreeItem *>(this));
    if (i < 0)
        const_cast<JSTreeItem *>(this)->updateSize();
    return parentItem->items.indexOf(const_cast<JSTreeItem *>(this));
}
int JSTreeItem::size() const
{
    if (m_size < 0)
        const_cast<JSTreeItem *>(this)->updateSize();
    return m_size;
}
JSTreeItem *JSTreeItem::child(int i)
{
    if (size() != items.size())
        sync();
    return items.value(i, nullptr);
}
QList<JSTreeItem *> JSTreeItem::pathList(bool includeThis) const
{
    QList<JSTreeItem *> list;
    for (JSTreeItem *i = const_cast<JSTreeItem *>(this); i; i = i->parentItem) {
        if (includeThis == false && i == this)
            continue;
        list.prepend(i);
    }
    return list;
}
QString JSTreeItem::path(const QString &sep)
{
    if (!m_path.isEmpty())
        return m_path;
    QStringList st;
    foreach (JSTreeItem *i, pathList(true)) {
        st.append(i->name);
    }
    st.removeFirst();
    m_path = st.join(sep);
    return m_path;
}
void JSTreeItem::sync()
{
    //qDebug()<<"sync"<<name;
    qDeleteAll(items);
    items.clear();
    //filter childs for some items
    //check recursive
    QJSValue v = value();
    foreach (JSTreeItem *i, pathList(false)) {
        if (i->value().strictlyEquals(v)) {
            descr = "recursive reference";
            qDebug() << path() << descr;
            return;
        }
    }
    //populate items
    QJSValueIterator it(v);
    while (it.hasNext()) {
        it.next();
        JSTreeItem *i = new JSTreeItem(this, it.name(), it.value());
        items.append(i);
    }
    std::sort(items.begin(), items.end(), lessThan);
}
void JSTreeItem::updateSize()
{
    sync();
    int cnt = items.size();
    if (m_size == cnt)
        return;
    m_size = cnt;
    sync();
}
bool JSTreeItem::lessThan(const JSTreeItem *i1, const JSTreeItem *i2)
{
    while (1) {
        if (i1->isFact) {
            if (i2->isFact)
                break;
            return true;
        }
        if (i2->isFact)
            return false;

        if (i1->isQObj) {
            if (i2->isQObj)
                break;
            return true;
        }
        if (i2->isQObj)
            return false;

        if (i1->isFunc) {
            if (i2->isFunc)
                break;
            return false;
        }
        if (i2->isFunc)
            return true;

        if (i1->isMap) {
            if (i2->isMap)
                break;
            return true;
        }
        if (i2->isMap)
            return false;

        break;
    }
    return i1->name.toLower().localeAwareCompare(i2->name.toLower()) < 0;
}
bool JSTreeItem::showThis(const QRegExp &regexp)
{
    if (name == "prototype")
        return false;
    if (name == "parent")
        return false;
    if (name == "children")
        return false;
    if (name == "visibleChildren")
        return false;
    if (name == "contentItem")
        return false;
    if (name == "contentData")
        return false;
    if (name == "data")
        return false;
    return path().contains(regexp);
}
//=============================================================================
