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
#ifndef MandalaTree_H
#define MandalaTree_H
//=============================================================================
#include <QtCore>
namespace Mandala {
//=============================================================================
class MandalaTree : public QAbstractListModel
{
    Q_OBJECT

public:
    typedef quint16 id_t;

    enum {                                          //id masks
        MANDALA_INDEX_MASK_FIELDS = ((1 << 5) - 1), // 5bit (32 fields)
        MANDALA_INDEX_MASK_GRP1 = ((1 << 10) - 1),  // 10-5=5bit (32 groups)
        MANDALA_INDEX_MASK_GRP2 = ((1 << 12) - 1),  // 12-10=2bit (4 types) [sns,ctr,state,cmd]
    };

    //root
    explicit MandalaTree(QObject *parent = 0);
    //any
    explicit MandalaTree(
        MandalaTree *parent, id_t id, QString name, QString descr, QString alias = QString());

    ~MandalaTree();

    //internal tree
    MandalaTree *child(int n);
    MandalaTree *parentItem() const;
    int num() const;

    //access child by name or index
    Q_INVOKABLE MandalaTree *at(const QString &s);
    Q_INVOKABLE MandalaTree *at(id_t idx);

    //deep search
    QList<MandalaTree *> fields();
    Q_INVOKABLE MandalaTree *find(QString s);
    Q_INVOKABLE MandalaTree *find(id_t id);
    Q_INVOKABLE MandalaTree *fieldByPath(QString s);
    Q_INVOKABLE MandalaTree *fieldByAlias(QString s);

    Q_INVOKABLE MandalaTree *field(QString s);

    Q_INVOKABLE QString path(int lev = 0) const;

    //item type and status
    Q_INVOKABLE bool isField(void) const;
    Q_INVOKABLE bool isFieldsGroup(void) const;
    Q_INVOKABLE bool isRoot(void) const;

    //value
    Q_INVOKABLE QVariant &operator=(const QVariant &v)
    {
        setValue(v);
        return m_value;
    }
    Q_INVOKABLE operator QVariant() const { return m_value; }

    //bind to another item
    virtual void bind(MandalaTree *f);

    //tree structure
    Q_INVOKABLE void addItem(MandalaTree *child);
    Q_INVOKABLE void removeItem(MandalaTree *child);

    enum MandalaListModelRoles {
        ItemRole = Qt::UserRole + 1,
        NameRole,
        ValueRole,
        DescrRole,
    };
    enum { //model columns
        MANDALA_ITEM_COLUMN_NAME = 0,
        MANDALA_ITEM_COLUMN_VALUE,
        MANDALA_ITEM_COLUMN_DESCR,
    };

protected:
    //ListModel override
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QHash<int, QByteArray> roleNames() const;
    virtual QVariant headerData(int section,
                                Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

private:
    QList<MandalaTree *> m_items;

    MandalaTree *m_parentItem;
    MandalaTree *m_bindItem;

    static int maxLevel;

public slots:
    virtual void clear(void);
    virtual void reset(void);
signals:
    void structChanged(MandalaTree *item);

    //-----------------------------------------
    //PROPERTIES
public:
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString descr READ descr CONSTANT)
    Q_PROPERTY(QString alias READ alias CONSTANT)

    Q_PROPERTY(quint16 id READ id CONSTANT)
    Q_PROPERTY(int level READ level CONSTANT)

    Q_PROPERTY(bool used READ used NOTIFY usedChanged)
    Q_PROPERTY(QString valueText READ valueText NOTIFY valueChanged)

    Q_PROPERTY(int size READ size NOTIFY sizeChanged)

public:
    virtual QVariant value(void) const;
    virtual bool setValue(const QVariant &v);

    virtual QString name(void) const;
    virtual QString descr(void) const;
    virtual QString alias(void) const;

    quint16 id(void) const;
    int level(void) const;

    virtual bool used(void) const;

    virtual QString valueText(void) const;

    int size() const;

protected:
    QVariant m_value;

    id_t m_id;
    int m_level;

    QString m_name;
    QString m_descr;
    QString m_alias;

signals:
    void valueChanged(MandalaTree *item);
    void usedChanged(MandalaTree *item);
    void sizeChanged(MandalaTree *item);
};
} // namespace Mandala
//=============================================================================
#endif
