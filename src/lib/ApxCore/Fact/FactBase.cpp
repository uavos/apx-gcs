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
#include "FactBase.h"
#include "Fact.h"
#include "FactPropertyBinding.h"

FactBase::FactBase(QObject *parent, const QString &name, FactBase::Flags flags)
    : QObject(parent)
{
    setName(name);
    setTreeType(Flag(uint(flags) & TypeMask));
    setOptions(flags & OptsMask);
}
FactBase::~FactBase()
{
    //qDebug() << path() << parent();
    setParentFact(nullptr);
    for (auto i : m_actions) {
        i->deleteFact();
    }
    for (auto i : m_facts) {
        i->deleteFact();
    }
}

void FactBase::bindProperty(Fact *src, QString name, bool oneway)
{
    unbindProperties(src, name);

    if (src == this) {
        qWarning() << "recursive bind:" << path() << name;
        return;
    }

    // find two-way binds to filter loops
    FactPropertyBinding *src_binding = nullptr;
    for (auto i : src->_property_binds) {
        if (!i->match(static_cast<Fact *>(this), name))
            continue;
        src_binding = i;
        break;
    }

    FactPropertyBinding *b = new FactPropertyBinding(static_cast<Fact *>(this),
                                                     src,
                                                     name,
                                                     src_binding);
    _property_binds.append(b);

    if (oneway)
        return;

    src->bindProperty(static_cast<Fact *>(this), name, true);
}
void FactBase::unbindProperty(QString name)
{
    unbindProperties(nullptr, name);
}
void FactBase::unbindProperties(Fact *src, const QString &name)
{
    foreach (auto i, _property_binds) {
        if (!i->match(src, name))
            continue;
        _property_binds.removeOne(i);
        delete i;
    }
}
bool FactBase::bindedProperty(Fact *src, QString name)
{
    for (auto i : _property_binds) {
        if (i->match(src, name))
            return true;
    }
    return false;
}

