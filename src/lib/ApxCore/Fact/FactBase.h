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
#pragma once

#include <QtCore>

class Fact;
class FactPropertyBinding;

typedef QList<Fact *> FactList;

class FactBase : public QObject
{
    Q_OBJECT

    Q_PROPERTY(FactBase::Flag treeType READ treeType WRITE setTreeType NOTIFY treeTypeChanged)
    Q_PROPERTY(FactBase::Flags options READ options WRITE setOptions NOTIFY optionsChanged)

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

    Q_PROPERTY(Fact *parentFact READ parentFact WRITE setParentFact NOTIFY parentFactChanged)

    Q_PROPERTY(int size READ size NOTIFY sizeChanged)
    Q_PROPERTY(int num READ num NOTIFY numChanged)

public:
    enum Flag {
        NoFlags = 0,

        //type of item [treeType]
        TypeMask = 0x0000000F,
        Root = 1,
        Group = 2,  // always expandable group
        Action = 3, // action to be triggered and shown as button

        //appearance options [options]
        OptsMask = 0x000FFFF0,
        Section = 1 << 4,        //flat model shows fact as section not folder
        CloseOnTrigger = 1 << 5, //close menu request on trigger
        IconOnly = 1 << 6,       //show only icon button (for actions)
        ShowDisabled = 1 << 7,   //action visible when disabled (for actions)

        FlatModel = 1 << 8,    //child items shown expanded as sections
        DragChildren = 1 << 9, //child items are draggable to change order

        PersistentValue = 1 << 10, //save and restore value in QSettings
        SystemSettings = 1 << 11,  //use default QSettings to store value

        FilterModel = 1 << 12,     //show search filter
        FilterSearchAll = 1 << 13, //search name/title/descr by filters
        FilterExclude = 1 << 14,   //exclude from search by filters

        ModifiedTrack = 1 << 15, //Track modified status
        ModifiedGroup = 1 << 16, //Track children's modified status

        ProgressTrack = 1 << 17, //Track children's progress

        HighlightActive = 1 << 18, //Show highlighted when active

        //data types [dataType]
        DataMask = 0x0FF00000,
        Text = 1 << 20,
        Float = 2 << 20,
        Int = 3 << 20,
        Bool = 4 << 20,
        Enum = 5 << 20,  // value=text of enumStrings (set by text or index or enumValues)
        Count = 6 << 20, // value=size - number of child items

        //complex data types
        MandalaID = 10 << 20, // Mandala ID
        Script = 11 << 20,    // script editor
        Key = 12 << 20,       // keyboard shortcut

        //actions data types
        Apply = 20 << 20,  // green apply button
        Remove = 21 << 20, // red trash button
        Stop = 22 << 20,   // red stop button
    };
    Q_DECLARE_FLAGS(Flags, Flag)
    Q_FLAG(Flags)
    Q_ENUM(Flag)

    explicit FactBase(QObject *parent, const QString &name, FactBase::Flags flags);
    ~FactBase();

    //internal tree
    Q_INVOKABLE void move(int n, bool safeMode = false);

    Q_INVOKABLE int indexInParent() const;
    Q_INVOKABLE int indexOfChild(Fact *item) const;

    Q_INVOKABLE Fact *child(int n) const;
    Q_INVOKABLE Fact *child(const QString &name, Qt::CaseSensitivity cs = Qt::CaseInsensitive) const;

    Q_INVOKABLE QString path(int maxLevel = -1, const QChar pathDelimiter = QChar('.')) const;
    Q_INVOKABLE QString path(const FactBase *root, const QChar pathDelimiter = QChar('.')) const;
    Q_INVOKABLE QStringList pathStringList(int maxLevel = -1) const;

    Q_INVOKABLE void bindProperty(Fact *src, QString name, bool oneway = false);
    Q_INVOKABLE void unbindProperties(Fact *src = nullptr, const QString &name = QString());
    Q_INVOKABLE void unbindProperty(QString name);

    FactList pathList() const;

    const FactList &facts() const;
    const FactList &actions() const;

public slots:
    void remove();
    void removeAll();

signals:
    //tree structure change signals for models
    void itemToBeInserted(int row, FactBase *item);
    void itemInserted(FactBase *item);
    void itemToBeRemoved(int row, FactBase *item);
    void itemRemoved(FactBase *item);
    void itemToBeMoved(int row, int dest, FactBase *item);
    void itemMoved(FactBase *item);

    void actionsUpdated();

    void removed();

private:
    FactList m_facts;
    FactList m_actions;

    QString makeNameUnique(const QString &s);
    QString nameSuffix;

    void addChild(Fact *item);
    void removeChild(Fact *item);
    void moveChild(Fact *item, int n, bool safeMode = false);

    void updateNum();
    void updateSize();
    void updateChildrenNums();

    void updatePath();

    QList<FactPropertyBinding *> _property_binds;

    //-----------------------------------------
    //PROPERTIES
public:
    FactBase::Flag treeType() const;
    void setTreeType(FactBase::Flag v);

    FactBase::Flags options() const;
    void setOptions(FactBase::Flags v);
    void setOption(FactBase::Flag opt, bool v = true);

    Fact *parentFact() const;
    void setParentFact(Fact *v);

    QString name(void) const;
    void setName(const QString &v);

    int size() const;
    int num() const;

protected:
    Flag m_treeType{NoFlags};
    Flags m_options{NoFlags};

    QPointer<QObject> m_parentFact{nullptr};

    QString m_name;
    int m_size{0};
    int m_num{0};

signals:
    void treeTypeChanged();
    void optionsChanged();

    void parentFactChanged();
    void pathChanged();

    void nameChanged();
    void sizeChanged();
    void numChanged();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FactBase::Flags)
