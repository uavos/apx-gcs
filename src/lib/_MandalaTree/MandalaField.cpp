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
#include "MandalaTree.h"
using namespace Mandala;
//=============================================================================
//root mandala
MandalaTree::MandalaTree(QObject *parent)
    : QAbstractListModel(parent)
    , m_parentItem(NULL)
    , m_bindItem(NULL)
    , m_id(0)
    , m_level(0)
    , m_name("m")
    , m_descr("Mandala")
{
    setObjectName(m_name);
}
//=============================================================================
//group or class parent of field
MandalaTree::MandalaTree(MandalaTree *parent, id_t id, QString name, QString descr, QString alias)
    : QAbstractListModel(parent)
    , m_parentItem(parent)
    , m_bindItem(NULL)
    , m_id(id)
    , m_level(0)
    , m_name(name)
    , m_descr(descr)
    , m_alias(alias)
{
    setObjectName(m_name);
    parent->addItem(this);

    //find tree item type
    for (MandalaTree *item = m_parentItem; item; item = item->m_parentItem)
        m_level++;
    if (maxLevel < m_level)
        maxLevel = m_level;

    emit structChanged(this);
}
//=============================================================================
int MandalaTree::maxLevel = 0;
MandalaTree::~MandalaTree()
{
    if (m_parentItem) {
        m_parentItem->m_items.removeAll(this);
    }
}
//=============================================================================
void MandalaTree::addItem(MandalaTree *item)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_items << item;
    item->m_parentItem = this;
    connect(item, &MandalaTree::valueChanged, this, &MandalaTree::valueChanged);
    connect(item, &MandalaTree::structChanged, this, &MandalaTree::structChanged);
    setProperty(item->name().toUtf8().data(), qVariantFromValue(item));
    endInsertRows();
    emit sizeChanged(this);
}
void MandalaTree::removeItem(MandalaTree *item)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_items << item;
    item->m_parentItem = this;
    connect(item, &MandalaTree::valueChanged, this, &MandalaTree::valueChanged);
    connect(item, &MandalaTree::structChanged, this, &MandalaTree::structChanged);
    setProperty(item->name().toUtf8().data(), qVariantFromValue(item));
    endInsertRows();
    emit sizeChanged(this);
}
//=============================================================================
void MandalaTree::bind(MandalaTree *v)
{
    if (m_bindItem == v)
        return;
    if (m_bindItem)
        disconnect(m_bindItem, 0, this, 0);
    m_bindItem = v;

    //map signals
    connect(m_bindItem, &MandalaTree::structChanged, this, &MandalaTree::structChanged);
    emit structChanged(v);

    connect(m_bindItem, &MandalaTree::valueChanged, this, &MandalaTree::valueChanged);
    emit valueChanged(v);

    //bind fields
    foreach (MandalaTree *f, fields()) {
        MandalaTree *mf = v->fieldByPath(f->path(2));
        if (!mf)
            continue;
        f->bind(mf);
    }
}
//=============================================================================
void MandalaTree::clear(void)
{
    foreach (MandalaTree *i, m_items) {
        i->clear();
    }
    qDeleteAll(m_items);
    m_items.clear();
}
//=============================================================================
void MandalaTree::reset(void)
{
    foreach (MandalaTree *i, m_items) {
        i->reset();
    }
    setValue(QVariant());
}
//=============================================================================
QVariant MandalaTree::value(void) const
{
    return m_value;
}
//=============================================================================
bool MandalaTree::setValue(const QVariant &v)
{
    if (m_value == v)
        return false;
    m_value = v;
    emit valueChanged(this);
    return true;
}
//=============================================================================
MandalaTree *MandalaTree::child(int n)
{
    if (n >= m_items.size())
        return NULL;
    return m_items.at(n);
}
//=============================================================================
MandalaTree *MandalaTree::parentItem() const
{
    return m_parentItem;
}
//=============================================================================
int MandalaTree::num() const
{
    if (!m_parentItem)
        return 0;
    return m_parentItem->m_items.indexOf(const_cast<MandalaTree *>(this));
}
//=============================================================================
//=============================================================================
QString MandalaTree::name(void) const
{
    return m_bindItem ? m_bindItem->name() : m_name;
}
QString MandalaTree::descr(void) const
{
    return m_bindItem ? m_bindItem->descr() : m_descr;
}
QString MandalaTree::alias(void) const
{
    return m_bindItem ? m_bindItem->alias() : m_alias;
}
quint16 MandalaTree::id(void) const
{
    return m_bindItem ? m_bindItem->id() : m_id;
}
int MandalaTree::level(void) const
{
    return m_level;
}
bool MandalaTree::used(void) const
{
    foreach (MandalaTree *item, m_items) {
        if (item->used())
            return true;
    }
    return false;
}
QString MandalaTree::valueText(void) const
{
    return value().toString();
}
int MandalaTree::size() const
{
    return m_items.size();
}
//=============================================================================
bool MandalaTree::isField(void) const
{
    return m_items.isEmpty();
}
bool MandalaTree::isFieldsGroup(void) const
{
    return (!m_items.isEmpty()) && m_items.first()->isField();
}
bool MandalaTree::isRoot(void) const
{
    return (!m_parentItem);
}
//=============================================================================
MandalaTree *MandalaTree::at(const QString &s)
{
    foreach (MandalaTree *item, m_items) {
        if (item->name() == s)
            return item;
    }
    return this;
}
MandalaTree *MandalaTree::at(id_t id)
{
    foreach (MandalaTree *item, m_items) {
        if (item->id() == id)
            return item;
    }
    if (id < m_items.count())
        return m_items.at(id);
    return this;
}
MandalaTree *MandalaTree::find(QString s)
{
    MandalaTree *item = this;
    while (!s.isEmpty()) {
        if (item->isFieldsGroup()) {
            item = item->at(s);
            break;
        } else {
            int i = s.indexOf('.');
            if (i <= 0)
                break;
            item = item->at(s.left(i));
            s.remove(0, i + 1);
        }
    }
    return item;
}
MandalaTree *MandalaTree::find(id_t id)
{
    MandalaTree *item = this;
    id_t i = id;
    switch (maxLevel - m_level) {
    case 3:
        i = id & ~MANDALA_INDEX_MASK_GRP1;
        break; //item is vehicle
    case 2:
        i = id & ~MANDALA_INDEX_MASK_FIELDS;
        break; //item is topmost group (class)
    default:
        break;
    }
    item = at(i);
    if (item == this)
        return this;
    return item->find(id);
}
MandalaTree *MandalaTree::fieldByAlias(QString s)
{
    foreach (MandalaTree *i, m_items) {
        if (i->isField()) {
            MandalaTree *f = i;
            if (f->alias() == s)
                return f;
        } else {
            MandalaTree *f = i->fieldByAlias(s);
            if (f)
                return f;
        }
    }
    return NULL;
}
MandalaTree *MandalaTree::fieldByPath(QString s)
{
    MandalaTree *f = find(s);
    if (f->isField())
        return static_cast<MandalaTree *>(f);
    return NULL;
}
MandalaTree *MandalaTree::field(QString s)
{
    MandalaTree *f = fieldByPath(s);
    if (f)
        return f;
    return fieldByAlias(s);
}
QString MandalaTree::path(int lev) const
{
    QString s = name();
    for (const MandalaTree *i = parentItem(); i && i->level() >= lev; i = i->parentItem()) {
        s.prepend(i->name() + ".");
    }
    return s;
}
QList<MandalaTree *> MandalaTree::fields()
{
    QList<MandalaTree *> list;
    foreach (MandalaTree *i, m_items) {
        if (i->isField()) {
            list.append(static_cast<MandalaTree *>(i));
        } else {
            list.append(i->fields());
        }
    }
    return list;
}
//=============================================================================
// LIST MODEL
//=============================================================================
int MandalaTree::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return size();
}
QHash<int, QByteArray> MandalaTree::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ItemRole] = "item";
    roles[NameRole] = "name";
    roles[ValueRole] = "value";
    roles[DescrRole] = "descr";
    return roles;
}
QVariant MandalaTree::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation)
    if (role == Qt::DisplayRole) {
        switch (section) {
        case MANDALA_ITEM_COLUMN_NAME:
            return tr("Name");
        case MANDALA_ITEM_COLUMN_VALUE:
            return tr("Value");
        case MANDALA_ITEM_COLUMN_DESCR:
            return tr("Description");
        }
    }
    return QVariant();
}
Qt::ItemFlags MandalaTree::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (isField()) {
        f |= Qt::ItemNeverHasChildren;
        if (index.column() == MANDALA_ITEM_COLUMN_VALUE)
            f |= Qt::ItemIsEditable;
    }
    return f;
}
QVariant MandalaTree::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_items.count())
        return QVariant();
    MandalaTree *item = m_items[index.row()];
    switch (role) {
    case ItemRole:
        return QVariant::fromValue(item);
    case NameRole:
        return item->name();
    case ValueRole:
        return item->value();
    case DescrRole:
        return item->descr();
    }
    return QVariant();
}
bool MandalaTree::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.row() < 0 || index.row() >= m_items.count() || role != Qt::EditRole)
        return false;
    m_items[index.row()]->setValue(value);
    return true;
}
//=============================================================================
//=============================================================================
