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
#include "Fact.h"
#include "FactListModel.h"
#include "FactListModelActions.h"

#include <App/App.h>
#include <App/AppLog.h>
#include <App/AppNotify.h>

#include <QColor>
#include <QFont>
#include <QFontDatabase>

Fact::Fact(QObject *parent,
           const QString &name,
           const QString &title,
           const QString &descr,
           Flags flags,
           const QString &icon)
    : FactData(nullptr, name, title, descr, flags)
{
    setIcon(icon);

    //models
    connect(this, &Fact::actionsUpdated, this, &Fact::updateModels);
    connect(this, &Fact::itemInserted, this, &Fact::updateModels);
    connect(this, &Fact::itemRemoved, this, &Fact::updateModels);

    connect(this, &Fact::parentFactChanged, this, &Fact::updateParentEnabled);
    updateParentEnabled();
    connect(this, &Fact::parentFactChanged, this, &Fact::updateParentVisible);
    updateParentVisible();

    connect(this, &Fact::optionsChanged, this, &Fact::onOptionsChanged);
    onOptionsChanged();

    connect(this, &Fact::parentFactChanged, this, [this]() {
        if (parentFact() && parentFact()->options() & Section)
            setSection(parentFact()->title());
    });

    //flags
    connect(this, &Fact::treeTypeChanged, this, &Fact::flagsChanged);
    connect(this, &Fact::dataTypeChanged, this, &Fact::flagsChanged);
    connect(this, &Fact::optionsChanged, this, &Fact::flagsChanged);

    //append to parent
    setParentFact(qobject_cast<Fact *>(parent));
}

void Fact::onOptionsChanged()
{
    if (options() & ProgressTrack) {
        connect(this, &Fact::sizeChanged, this, &Fact::trackProgress);
    } else {
        disconnect(this, &Fact::sizeChanged, this, &Fact::trackProgress);
    }
    // icon
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

QVariant Fact::data(int col, int role)
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
            return QColor(Qt::white);
        }
        if (col == Fact::FACT_MODEL_COLUMN_VALUE) {
            if (!enabled())
                return QColor(Qt::darkGray);
            if (modified())
                return QColor(Qt::yellow);
            if (size() && !text().startsWith('[')) //expandable
                return QColor(Qt::darkGray);
            if (isZero())
                return QColor(Qt::gray);
            //if (isDefault())
            //    return QColor(Qt::blue).lighter(180);
            return QColor(Qt::cyan).lighter(180);
        }
        return QColor(Qt::darkCyan);
    case Qt::BackgroundRole:
        return QVariant();
    case Qt::FontRole: {
        // QFont font(QFontDatabase::systemFont(QFontDatabase::GeneralFont));
        QFont font(QGuiApplication::font());
        if (col == Fact::FACT_MODEL_COLUMN_DESCR)
            return font;
        if (treeType() && col == Fact::FACT_MODEL_COLUMN_NAME)
            font.setBold(true);
        return font;
    }
    case Qt::ToolTipRole:
        if (col == Fact::FACT_MODEL_COLUMN_NAME) {
            return toolTip();
        } else if (col == Fact::FACT_MODEL_COLUMN_VALUE) {
            if (size()) {
                QString s = name();
                for (int i = 0; i < size(); ++i) {
                    Fact *f = child(i);
                    s += QString("\n%1: %2").arg(f->title(), f->text());
                }
                return s;
            } else {
                return toolTip();
            }
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
        const QString s = text();
        if (s.isEmpty()) {
            if (treeType() == Group) {
                return QVariant();
            }
        }
        if (role == Qt::EditRole)
            return editorText();
        /*if (role == Qt::EditRole && enumStrings().size() <= 1) {
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
        }*/
        return s;
    }
    case Fact::FACT_MODEL_COLUMN_DESCR:
        return descr();
    }
    return QVariant();
}

