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
#ifndef FactTree_H
#define FactTree_H
//=============================================================================
#include <QtCore>
//=============================================================================
class FactBase : public QObject
{
    Q_OBJECT

    Q_PROPERTY(FactBase::Flag treeType READ treeType WRITE setTreeType NOTIFY treeTypeChanged)
    Q_PROPERTY(FactBase::Flags options READ options WRITE setOptions NOTIFY optionsChanged)

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

    Q_PROPERTY(FactBase *parentFact READ parentFact WRITE setParentFact NOTIFY parentFactChanged)

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
        OptsMask = 0x0000FFF0,
        Section = 1 << 4,          //flat model shows fact as section not folder
        CloseOnTrigger = 1 << 5,   //close menu request on trigger
        IconOnly = 1 << 6,         //show only icon button (for actions)
        ShowDisabled = 1 << 7,     //action visible when disabled (for actions)
        FlatModel = 1 << 8,        //child items shown expanded as sections
        DragChildren = 1 << 9,     //child items are draggable to change order
        PersistentValue = 1 << 10, //save and restore value in QSettings
        SystemSettings = 1 << 11,  //use default QSettings to store value
        FilterSearchAll = 1 << 12, //search name/title/descr by filters

        //data types [dataType]
        DataMask = 0x00FF0000,
        Const = 1 << 16,
        Text = 2 << 16,
        Float = 3 << 16,
        Int = 4 << 16,
        Bool = 5 << 16,
        Enum = 6 << 16, // value=value of enumStrings (set by text or index or enumValues)

        //complex data types
        Mandala = 10 << 16, // Mandala ID
        Script = 11 << 16,  // script editor
        Key = 12 << 16,     // keyboard shortcut

        //actions data types
        Apply = 20 << 16,  // green apply button
        Remove = 21 << 16, // red trash button
        Stop = 22 << 16,   //red stop button
    };
    Q_DECLARE_FLAGS(Flags, Flag)
    Q_FLAG(Flags)
    Q_ENUM(Flag)

    explicit FactBase(QObject *parent, const QString &name, FactBase::Flags flags);
    ~FactBase();

    //internal tree
    Q_INVOKABLE void move(int n, bool safeMode = false);

    Q_INVOKABLE int indexInParent() const;
    Q_INVOKABLE int indexOfChild(FactBase *item) const;

    Q_INVOKABLE FactBase *child(int n) const;
    Q_INVOKABLE FactBase *child(const QString &name,
                                Qt::CaseSensitivity cs = Qt::CaseInsensitive) const;

    Q_INVOKABLE QString path(int maxLevel = -1, const QChar pathDelimiter = QChar('.')) const;
    Q_INVOKABLE QStringList pathStringList(int maxLevel = -1) const;

    QList<FactBase *> pathList() const;

    QList<FactBase *> actions() const;

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
    QList<FactBase *> m_children;
    QList<FactBase *> m_actions;

    QString makeNameUnique(const QString &s);
    QString nameSuffix;

    void addChild(FactBase *item);
    void removeChild(FactBase *item);
    void moveChild(FactBase *item, int n, bool safeMode = false);

    void updateNum();
    void updateSize();
    void updateChildrenNums();

    void updatePath();

    //-----------------------------------------
    //PROPERTIES
public:
    FactBase::Flag treeType() const;
    void setTreeType(FactBase::Flag v);

    FactBase::Flags options() const;
    void setOptions(FactBase::Flags v);
    void setOption(FactBase::Flag opt, bool v = true);

    FactBase *parentFact() const;
    void setParentFact(FactBase *v);

    QString name(void) const;
    void setName(const QString &v);

    int size() const;
    int num() const;

protected:
    Flag m_treeType;
    Flags m_options;

    QPointer<FactBase> m_parentFact;

    QString m_name;
    int m_size;
    int m_num;

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
//=============================================================================
#endif
