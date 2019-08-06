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
#include "Fact.h"
#include <App/AppRoot.h>
#include <ApxLog.h>
#include <QFont>
#include <QFontDatabase>
//=============================================================================
Fact::Fact(
    FactBase *parent, const QString &name, const QString &title, const QString &descr, Flags flags)
    : FactData(parent, name, title, descr, flags)
    , bindedFact(nullptr)
    , blockNotify(false)
    , m_model(nullptr)
    , m_actionsModel(nullptr)
    , m_enabled(true)
    , m_visible(true)
    , m_active(false)
    , m_progress(-1)
    , m_link(nullptr)
    , m_parentEnabled(true)
    , m_parentVisible(true)
{
    //models
    m_model = new FactListModel(this);
    m_actionsModel = new FactListModelActions(this);

    connect(this, &Fact::parentFactChanged, this, &Fact::updateParentEnabled);
    updateParentEnabled();
    connect(this, &Fact::parentFactChanged, this, &Fact::updateParentVisible);
    updateParentVisible();

    //default icons
    if (m_icon.isEmpty()) {
        if (options() & Apply)
            m_icon = "check";
        else if (options() & Remove)
            m_icon = "delete";
        else if (options() & Stop)
            m_icon = "close-circle";
    }

    if (treeType() == Action && parent) {
        connect(this, &Fact::triggered, static_cast<Fact *>(parent), &Fact::actionTriggered);
    }

    //number of children in status
    if ((treeType() == Group) && (dataType() == Const)) {
        connect(this, &Fact::sizeChanged, this, &Fact::statusChanged);
    }

    //append to parent
    if (parent) {
        if (m_name.contains('#')) {
            connect(parent, &FactData::sizeChanged, this, &FactData::nameChanged);
        }
        if (parent->options() & Section)
            setSection(static_cast<Fact *>(parent)->title());

        setParentFact(parent);
    }
}
//=============================================================================
QVariant Fact::data(int col, int role) const
{
    switch (role) {
    case ModelDataRole:
    case FactRole:
        return QVariant::fromValue(const_cast<Fact *>(this));
    case NameRole:
        return name();
    case ValueRole:
        return value();
    case TextRole:
        return text();
    case Qt::ForegroundRole:
        if (col == Fact::FACT_MODEL_COLUMN_NAME) {
            if (modified())
                return QColor(Qt::red).lighter();
            if (!enabled())
                return QColor(Qt::gray);
            if (active())
                return QColor(Qt::green).lighter();
            if (isZero())
                return QColor(Qt::gray);
            //if(treeItemType()==Fact::FactItem) return QColor(Qt::white);
            //return QColor(Qt::green).lighter(195);
            return QColor(Qt::white); //QVariant();
        }
        if (col == Fact::FACT_MODEL_COLUMN_VALUE) {
            if (!enabled())
                return QColor(Qt::darkGray);
            if (size())
                return QColor(Qt::darkGray); //expandable
            if (modified())
                return QColor(Qt::yellow);
            if (isZero())
                return QColor(Qt::gray);
            //if(ftype==ft_string) return QVariant();
            //if(ftype==ft_varmsk) return QColor(Qt::cyan);
            return QColor(Qt::cyan).lighter(180);
        }
        return QColor(Qt::darkCyan);
    case Qt::BackgroundRole:
        return QVariant();
    case Qt::FontRole: {
        QFont font(QFontDatabase::systemFont(QFontDatabase::GeneralFont)); //qApp->font());
        if (col == Fact::FACT_MODEL_COLUMN_DESCR)
            return QVariant();
        if (treeType() && col == Fact::FACT_MODEL_COLUMN_NAME)
            font.setBold(true);
        //if(ftype>=ft_regPID) return QFont("Monospace",-1,column==tc_field?QFont::Bold:QFont::Normal);
        //if(col==FACT_MODEL_COLUMN_NAME) return QFont("Monospace",-1,QFont::Normal,isModified());
        //if(ftype==ft_string) return QFont("",-1,QFont::Normal,true);
        return font;
    }
    case Qt::ToolTipRole:
        if (col == Fact::FACT_MODEL_COLUMN_NAME) {
            return info();
        } else if (col == Fact::FACT_MODEL_COLUMN_VALUE) {
            if (size()) {
                QString s = name();
                for (int i = 0; i < size(); ++i) {
                    Fact *f = child(i);
                    s += QString("\n%1: %2").arg(f->title()).arg(f->text());
                }
                return s;
            } else
                descr();
        }
        return data(col, Qt::DisplayRole);
    }

    //value roles
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    switch (col) {
    case Fact::FACT_MODEL_COLUMN_NAME:
        return title();
    case Fact::FACT_MODEL_COLUMN_VALUE: {
        if (dataType() == Script)
            return status();
        const QString s = text();
        if (s.isEmpty()) {
            if (!status().isEmpty())
                return status();
            if (treeType() == Group) {
                //if(isZero())return tr("default");
                return QVariant();
            }
        }
        if (role == Qt::EditRole && enumStrings().size() <= 1) {
            if (dataType() == Bool)
                return value().toBool();
            if (dataType() == Int)
                return value().toInt();
            if (dataType() == Float) {
                bool ok = false;
                s.toDouble(&ok);
                if (ok)
                    return s;
                return value().toDouble();
            }
        }
        return s;
    }
    case Fact::FACT_MODEL_COLUMN_DESCR:
        return descr();
    }
    return QVariant();
}
//=============================================================================
QString Fact::info() const
{
    QStringList st;
    QString sDataType;
    if (dataType())
        sDataType = QMetaEnum::fromType<FactBase::Flag>().valueToKey(dataType());
    if (!units().isEmpty())
        sDataType += (sDataType.isEmpty() ? "" : ", ") + units();
    if (sDataType.isEmpty())
        st << QString("%1").arg(name());
    else
        st << QString("%1 [%2]").arg(name()).arg(sDataType);
    if (!descr().isEmpty())
        st << descr();
    st << path();
    if (!m_enumStrings.isEmpty()) {
        if (m_enumStrings.size() > 25)
            st << QString("{%1}").arg(m_enumStrings.size());
        else
            st << QString("{%1}").arg(m_enumStrings.join(','));
    }
    return st.join('\n');
}
//=============================================================================
QByteArray Fact::hash() const
{
    QCryptographicHash h(QCryptographicHash::Sha1);
    hashData(&h);
    return h.result();
}
void Fact::hashData(QCryptographicHash *h) const
{
    for (int i = 0; i < size(); ++i) {
        Fact *f = child(i);
        f->hashData(h);
    }
    //h->addData(name().toUtf8());
    h->addData(title().toUtf8());
    h->addData(descr().toUtf8());
    //h->addData(section().toUtf8());
    //h->addData(QString::number(num()).toUtf8());
    h->addData(QString::number(precision()).toUtf8());
    h->addData(min().toString().toUtf8());
    h->addData(max().toString().toUtf8());
    h->addData(QString::number(treeType()).toUtf8());
    h->addData(QString::number(dataType()).toUtf8());
    h->addData(QString::number(size()).toUtf8());
    h->addData(enumStrings().join("").toUtf8());

    h->addData(text().toUtf8());
    h->addData(userData.toString().toUtf8());
}
//=============================================================================
QVariant Fact::findValue(const QString &namePath)
{
    Fact *f = findChild(namePath);
    if (!f) {
        apxConsoleW() << "FactSystem fact not found:" << namePath;
        return QVariant();
    }
    if (dataType() == Enum)
        return f->text();
    return f->value();
}
//=============================================================================
Fact *Fact::findChild(const QString &factNamePath, bool exactMatch) const
{
    QList<Fact *> slist;
    bool del = factNamePath.contains('.');
    for (int i = 0; i < size(); ++i) {
        Fact *f = child(i);
        if (f->name() == factNamePath)
            return f;
        if (del && f->path().endsWith(factNamePath))
            return f;
        if ((!exactMatch) && f->name().startsWith(factNamePath))
            slist.append(f);
    }
    if (del) {
        for (int i = 0; i < size(); ++i) {
            Fact *f = child(i);
            f = f->findChild(factNamePath);
            if (f)
                return f;
        }
    }
    //qDebug()<<slist.size();
    if (slist.size() == 1)
        return slist.first();
    if (exactMatch && (!del))
        apxConsoleW() << "Fact not found:" << factNamePath; //<<sender();
    return nullptr;
}
//=============================================================================
Fact *Fact::childByTitle(const QString &factTitle) const
{
    for (int i = 0; i < size(); ++i) {
        Fact *f = child(i);
        if (f->title() == factTitle)
            return f;
    }
    return nullptr;
}
//=============================================================================
QString Fact::titlePath(const QChar pathDelimiter) const
{
    QString s;
    for (const FactBase *i = this; i; i = i->parentFact()) {
        const Fact *f = static_cast<const Fact *>(i);
        if (i->treeType() == Root) {
            s.prepend(pathDelimiter);
            break;
        }
        if (i->options() & Section)
            continue;
        QString s2 = f->pTitle();
        if (s.isEmpty())
            s = s2;
        else
            s.prepend(s2 + pathDelimiter);
    }
    return s.isEmpty() ? title() : s;
}
QString Fact::pTitle() const
{
    return title().simplified().replace('/', '_').replace('\n', ' ').replace('=', ' ');
}
//=============================================================================
Fact *Fact::factByTitlePath(const QString &spath) const
{
    int del = spath.indexOf('/');
    if (del < 0) {
        //find child fact by title
        for (int i = 0; i < size(); ++i) {
            Fact *f = child(i);
            if (f->pTitle() == spath)
                return f;
        }
        return nullptr;
    }
    //ignore leading delimiter
    if (del == 0)
        return factByTitlePath(spath.mid(1));
    //find child fact section by title
    QString s = spath.left(del);
    for (int i = 0; i < size(); ++i) {
        Fact *f = child(i);
        if (f->pTitle() == s)
            return f->factByTitlePath(spath.mid(del + 1));
    }
    return nullptr;
}
//=============================================================================
bool Fact::lessThan(Fact *rightFact) const
{
    //no sorting by default
    return num() < rightFact->num();
}
//=============================================================================
void Fact::trigger(void)
{
    if (!enabled())
        return;
    if (bindedFact)
        bindedFact->trigger();
    //qDebug()<<"trigger"<<name();
    emit triggered();
}
void Fact::requestDefaultMenu()
{
    requestMenu(QVariantMap());
}
void Fact::requestMenu(QVariantMap opts)
{
    emit menuRequested(opts);
    AppRoot::instance()->factRequestMenu(this, opts);
}
//=============================================================================
void Fact::bind(FactData *fact)
{
    FactData::bind(fact);
    bool rebind = bindedFact;
    if (bindedFact) {
        disconnect(bindedFact, nullptr, this, nullptr);
    }
    bindedFact = qobject_cast<Fact *>(fact);
    if (bindedFact) {
        connect(bindedFact, &Fact::actionsModelChanged, this, &Fact::actionsModelChanged);
        connect(bindedFact, &Fact::statusChanged, this, &Fact::statusChanged);
        connect(bindedFact, &Fact::activeChanged, this, &Fact::activeChanged);
        connect(bindedFact, &Fact::progressChanged, this, &Fact::progressChanged);
        connect(bindedFact, &Fact::iconChanged, this, &Fact::iconChanged);
        connect(bindedFact, &Fact::qmlPageChanged, this, &Fact::qmlPageChanged);

        connect(bindedFact, &Fact::actionTriggered, this, &Fact::actionTriggered);
    }
    if (rebind) {
        emit actionsModelChanged();
        emit statusChanged();
        emit activeChanged();
        emit progressChanged();
        emit iconChanged();
        emit qmlPageChanged();
    }
}
//=============================================================================
void Fact::setValues(const QVariantMap &values)
{
    foreach (const QString &key, values.keys()) {
        Fact *f = child(key);
        if (f)
            f->setValue(values.value(key));
    }
}
QJsonObject Fact::toJson(bool array) const
{
    if (array) {
        QJsonArray jsa;
        for (int i = 0; i < size(); ++i) {
            const Fact *f = static_cast<const Fact *>(child(i));
            jsa.append(f->toJson(false));
        }
        QJsonObject jso;
        jso.insert(name(), jsa);
        return jso;
    }
    QJsonObject jso;
    for (int i = 0; i < size(); ++i) {
        const Fact *f = static_cast<const Fact *>(child(i));
        QJsonValue v;
        if (f->dataType() == Text)
            v = f->text();
        else if (f->enumStrings().isEmpty())
            v = QJsonValue::fromVariant(f->value());
        else
            v = f->text();
        bool noData = f->dataType() == NoFlags || f->dataType() == Const;
        if (f->size() > 0) {
            QJsonObject vo = f->toJson(false);
            if (!noData) {
                vo.insert("value", v);
            }
            jso.insert(f->name(), vo);
            continue;
        }
        if (noData || f->text().isEmpty())
            continue;
        jso.insert(f->name(), v);
    }
    return jso;
}
void Fact::fromJson(const QJsonObject &jso)
{
    foreach (QString key, jso.keys()) {
        QJsonValue v = jso[key];
        Fact *f = child(key);
        if (!f)
            continue;
        if (v.isObject()) {
            f->fromJson(v.toObject());
            continue;
        }
        if (v.isArray()) {
            continue;
        }
        f->setValue(v.toVariant());
    }
}
//=============================================================================
//=============================================================================
FactListModel *Fact::model() const
{
    return m_model;
}
void Fact::setModel(FactListModel *v)
{
    m_model->deleteLater();
    m_model = v;
    emit modelChanged();
}
FactListModelActions *Fact::actionsModel() const
{
    if (bindedFact)
        return bindedFact->actionsModel();
    return m_actionsModel;
}
void Fact::setActionsModel(FactListModelActions *v)
{
    m_actionsModel->deleteLater();
    m_actionsModel = v;
    emit actionsModelChanged();
}
bool Fact::enabled() const
{
    return m_enabled && m_parentEnabled;
}
void Fact::setEnabled(const bool &v)
{
    if (m_enabled == v)
        return;
    m_enabled = v;
    emit enabledChanged();

    for (int i = 0; i < size(); ++i) {
        child(i)->updateParentEnabled();
    }
}
void Fact::updateParentEnabled()
{
    bool v = parentFact() ? parentFact()->enabled() : true;
    if (m_parentEnabled == v)
        return;
    bool pv = enabled();
    m_parentEnabled = v;
    if (pv != enabled())
        emit enabledChanged();
}
bool Fact::visible() const
{
    return m_visible;
}
void Fact::setVisible(const bool &v)
{
    if (m_visible == v)
        return;
    m_visible = v;
    emit visibleChanged();

    for (int i = 0; i < size(); ++i) {
        child(i)->updateParentVisible();
    }
}
void Fact::updateParentVisible()
{
    bool v = parentFact() ? parentFact()->visible() : true;
    if (m_parentVisible == v)
        return;
    bool pv = visible();
    m_parentVisible = v;
    if (pv != visible())
        emit visibleChanged();
}
QString Fact::section() const
{
    return m_section;
}
void Fact::setSection(const QString &v)
{
    QString s = v.trimmed();
    if (m_section == s)
        return;
    m_section = s;
    emit sectionChanged();
}
QString Fact::status() const
{
    if (bindedFact)
        return bindedFact->status();
    if (treeType() == Group && dataType() == Const && m_status.isEmpty()) {
        return size() > 0 ? QString::number(size()) : QString();
    }
    return m_status;
}
void Fact::setStatus(const QString &v)
{
    if (bindedFact) {
        bindedFact->setStatus(v);
        return;
    }
    QString s = v.trimmed();
    if (m_status == s)
        return;
    m_status = s;
    emit statusChanged();
}
bool Fact::active() const
{
    if (bindedFact)
        return bindedFact->active();
    return m_active;
}
void Fact::setActive(const bool &v)
{
    if (bindedFact) {
        bindedFact->setActive(v);
        return;
    }
    if (m_active == v)
        return;
    m_active = v;
    emit activeChanged();
}
int Fact::progress() const
{
    if (bindedFact)
        return bindedFact->progress();
    return m_progress;
}
void Fact::setProgress(const int &v)
{
    if (bindedFact) {
        bindedFact->setProgress(v);
        return;
    }
    if (m_progress == v)
        return;
    m_progress = v;
    emit progressChanged();
    if (!blockNotify) {
        AppRoot::instance()->factNotify(this);
    }
}
QString Fact::icon() const
{
    if (bindedFact && m_icon.isEmpty())
        return bindedFact->icon();
    return m_icon;
}
void Fact::setIcon(const QString &v)
{
    if (m_icon == v)
        return;
    m_icon = v;
    emit iconChanged();
}
Fact *Fact::link() const
{
    return m_link;
}
void Fact::setLink(Fact *v)
{
    if (m_link == v)
        return;
    m_link = v;
    emit linkChanged();
}
QString Fact::qmlPage() const
{
    if (bindedFact && m_qmlPage.isEmpty())
        return bindedFact->qmlPage();
    return m_qmlPage;
}
void Fact::setQmlPage(const QString &v)
{
    if (m_qmlPage == v)
        return;
    m_qmlPage = v;
    emit qmlPageChanged();
}
QColor Fact::color() const
{
    return m_color;
}
void Fact::setColor(const QColor &v)
{
    if (m_color == v)
        return;
    m_color = v;
    emit colorChanged();
}
//=============================================================================
//=============================================================================