QString Fact::toolTip() const
{
    QStringList st;
    QString sDataType;
    if (dataType())
        sDataType = QMetaEnum::fromType<FactBase::Flag>().valueToKey(static_cast<int>(dataType()));
    if (!units().isEmpty())
        sDataType += (sDataType.isEmpty() ? "" : ", ") + units();
    QString s = name();
    if (s != title())
        s = QString("%1 (%2)").arg(s, title());

    if (!sDataType.isEmpty())
        s = QString("%1 [%2]").arg(s, sDataType);

    st << s;

    if (!descr().isEmpty())
        st << QString("Descr: %1").arg(descr());
    st << QString("Path: %1").arg(path());
    if (!m_enumStrings.isEmpty()) {
        if (m_enumStrings.size() > 25)
            st << QString("Values: [%1]").arg(m_enumStrings.size());
        else
            st << QString("Values: {%1}").arg(m_enumStrings.join(','));
    }
    if (!defaultValue().isNull())
        st << QString("Default: %1").arg(defaultValue().toString());
    if (!min().isNull())
        st << QString("Min: %1").arg(min().toString());
    if (!max().isNull())
        st << QString("Max: %1").arg(max().toString());
    if (increment() > 0)
        st << QString("Increment: %1").arg(increment());
    if (precision() >= 0)
        st << QString("Precision: %1").arg(precision());

    if (!section().isEmpty())
        st << QString("Section: %1").arg(section());

    if (!opts().isEmpty()) {
        st << "";
        st << "[opts]";
        for (auto k : opts().keys())
            st << QString("%1: %2").arg(k, opts().value(k).toString());
    }

    return st.join('\n');
}

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
    h->addData(QString::number(precision()).toUtf8());
    h->addData(min().toString().toUtf8());
    h->addData(max().toString().toUtf8());
    h->addData(QString::number(treeType()).toUtf8());
    h->addData(QString::number(dataType()).toUtf8());
    h->addData(QString::number(size()).toUtf8());
    h->addData(enumStrings().join("").toUtf8());

    h->addData(value().toString().toUtf8());
}

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
    return facts().contains(child);
}

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

Fact *Fact::findChild(const QString &factNamePath, bool exactMatch) const
{
    FactList slist;
    bool del = factNamePath.contains('.');
    for (auto i : facts()) {
        if (i->name() == factNamePath)
            return i;
        if (del && i->path().endsWith(factNamePath))
            return i;
        if ((!exactMatch) && i->name().startsWith(factNamePath))
            slist.append(i);
    }
    if (del) {
        for (auto i : facts()) {
            i = i->findChild(factNamePath);
            if (i)
                return i;
        }
    }
    //qDebug()<<slist.size();
    if (slist.size() == 1)
        return slist.first();
    if (exactMatch && (!del))
        apxConsoleW() << "Fact not found:" << factNamePath; //<<sender();
    return nullptr;
}

Fact *Fact::childByTitle(const QString &factTitle) const
{
    for (int i = 0; i < size(); ++i) {
        Fact *f = child(i);
        if (f->title() == factTitle)
            return f;
    }
    return nullptr;
}

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

