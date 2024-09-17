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

#include "FactData.h"

class Fact : public FactData
{
    Q_OBJECT

    Q_PROPERTY(FactBase::Flags flags READ flags WRITE setFlags NOTIFY flagsChanged)

    Q_PROPERTY(QAbstractListModel *model READ model NOTIFY modelChanged)
    Q_PROPERTY(QAbstractListModel *actionsModel READ actionsModel NOTIFY actionsModelChanged)

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

    Q_PROPERTY(QString section READ section WRITE setSection NOTIFY sectionChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(int progress READ progress WRITE setProgress NOTIFY progressChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

    Q_PROPERTY(QString icon READ icon WRITE setIcon NOTIFY iconChanged)

    Q_PROPERTY(Fact *binding READ binding WRITE setBinding NOTIFY bindingChanged)
    Q_PROPERTY(Fact *menu READ menu WRITE setMenu NOTIFY menuChanged)

    Q_PROPERTY(QVariantMap opts READ opts WRITE setOpts NOTIFY optsChanged)

public:
    explicit Fact(QObject *parent = nullptr,
                  const QString &name = QString(),
                  const QString &title = QString(),
                  const QString &descr = QString(),
                  FactBase::Flags flags = FactBase::Flags(NoFlags),
                  const QString &icon = QString());

    Q_INVOKABLE QByteArray hash() const;

    Q_INVOKABLE QVariant findValue(const QString &namePath);

    Q_INVOKABLE Fact *findChild(const QString &factName, bool exactMatch = true) const;
    Q_INVOKABLE Fact *childByTitle(const QString &factTitle) const;

    Q_INVOKABLE QString titlePath(const QChar pathDelimiter = QChar('/')) const;
    Q_INVOKABLE Fact *factByTitlePath(const QString &spath) const;

    //Group fact values (settings)
    Q_INVOKABLE void setValues(const QVariantMap &values);

    Q_INVOKABLE virtual QVariant toVariant();
    Q_INVOKABLE virtual void fromVariant(const QVariant &var);

    Q_INVOKABLE QJsonDocument toJsonDocument();
    Q_INVOKABLE bool fromJsonDocument(QByteArray data);
    Q_INVOKABLE static QVariant parseJsonDocument(QByteArray data);

    virtual bool lessThan(Fact *other) const;           //sorting helper
    virtual bool showThis(QRegularExpression re) const; //filter helper

    //data model
    enum {
        FACT_MODEL_COLUMN_NAME = 0,
        FACT_MODEL_COLUMN_VALUE,
        FACT_MODEL_COLUMN_DESCR,

        FACT_MODEL_COLUMN_CNT,
    };
    enum FactModelRoles {
        ModelDataRole = Qt::UserRole + 1,
        FactRole,
        NameRole,
        ValueRole,
        TextRole,
        EditorWidgetRole,
    };

    Q_INVOKABLE bool hasParent(Fact *parent) const;
    Q_INVOKABLE bool hasChild(Fact *child) const;

    template<class T>
    T *findParent() const
    {
        for (Fact *i = const_cast<Fact *>(this); i; i = i->parentFact()) {
            T *p = qobject_cast<T *>(i);
            if (p)
                return p;
        }
        return nullptr;
    }

    template<class T>
    FactListT<T> findFacts() const
    {
        FactListT<T> list;
        for (auto &i : facts()) {
            T *p = qobject_cast<T *>(i);
            if (p)
                list.append(p);
        }
        return list;
    }

    virtual QVariant data(int col, int role);

    Q_INVOKABLE virtual QString toolTip() const;

    virtual void hashData(QCryptographicHash *h) const;

    //create action fact that opens this fact, or binded to this action
    Q_INVOKABLE Fact *createAction(Fact *parent);

    //forward to app instance with fact opts
    Q_INVOKABLE QObject *loadQml(const QString &qmlFile);

    //Mandala support
    Fact *mandala() const;
    void setMandala(Fact *v);
    virtual QString mandalaToString(quint16 pid_raw) const override;
    virtual quint16 stringToMandala(const QString &s) const override;

public:
    FactBase::Flags flags() const;
    void setFlags(FactBase::Flags v);

    QAbstractListModel *model();
    void setModel(QAbstractListModel *v);

    QAbstractListModel *actionsModel();
    void setActionsModel(QAbstractListModel *v);

    bool enabled() const;
    void setEnabled(const bool v);

    bool visible() const;
    void setVisible(const bool v);

    QString section() const;
    void setSection(const QString &v);

    bool active() const;
    void setActive(const bool v);

    int progress() const;
    void setProgress(const int v);
    bool busy() const;

    QString icon() const;
    void setIcon(const QString &v);

    Fact *binding() const;
    void setBinding(Fact *v);

    Fact *menu();
    void setMenu(Fact *v);

    QVariantMap opts() const;
    void setOpts(const QVariantMap &v);
    void setOpt(const QString &name, const QVariant &v);

protected:
    QAbstractListModel *m_model{nullptr};
    QAbstractListModel *m_actionsModel{nullptr};

    bool m_enabled{true};
    bool m_visible{true};
    QString m_section;
    QString m_statusText;
    bool m_active{false};
    int m_progress{-1};
    QString m_icon;

    QPointer<Fact> m_binding{nullptr};
    QPointer<Fact> m_menu{nullptr};

    QVariantMap m_opts;

    int m_scnt{0};

private:
    Fact *m_mandala{nullptr};

    int m_progress_s{0};

private:
    QString pTitle() const;

    void updateBinding(Fact *src);

    //tree properties propagate
private:
    bool m_parentEnabled{true};
    void updateParentEnabled();
    bool m_parentVisible{true};
    void updateParentVisible();

private slots:
    void updateModels();
    void onOptionsChanged();
    void trackProgress();

public slots:
    //trigger fact from UI (f.ex. to display menu)
    void trigger(QVariantMap opts = QVariantMap());

signals:
    void triggered(QVariantMap opts);
    void menuBack();

signals:
    void flagsChanged();

    void modelChanged();
    void actionsModelChanged();
    void enabledChanged();
    void visibleChanged();

    void sectionChanged();
    void activeChanged();
    void progressChanged();
    void busyChanged();

    void iconChanged();

    void bindingChanged();
    void menuChanged();

    void qmlPageChanged();
    void optsChanged();

    void scntChanged();
};
