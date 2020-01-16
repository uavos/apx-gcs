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
#include <App/App.h>
#include <App/AppLog.h>
#include <App/AppNotify.h>
#include <QColor>
#include <QFont>
#include <QFontDatabase>
//=============================================================================
Fact::Fact(QObject *parent,
           const QString &name,
           const QString &title,
           const QString &descr,
           Flags flags,
           const QString &icon)
    : FactData(parent, name, title, descr, flags)
    , m_mandala(nullptr)
    , m_model(nullptr)
    , m_actionsModel(nullptr)
    , m_enabled(true)
    , m_visible(true)
    , m_active(false)
    , m_progress(-1)
    , m_icon(icon)
    , m_bind(nullptr)
    , m_parentEnabled(true)
    , m_parentVisible(true)
{
    //models
    //m_model = new FactListModel(this);
    //m_actionsModel = new FactListModelActions(this);
    connect(this, &Fact::actionsUpdated, this, &Fact::updateModels);
    connect(this, &Fact::itemInserted, this, &Fact::updateModels);
    connect(this, &Fact::itemRemoved, this, &Fact::updateModels);

    connect(this, &Fact::parentFactChanged, this, &Fact::updateParentEnabled);
    updateParentEnabled();
    connect(this, &Fact::parentFactChanged, this, &Fact::updateParentVisible);
    updateParentVisible();

    connect(this, &Fact::optionsChanged, this, &Fact::updateDefaultIcon);
    updateDefaultIcon();

    //number of children in status
    connect(this, &Fact::sizeChanged, this, &Fact::statusChanged);

    if (m_name.contains('#')) {
        connect(this, &Fact::numChanged, this, &Fact::nameChanged);
    }

    connect(this, &Fact::parentFactChanged, this, [this]() {
        if (parentFact() && parentFact()->options() & Section)
            setSection(static_cast<Fact *>(parentFact())->title());
    });

    //flags
    connect(this, &Fact::treeTypeChanged, this, &Fact::flagsChanged);
    connect(this, &Fact::dataTypeChanged, this, &Fact::flagsChanged);
    connect(this, &Fact::optionsChanged, this, &Fact::flagsChanged);

    //append to parent
    setParentFact(qobject_cast<FactBase *>(parent));

    /*if (!name.isEmpty() && name.front().isUpper()) { //.toLower() != name) {
        qDebug() << path() << name;
    }*/
}
//=============================================================================
void Fact::updateDefaultIcon()
{
    if (!icon().isEmpty())
        return;
    switch (dataType()) {
    default:
        break;
    case Apply:
        setIcon("check");
        break;
    case Remove:
        setIcon("delete");
        break;
    case Stop:
        setIcon("close-circle");
        break;
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
        sDataType = QMetaEnum::fromType<FactBase::Flag>().valueToKey(static_cast<int>(dataType()));
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
bool Fact::hasParent(Fact *parent) const
{
    for (const Fact *i = parentFact(); i; i = i->parentFact()) {
        if (i == parent)
            return true;
    }
    return false;
}
bool Fact::hasChild(Fact *child) const
{
    return children().contains(child);
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
bool Fact::showThis(QRegExp re) const
{
    if (name().contains(re))
        return true;
    if (!(options() & FilterSearchAll))
        return false;
    if (title().contains(re))
        return true;
    if (descr().contains(re))
        return true;
    return false;
}
//=============================================================================
void Fact::trigger(QVariantMap opts)
{
    if (!enabled())
        return;

    if (bind()) {
        if (bind()->treeType() == Action) {
            bind()->trigger(opts);
            return;
        }
    }

    //qDebug() << "trigger" << path();
    emit triggered(opts);
    AppRoot::instance()->factTriggered(this, opts);

    if (bind() && size() <= 0)
        bind()->trigger(opts);
}
Fact *Fact::menu()
{
    if (size() > 0)
        return this;
    if (!qmlPage().isEmpty())
        return this;
    if (treeType() == Root)
        return this;
    if (dataType() == Mandala)
        return this;

    if (treeType() == Group)
        return bind() ? bind()->menu() : this;

    if (bind())
        return bind()->menu();
    return nullptr;
}
//=============================================================================
QObject *Fact::loadQml(const QString &qmlFile)
{
    QVariantMap opts;
    opts.insert("fact", QVariant::fromValue(this));
    return App::instance()->engine()->loadQml(qmlFile, opts);
}
//=============================================================================
void Fact::bind(FactData *fact)
{
    FactData::bind(fact);
    bool rebind = bind();
    if (bind()) {
        disconnect(m_bind, nullptr, this, nullptr);
    }
    m_bind = qobject_cast<Fact *>(fact);
    if (bind()) {
        //connect(m_bind, &Fact::actionsModelChanged, this, &Fact::actionsModelChanged);
        connect(m_bind, &Fact::statusChanged, this, &Fact::statusChanged);
        connect(m_bind, &Fact::activeChanged, this, &Fact::activeChanged);
        connect(m_bind, &Fact::progressChanged, this, &Fact::progressChanged);
        connect(m_bind, &Fact::enabledChanged, this, &Fact::enabledChanged);
        connect(m_bind, &Fact::iconChanged, this, &Fact::iconChanged);
        connect(m_bind, &Fact::qmlPageChanged, this, &Fact::qmlPageChanged);
    }
    if (rebind) {
        //emit actionsModelChanged();
        emit statusChanged();
        emit activeChanged();
        emit progressChanged();
        emit enabledChanged();
        emit iconChanged();
        emit qmlPageChanged();
    }
}
//=============================================================================
Fact *Fact::createAction(Fact *parent)
{
    Fact *f = new Fact(parent, m_name, "", "", Action | dataType() | options(), icon());
    f->bind(this);
    return f;
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
QJsonObject Fact::valuesToJson(bool array) const
{
    if (array) {
        QJsonArray jsa;
        for (int i = 0; i < size(); ++i) {
            const Fact *f = static_cast<const Fact *>(child(i));
            jsa.append(f->valuesToJson(false));
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
            QJsonObject vo = f->valuesToJson(false);
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
void Fact::valuesFromJson(const QJsonObject &jso)
{
    foreach (QString key, jso.keys()) {
        QJsonValue v = jso[key];
        Fact *f = child(key);
        if (!f)
            continue;
        if (v.isObject()) {
            f->valuesFromJson(v.toObject());
            continue;
        }
        if (v.isArray()) {
            continue;
        }
        f->setValue(v.toVariant());
    }
}
//=============================================================================
Fact::MandalaMap *Fact::mandala() const
{
    for (const Fact *f = this; f; f = f->parentFact()) {
        if (f->m_mandala)
            return f->m_mandala;
    }
    return nullptr;
}
QString Fact::mandalaToString(quint16 mid) const
{
    MandalaMap *m = mandala();
    if (!m)
        return QString();
    Fact *f = m->value(mid);
    return f ? f->title() : QString();
}
quint16 Fact::stringToMandala(const QString &s) const
{
    if (s.isEmpty() || s == "0")
        return 0;
    MandalaMap *m = mandala();
    if (!m)
        return 0;

    //try int
    bool ok = false;
    uint i = s.toUInt(&ok);
    if (ok && i < 0xFFFF) {
        quint16 mid = static_cast<quint16>(i);
        Fact *f = m->value(mid);
        if (f)
            return mid;
    }
    //try text
    for (auto mid : m->keys()) {
        if (m->value(mid)->title() != s)
            continue;
        return mid;
    }
    return 0;
}
QStringList Fact::mandalaNames() const
{
    QStringList st;
    MandalaMap *m = mandala();
    if (!m)
        return st;
    for (auto f : m->values())
        st << f->title();
    return st;
}
void Fact::setMandalaMap(MandalaMap *v)
{
    m_mandala = v;
}
//=============================================================================
//=============================================================================
void Fact::updateModels()
{
    if (size() > 0) {
        if (!m_model) {
            emit modelChanged();
        }
    } else {
        if (m_model) {
            emit modelChanged();
        }
    }
    if (!actions().isEmpty()) {
        if (!m_actionsModel) {
            emit actionsModelChanged();
        }
    } else {
        if (m_actionsModel) {
            emit actionsModelChanged();
        }
    }
}
//=============================================================================
FactBase::Flags Fact::flags(void) const
{
    return treeType() | dataType() | options();
}
void Fact::setFlags(FactBase::Flags v)
{
    setTreeType(static_cast<Flag>(static_cast<int>(v & TypeMask)));
    setDataType(static_cast<Flag>(static_cast<int>(v & DataMask)));
    setOptions(v & OptsMask);
}
FactListModel *Fact::model()
{
    /*if (!m_model && size() <= 0 && bind()) {
        return bind()->model();
    }*/
    bool bEmpty = size() <= 0 && treeType() != Group;
    if (!m_model) {
        if (!bEmpty) {
            m_model = new FactListModel(this);
            m_model->sync();
        }
    } else if (bEmpty) {
        m_model->deleteLater();
        m_model = nullptr;
    }
    return m_model;
}
void Fact::setModel(FactListModel *v)
{
    if (m_model)
        m_model->deleteLater();
    m_model = v;
    emit modelChanged();
}
FactListModelActions *Fact::actionsModel()
{
    bool bEmpty = actions().isEmpty();
    if (!m_actionsModel) {
        if (!bEmpty)
            m_actionsModel = new FactListModelActions(this);
    } else if (bEmpty) {
        m_actionsModel->deleteLater();
        m_actionsModel = nullptr;
    }
    return m_actionsModel;
}
void Fact::setActionsModel(FactListModelActions *v)
{
    if (m_actionsModel)
        m_actionsModel->deleteLater();
    m_actionsModel = v;
    emit actionsModelChanged();
}
bool Fact::enabled() const
{
    if (bind())
        return bind()->enabled();
    return m_enabled && m_parentEnabled;
}
void Fact::setEnabled(const bool v)
{
    //qDebug() << "BEGIN" << path() << v << m_parentEnabled;
    if (bind()) {
        bind()->setEnabled(v);
        return;
    }
    if (m_enabled == v)
        return;
    m_enabled = v;
    emit enabledChanged();

    for (int i = 0; i < size(); ++i) {
        child(i)->updateParentEnabled();
    }
    for (int i = 0; i < actions().size(); ++i) {
        Fact *f = static_cast<Fact *>(actions().at(i));
        f->updateParentEnabled();
    }
    //qDebug() << "END" << path() << v << m_parentEnabled;
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
void Fact::setVisible(const bool v)
{
    if (m_visible == v)
        return;
    m_visible = v;
    emit visibleChanged();

    for (int i = 0; i < size(); ++i) {
        child(i)->updateParentVisible();
    }
    for (int i = 0; i < actions().size(); ++i) {
        Fact *f = static_cast<Fact *>(actions().at(i));
        f->updateParentVisible();
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
    if (bind())
        return bind()->status();
    if (treeType() == Group && dataType() == Const && m_status.isEmpty()) {
        return size() > 0 ? QString::number(size()) : QString();
    }
    return m_status;
}
void Fact::setStatus(const QString &v)
{
    if (bind()) {
        bind()->setStatus(v);
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
    if (bind())
        return bind()->active();
    return m_active;
}
void Fact::setActive(const bool v)
{
    if (bind()) {
        bind()->setActive(v);
        return;
    }
    if (m_active == v)
        return;
    m_active = v;
    emit activeChanged();
}
int Fact::progress() const
{
    if (bind())
        return bind()->progress();
    return m_progress;
}
void Fact::setProgress(const int v)
{
    if (bind()) {
        bind()->setProgress(v);
        return;
    }
    if (m_progress == v)
        return;
    int vp = m_progress;
    m_progress = v;
    emit progressChanged();
    AppRoot::instance()->updateProgress(this);
    if ((vp < 0 && v >= 0) || (vp >= 0 && v < 0)) {
        emit busyChanged();
        if (busy()) {
            AppNotify::instance()->report(this);
        }
    }
}
bool Fact::busy() const
{
    return progress() >= 0;
}
QString Fact::icon() const
{
    if (bind() && m_icon.isEmpty())
        return bind()->icon();
    return m_icon;
}
void Fact::setIcon(const QString &v)
{
    if (m_icon == v)
        return;
    m_icon = v;
    emit iconChanged();
}
Fact *Fact::bind() const
{
    return m_bind.isNull() ? nullptr : m_bind;
}
void Fact::setBind(Fact *v)
{
    if (m_bind == v)
        return;
    bind(v);
    emit bindChanged();
}
QString Fact::qmlPage() const
{
    if (bind() && m_qmlPage.isEmpty())
        return bind()->qmlPage();
    return m_qmlPage;
}
void Fact::setQmlPage(const QString &v)
{
    if (m_qmlPage == v)
        return;
    m_qmlPage = v;
    emit qmlPageChanged();
}
QVariantMap Fact::opts() const
{
    return m_opts;
}
void Fact::setOpts(const QVariantMap &v)
{
    if (m_opts == v)
        return;
    m_opts = v;
    emit optsChanged();
}
void Fact::setOpt(const QString &name, const QVariant &v)
{
    if (name.isEmpty())
        return;
    if (v.isNull()) {
        if (!m_opts.contains(name))
            return;
        m_opts.remove(name);
    } else {
        if (m_opts.value(name) == v)
            return;
        m_opts.insert(name, v);
    }
    emit optsChanged();
}
//=============================================================================
//=============================================================================