bool Fact::lessThan(Fact *other) const
{
    //no sorting by default
    return num() < other->num();
}
bool Fact::showThis(QRegularExpression re) const
{
    if (options() & FilterExclude)
        return false;
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

void Fact::trigger(QVariantMap opts)
{
    if (!enabled())
        return;

    if (treeType() == Action && dataType() == Bool) {
        setValue(!value().toBool());
    }

    if (binding()) {
        if (binding()->treeType() == Action) {
            binding()->trigger(opts);
            return;
        }
    }

    //qDebug() << "trigger" << path();
    emit triggered(opts);
    emit AppRoot::instance() -> factTriggered(this, opts);

    if (binding() && size() <= 0)
        binding()->trigger(opts);
}

QObject *Fact::loadQml(const QString &qmlFile)
{
    QVariantMap opts;
    opts.insert("fact", QVariant::fromValue(this));
    return App::loadQml(qmlFile, opts);
}

void Fact::updateBinding(Fact *src)
{
    if (m_binding == src)
        return;

    if (m_binding) {
        unbindProperties(m_binding);
        m_binding->unbindProperties(this);
    }
    m_binding = src;
    setMenu(src);

    if (!src)
        return;

    if (m_title.isEmpty())
        bindProperty(src, "title", true);
    if (m_descr.isEmpty())
        bindProperty(src, "descr", true);
    if (m_icon.isEmpty())
        bindProperty(src, "icon", true);
    //if (m_opts.isEmpty())
    //    bindProperty(src, "opts", true);

    if (treeType() != Action && src->treeType() != Action) {
        bindProperty(src, "value");
        bindProperty(src, "modified");

        bindProperty(src, "backupValue", true);
        bindProperty(src, "defaultValue", true);
        bindProperty(src, "enabled", true);
        bindProperty(src, "progress", true);
        bindProperty(src, "active", true);

        if (!dataType())
            setDataType(src->dataType());

        if (treeType() != Group && src->treeType() != Group) {
            bindProperty(src, "precision", true);
            bindProperty(src, "min", true);
            bindProperty(src, "max", true);
            bindProperty(src, "valueText", true);
            bindProperty(src, "enumStrings", true);
            bindProperty(src, "units", true);
        }

    } else {
        bindProperty(src, "enabled", true);
    }
}

Fact *Fact::createAction(Fact *parent)
{
    Fact *f = new Fact(parent, name(), "", "", Action | dataType() | options(), icon());
    f->setBinding(this);
    return f;
}

void Fact::setValues(const QVariantMap &values)
{
    foreach (const QString &key, values.keys()) {
        Fact *f = child(key);
        if (f)
            f->setValue(values.value(key));
    }
}
QJsonDocument Fact::toJsonDocument()
{
    return QJsonDocument::fromVariant(toVariant());
}
bool Fact::fromJsonDocument(QByteArray data)
{
    auto var = parseJsonDocument(data);
    if (var.isNull())
        return false;
    fromVariant(var);
    return true;
}
QVariant Fact::parseJsonDocument(QByteArray data)
{
    QJsonParseError err;
    auto json = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        apxMsgW() << err.errorString();
        return {};
    }
    if (json.isObject() || json.isArray()) {
        return json.toVariant();
    }
    return {};
}

QVariant Fact::toVariant()
{
    if (treeType() == Action || !visible())
        return {};
    if (size() > 0) {
        if (treeType() != NoFlags && dataType() == Count) {
            QVariantList a;
            for (auto i : facts()) {
                QVariant v = i->toVariant();
                if (!v.isNull())
                    a.append(v);
            }
            if (a.isEmpty())
                return {};
            return a;
        }

        QVariantMap h;
        for (auto i : facts()) {
            QVariant v = i->toVariant();
            if (!v.isNull())
                h.insert(i->name(), v);
        }
        if (h.isEmpty())
            return {};
        return h;
    }

    QVariant v;
    do {
        if (treeType() != NoFlags)
            break;
        if (dataType() == NoFlags || dataType() == Count || valueText().isEmpty())
            break;

        if (dataType() == Text || dataType() == Bool)
            v = valueText();
        else if (enumStrings().isEmpty())
            v = value();
        else
            v = valueText();
    } while (0);

    return v;
}
void Fact::fromVariant(const QVariant &var)
{
    if (var.isNull())
        return;

    if (var.typeId() == QMetaType::QVariantMap) {
        auto m = var.value<QVariantMap>();
        for (auto key : m.keys()) {
            Fact *f = child(key);
            if (!f) {
                qWarning() << "missing json fact" << key << path();
                continue;
            }
            auto v = m.value(key);
            if (v.typeId() == QMetaType::QVariantMap) {
                f->fromVariant(v);
                continue;
            }
            if (v.typeId() == QMetaType::QVariantList) {
                f->fromVariant(v);
                continue;
            }
            f->setValue(v);
        }
        return;
    }
    if (var.typeId() == QMetaType::QVariantList) {
        // must be implemented in subclasses to create children structure from array
        return;
    }
    if (size() > 0)
        return;
    if (treeType() != NoFlags)
        return;
    if (dataType() == NoFlags || dataType() == Count)
        return;

    setValue(var);
}