//=============================================================================
const FactList &FactBase::facts() const
{
    return m_facts;
}
const FactList &FactBase::actions() const
{
    return m_actions;
}
//=============================================================================
void FactBase::addChild(Fact *item)
{
    //item->Qobject::setParent(this);
    if (item->treeType() == Action) {
        if (!m_actions.contains(item)) {
            m_actions.append(item);
            emit actionsUpdated();
        }
        return;
    }
    if (m_facts.contains(item))
        return;
    if (property(item->name().toUtf8()).isValid()) {
        qWarning() << "Property override:" << path() << item->name();
    }
    emit itemToBeInserted(m_facts.count(), item);
    m_facts.append(item);
    updateChildrenNums();
    updateSize();
    emit itemInserted(item);
}
void FactBase::removeChild(Fact *item)
{
    int i = m_actions.indexOf(item);
    if (i >= 0) {
        m_actions.removeAll(item);
        emit actionsUpdated();
    }
    i = m_facts.indexOf(item);
    if (i < 0)
        return;
    emit itemToBeRemoved(i, item);
    m_facts.removeAt(i);
    updateChildrenNums();
    updateSize();
    emit itemRemoved(item);
}
void FactBase::moveChild(Fact *item, int n, bool safeMode)
{
    int i = m_facts.indexOf(item);
    if (i < 0 || i == n)
        return;
    if (safeMode) {
        emit itemToBeRemoved(i, item);
        m_facts.removeAt(i);
        emit itemRemoved(item);
        emit itemToBeInserted(n, item);
        m_facts.insert(n, item);
        emit itemInserted(item);
        updateChildrenNums();
    } else {
        if (n > i)
            n++;
        emit itemToBeMoved(i, n, item);
        m_facts.removeAt(i);
        m_facts.insert(n, item);
        emit itemMoved(item);
    }
    for (int i = 0; i < m_facts.count(); ++i)
        child(i)->updateNum();
}
//=============================================================================
void FactBase::deleteFact()
{
    setParentFact(nullptr);
    emit removed();
    deleteLater();
}
void FactBase::deleteChildren()
{
    if (m_facts.count() <= 0)
        return;
    for (int i = 0; i < m_facts.count(); i++) {
        FactBase *item = child(i);
        disconnect(this, nullptr, item, nullptr);
        item->deleteChildren();
    }
    //qDebug()<<"removeAll"<<this;
    while (m_facts.count() > 0) {
        FactBase *item = child(m_facts.count() - 1);
        disconnect(this, nullptr, item, nullptr);
        emit itemToBeRemoved(m_facts.count() - 1, item);
        m_facts.takeLast();
        m_size = m_facts.count();
        item->removed();
        emit itemRemoved(item);
        item->deleteLater();
    }
    emit sizeChanged();
}
void FactBase::move(int n, bool safeMode)
{
    FactBase *p = parentFact();
    if (p)
        p->moveChild(static_cast<Fact *>(this), n, safeMode);
}
//=============================================================================
int FactBase::num() const
{
    return m_num;
}
//=============================================================================
Fact *FactBase::child(int n) const
{
    return m_facts.value(n, nullptr);
}
//=============================================================================
int FactBase::indexOfChild(Fact *item) const
{
    return m_facts.indexOf(item);
}
int FactBase::indexInParent() const
{
    return parentFact()
               ? parentFact()->indexOfChild(static_cast<Fact *>(const_cast<FactBase *>(this)))
               : -1;
}
//=============================================================================
Fact *FactBase::child(const QString &name, Qt::CaseSensitivity cs) const
{
    for (auto i : m_facts) {
        if (i->name().compare(name, cs) == 0)
            return i;
    }
    return nullptr;
}
//=============================================================================
QStringList FactBase::pathStringList(int maxLevel) const
{
    QStringList st;
    for (const FactBase *i = this; i; i = i->parentFact()) {
        st.insert(0, i->name());
        if (i->treeType() == Root)
            break;
        if (maxLevel-- == 0)
            break;
    }
    return st;
}
QString FactBase::path(int maxLevel, const QChar pathDelimiter) const
{
    return pathStringList(maxLevel).join(pathDelimiter);
}
FactList FactBase::pathList() const
{
    FactList list;
    for (const FactBase *i = this; i;) {
        list.append(static_cast<Fact *>(const_cast<FactBase *>(i)));
        if (i->treeType() == Root)
            break;
        i = i->parentFact();
    }
    return list;
}
QString FactBase::path(const FactBase *root, const QChar pathDelimiter) const
{
    int level = -1;
    for (const FactBase *i = this; i && i != root; i = i->parentFact()) {
        level++;
    }
    return path(level, pathDelimiter);
}
QString FactBase::jsname() const
{
    QString s = name();
    if (s.contains('#')) {
        // array properties are accessible through fact's 'model'
        return "#";
    }
    s = s.simplified()
            .trimmed()
            .replace(' ', '_')
            .replace('.', '_')
            .replace(':', '_')
            .replace('/', '_')
            .replace('\\', '_')
            .replace('?', '_')
            .replace('-', '_')
            .replace('+', '_');

    // check for reserved names
    const char *r = nullptr, *n = s.toUtf8();
    if (metaObject()->indexOfProperty(n) >= 0) {
        r = "property";
    } else if (metaObject()->indexOfMethod(n) >= 0) {
        r = "method";
    } else if (metaObject()->indexOfSignal(n) >= 0) {
        r = "signal";
    } else if (metaObject()->indexOfSlot(n) >= 0) {
        r = "slot";
    }
    if (r) {
        qWarning() << QString("Name is reserved for %1: %2 (%3)").arg(r).arg(s).arg(path());
        // just a warning by now
    }
    return s;
}
QString FactBase::jspath() const
{
    QStringList st;
    for (const FactBase *i = this; i; i = i->parentFact()) {
        st.insert(0, i->jsname());
        if (i->treeType() == Root)
            break;
    }
    return st.join('.');
}
//=============================================================================
void FactBase::updateNum()
{
    int v = indexInParent();
    if (v < 0)
        return;
    if (m_num == v)
        return;
    m_num = v;
    emit numChanged();
}
void FactBase::updateSize()
{
    int v = m_facts.size();
    if (m_size == v)
        return;
    m_size = v;
    emit sizeChanged();
}
void FactBase::updateChildrenNums()
{
    for (auto i : m_facts)
        i->updateNum();
}
//=============================================================================
FactBase::Flag FactBase::treeType(void) const
{
    return m_treeType;
}
void FactBase::setTreeType(FactBase::Flag v)
{
    v = static_cast<Flag>(v & TypeMask);
    if (m_treeType == v)
        return;
    m_treeType = v;
    emit treeTypeChanged();
    if (v == Action && parentFact()) {
        parentFact()->removeChild(static_cast<Fact *>(this));
        parentFact()->addChild(static_cast<Fact *>(this));
    }
}
FactBase::Flags FactBase::options(void) const
{
    return m_options;
}
void FactBase::setOptions(FactBase::Flags v)
{
    v &= OptsMask;
    if (m_options == v)
        return;
    m_options = v;
    emit optionsChanged();
    if (m_options & Section)
        setTreeType(Group);
}
void FactBase::setOption(FactBase::Flag opt, bool v)
{
    if (v)
        setOptions(options() | opt);
    else
        setOptions(options() & (~opt));
}
int FactBase::size(void) const
{
    return m_size;
}
QString FactBase::name(void) const
{
    return objectName();
}
void FactBase::setName(QString s)
{
    s = s.trimmed();
    if (name() == s)
        return;

    setObjectName(s);
    emit nameChanged();
}
Fact *FactBase::parentFact() const
{
    return qobject_cast<Fact *>(m_parentFact);
}
void FactBase::setParentFact(Fact *v)
{
    FactBase *prevParent = parentFact();
    if (prevParent == v)
        return;
    if (prevParent) {
        prevParent->removeChild(static_cast<Fact *>(this));
    }

    //QObject::setParent(v);
    m_parentFact = v;
    emit parentFactChanged();
    if (!v)
        return;
    v->addChild(static_cast<Fact *>(this));
    updatePath();
}
void FactBase::updatePath()
{
    for (auto f : m_facts) {
        f->updatePath();
    }
    emit pathChanged();
}
