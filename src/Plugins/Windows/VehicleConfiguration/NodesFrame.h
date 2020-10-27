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
#ifndef NodesFrame_H
#define NodesFrame_H
//=============================================================================
#include <TreeModel/FactTreeView.h>
#include <Vehicles/Vehicles.h>
#include <QtWidgets>
//=============================================================================
class NodesFrame : public QWidget
{
    Q_OBJECT
public:
    NodesFrame(QWidget *parent = nullptr);

private:
    FactTreeWidget *treeWidget;

    QToolBar *toolBar;
    QVBoxLayout *vlayout;

    QAction *aUndo;
    QAction *aDefaults;
    QLabel *lbUavName;

    template<class T = Fact>
    inline QList<T *> selectedItems() const
    {
        QList<T *> list;
        foreach (QModelIndex index, treeWidget->tree->selectionModel()->selectedRows()) {
            Fact *i = index.data(Fact::ModelDataRole).value<Fact *>();
            if (!i)
                continue;
            T *f = qobject_cast<T *>(i);
            if (f)
                list.append(f);
        }
        return list;
    }

    void addNodeTools(QMenu *menu, Fact *fact, QString nodeName);
    void updateMenuTitles(QMenu *menu);

    Vehicle *vehicle;

private slots:
    void vehicleSelected(Vehicle *v);
    void updateActions(void);

    void treeContextMenu(const QPoint &pos);

    void aUndo_triggered(void);
    void aDefaults_triggered(void);
};
//=============================================================================
#endif