Fact *Fact::mandala() const
{
    for (const Fact *f = this; f; f = f->parentFact()) {
        if (f->m_mandala)
            return f->m_mandala;
    }
    return AppRoot::instance()->mandala();
}
void Fact::setMandala(Fact *v)
{
    m_mandala = v;
    updateValueText();
}
QString Fact::mandalaToString(quint16 pid_raw) const
{
    Fact *m = mandala();
    return m ? m->mandalaToString(pid_raw) : QString();
}
quint16 Fact::stringToMandala(const QString &s) const
{
    Fact *m = mandala();
    return m ? m->stringToMandala(s) : 0;
}

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
QAbstractItemModel *Fact::model()
{
    /*if (!m_model && size() <= 0 && bind()) {
        return bind()->model();
    }*/
    bool bEmpty = size() <= 0 && treeType() != Group;
    if (!m_model) {
        if (!bEmpty) {
            FactListModel *m = new FactListModel(this);
            m_model = m;
            m->sync();
        }
    } else if (bEmpty && qobject_cast<FactListModel *>(m_model)) {
        m_model->deleteLater();
        m_model = nullptr;
    }
    return m_model;
}
void Fact::setModel(QAbstractItemModel *v)
{
    if (m_model)
        m_model->deleteLater();
    m_model = v;
    emit modelChanged();
}
QAbstractItemModel *Fact::actionsModel()
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
void Fact::setActionsModel(QAbstractItemModel *v)
{
    if (m_actionsModel)
        m_actionsModel->deleteLater();
    m_actionsModel = v;
    emit actionsModelChanged();
}
bool Fact::enabled() const
{
    return m_enabled && m_parentEnabled;
}
void Fact::setEnabled(const bool v)
{
    /*if (v == false && name() == "vehicles") {
        qDebug() << "BEGIN" << path() << v << m_parentEnabled;
    }*/
    //qDebug() << "BEGIN" << path() << v << m_parentEnabled;
    if (m_enabled == v)
        return;
    m_enabled = v;
    emit enabledChanged();

    for (int i = 0; i < size(); ++i) {
        child(i)->updateParentEnabled();
    }
    for (auto i : actions()) {
        i->updateParentEnabled();
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

    //update children
    for (auto i : facts()) {
        i->updateParentEnabled();
    }
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

    for (auto i : facts()) {
        i->updateParentVisible();
    }
    for (auto i : actions()) {
        i->updateParentVisible();
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
bool Fact::active() const
{
    return m_active;
}
void Fact::setActive(const bool v)
{
    if (m_active == v)
        return;
    m_active = v;
    emit activeChanged();
}
int Fact::progress() const
{
    return m_progress;
}
void Fact::setProgress(const int v)
{
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

    // check parent with ProgressTrack
    if (parentFact() && parentFact()->options() & ProgressTrack) {
        parentFact()->trackProgress();
    }
}
void Fact::trackProgress()
{
    int ncnt = 0, v = 0;
    if (options() & ProgressTrack) {
        for (auto const f : facts()) {
            int np = f->progress();
            if (np < 0)
                continue;
            ncnt++;
            v += np;
        }
    }
    if (ncnt > 0) {
        if (m_progress_s < ncnt) {
            m_progress_s = ncnt;
            //qDebug()<<"progressCnt"<<p->progress_s<<ncnt<<p->name()<<name();
        } else if (ncnt < m_progress_s) {
            v += (m_progress_s - ncnt) * 100;
            ncnt = m_progress_s;
        }
        setProgress(v / ncnt);
    } else {
        m_progress_s = 0;
        setProgress(-1);
        //qDebug()<<"progressCnt"<<p->progress_s;
    }
}

bool Fact::busy() const
{
    return progress() >= 0;
}
QString Fact::icon() const
{
    return m_icon;
}
void Fact::setIcon(const QString &v)
{
    if (m_icon == v)
        return;
    m_icon = v;
    emit iconChanged();

    // test availability
    if (!v.isEmpty()) {
        if (App::materialIconChar(v).isEmpty())
            qWarning() << path();
    }
}
Fact *Fact::binding() const
{
    return m_binding;
}
void Fact::setBinding(Fact *v)
{
    if (m_binding == v)
        return;
    updateBinding(v);
    emit bindingChanged();
}
Fact *Fact::menu()
{
    if (m_menu)
        return m_menu->menu();

    if (size() > 0)
        return this;
    if (opts().contains("page"))
        return this;
    if (treeType() == Root)
        return this;
    if (dataType() == Int && units() == "mandala")
        return mandala();

    if (treeType() == Group)
        return this;

    return nullptr;
}
void Fact::setMenu(Fact *v)
{
    if (m_menu == v)
        return;
    m_menu = v;
    emit menuChanged();
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
