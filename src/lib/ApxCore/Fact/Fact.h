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
#include "FactAction.h"
#include <QColor>
class FactSystem;
//=============================================================================
class Fact : public FactData
{
    Q_OBJECT

    Q_PROPERTY(FactListModel *model READ model NOTIFY modelChanged)
    Q_PROPERTY(FactListModelActions *actionsModel READ actionsModel NOTIFY actionsModelChanged)

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

    Q_PROPERTY(QString section READ section WRITE setSection NOTIFY sectionChanged)
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(int progress READ progress WRITE setProgress NOTIFY progressChanged)

    Q_PROPERTY(QString icon READ icon NOTIFY iconChanged)

    Q_PROPERTY(Fact *link READ link NOTIFY linkChanged)
    Q_PROPERTY(QString qmlPage READ qmlPage NOTIFY qmlPageChanged)

    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    explicit Fact(FactBase *parent,
                  const QString &name,
                  const QString &title,
                  const QString &descr,
                  FactBase::Flags flags = FactBase::Flags(NoFlags));

    Q_INVOKABLE QByteArray hash() const;

    Q_INVOKABLE QVariant findValue(const QString &namePath);

    Q_INVOKABLE Fact *findChild(const QString &factName, bool exactMatch = true) const;
    Q_INVOKABLE Fact *childByTitle(const QString &factTitle) const;

    Q_INVOKABLE QString titlePath(const QChar pathDelimiter = QChar('/')) const;
    Q_INVOKABLE Fact *factByTitlePath(const QString &spath) const;

    Q_INVOKABLE void setValues(const QVariantMap &values);

    Q_INVOKABLE virtual QJsonObject toJson(bool array = false) const;
    Q_INVOKABLE virtual void fromJson(const QJsonObject &jso);

    QVariant userData;

    virtual bool lessThan(Fact *rightFact) const; //sorting helper

    //actions
    QList<FactAction *> actions;

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

    void bind(FactData *fact) override;

private:
    QPointer<Fact> bindedFact;

    QString pTitle() const;

protected:
    bool blockNotify;

public slots:
    virtual void trigger(void); //execute fact event (onClick)
    void requestDefaultMenu();
    void requestMenu(QVariantMap opts = QVariantMap());

signals:
    void triggered();
    void actionTriggered();
    void menuRequested(QVariantMap opts);
    void menuBack();

    //---------------------------------------
    // PROPERTIES
public:
    FactListModel *model() const;
    void setModel(FactListModel *v);

    FactListModelActions *actionsModel() const;
    void setActionsModel(FactListModelActions *v);

    bool enabled() const;
    void setEnabled(const bool &v);

    bool visible() const;
    void setVisible(const bool &v);

    QString section() const;
    void setSection(const QString &v);

    QString status() const;
    void setStatus(const QString &v);

    bool active() const;
    void setActive(const bool &v);

    int progress() const;
    void setProgress(const int &v);

    QString icon() const;
    void setIcon(const QString &v);

    Fact *link() const;
    void setLink(Fact *v);

    QString qmlPage() const;
    void setQmlPage(const QString &v);

    QColor color() const;
    void setColor(const QColor &v);

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
    Fact *m_link;
    QString m_qmlPage;
    QColor m_color;

signals:
    void modelChanged();
    void actionsModelChanged();
    void enabledChanged();
    void visibleChanged();

    void sectionChanged();
    void statusChanged();
    void activeChanged();
    void progressChanged();

    void iconChanged();
    void linkChanged();
    void qmlPageChanged();
    void colorChanged();

    //tree properties propagate
private:
    bool m_parentEnabled;
    void updateParentEnabled();
    bool m_parentVisible;
    void updateParentVisible();
};
//=============================================================================
#endif
