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
#ifndef Fact_H
#define Fact_H
//=============================================================================
#include "FactData.h"
#include "FactListModel.h"
#include "FactListModelActions.h"
//=============================================================================
class Fact : public FactData
{
    Q_OBJECT

    Q_PROPERTY(FactBase::Flags flags READ flags WRITE setFlags NOTIFY flagsChanged)

    Q_PROPERTY(FactListModel *model READ model NOTIFY modelChanged)
    Q_PROPERTY(FactListModelActions *actionsModel READ actionsModel NOTIFY actionsModelChanged)

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

    Q_PROPERTY(QString section READ section WRITE setSection NOTIFY sectionChanged)
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(int progress READ progress WRITE setProgress NOTIFY progressChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

    Q_PROPERTY(QString icon READ icon WRITE setIcon NOTIFY iconChanged)

    Q_PROPERTY(Fact *bind READ bind WRITE setBind NOTIFY bindChanged)

    Q_PROPERTY(QString qmlPage READ qmlPage WRITE setQmlPage NOTIFY qmlPageChanged)
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
    Q_INVOKABLE virtual void valuesFromJson(const QJsonObject &jso);
    Q_INVOKABLE virtual QJsonObject valuesToJson(bool array = false) const;

    QVariant userData;

    virtual bool lessThan(Fact *rightFact) const; //sorting helper
    virtual bool showThis(QRegExp re) const;      //filter helper

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
    };

    // type conversions
    Fact *parentFact() const { return qobject_cast<Fact *>(FactBase::parentFact()); }

    Q_INVOKABLE bool hasParent(Fact *parent) const;
    Q_INVOKABLE bool hasChild(Fact *child) const;

    template<class T>
    T findParent() const
    {
        for (FactBase *i = parentFact(); i; i = i->parentFact()) {
            T p = qobject_cast<T>(i);
            if (p)
                return p;
        }
        return nullptr;
    }

    template<class T = Fact>
    T *child(int n) const
    {
        return qobject_cast<T *>(FactBase::child(n));
    }

    template<class T = Fact>
    T *child(const QString &name) const
    {
        return qobject_cast<T *>(FactBase::child(name));
    }

    virtual QVariant data(int col, int role) const;

    Q_INVOKABLE virtual QString info() const;

    virtual void hashData(QCryptographicHash *h) const;

    Q_INVOKABLE void bind(FactData *fact) override;

    //create action fact that opens this fact, or binded to this action
    Q_INVOKABLE Fact *createAction(Fact *parent);

    //fact menu. returns fact (this or binded) which has the menu or null
    Q_INVOKABLE Fact *menu();

    //forward to app instance with fact opts
    Q_INVOKABLE QObject *loadQml(const QString &qmlFile);

    //Mandala support
    typedef QMap<quint16, Fact *> MandalaMap;

    QString mandalaToString(quint16 mid) const override;
    quint16 stringToMandala(const QString &s) const override;
    QStringList mandalaNames() const;
    void setMandalaMap(MandalaMap *v);

protected:
    MandalaMap *mandala() const;
    MandalaMap *m_mandala;

private:
    QString pTitle() const;

    void updateDefaultIcon();

private slots:
    void updateModels();

public slots:
    //trigger fact from UI (f.ex. to display menu)
    void trigger(QVariantMap opts = QVariantMap());

signals:
    void triggered(QVariantMap opts);
    void menuBack();

    //---------------------------------------
    // PROPERTIES
public:
    FactBase::Flags flags() const;
    void setFlags(FactBase::Flags v);

    FactListModel *model();
    void setModel(FactListModel *v);

    FactListModelActions *actionsModel();
    void setActionsModel(FactListModelActions *v);

    bool enabled() const;
    void setEnabled(const bool v);

    bool visible() const;
    void setVisible(const bool v);

    QString section() const;
    void setSection(const QString &v);

    QString status() const;
    void setStatus(const QString &v);

    bool active() const;
    void setActive(const bool v);

    int progress() const;
    void setProgress(const int v);
    bool busy() const;

    QString icon() const;
    void setIcon(const QString &v);

    Fact *bind() const;
    void setBind(Fact *v);

    QString qmlPage() const;
    void setQmlPage(const QString &v);

    QVariantMap opts() const;
    void setOpts(const QVariantMap &v);
    void setOpt(const QString &name, const QVariant &v);

protected:
    FactListModel *m_model;
    FactListModelActions *m_actionsModel;

    bool m_enabled;
    bool m_visible;
    QString m_section;
    QString m_status;
    bool m_active;
    int m_progress;
    QString m_icon;

    QPointer<Fact> m_bind;

    QString m_qmlPage;
    QVariantMap m_opts;

signals:
    void flagsChanged();

    void modelChanged();
    void actionsModelChanged();
    void enabledChanged();
    void visibleChanged();

    void sectionChanged();
    void statusChanged();
    void activeChanged();
    void progressChanged();
    void busyChanged();

    void iconChanged();

    void bindChanged();

    void qmlPageChanged();
    void optsChanged();

    //tree properties propagate
private:
    bool m_parentEnabled;
    void updateParentEnabled();
    bool m_parentVisible;
    void updateParentVisible();
};
//=============================================================================
typedef QList<Fact *> FactList;
//=============================================================================
#endif
