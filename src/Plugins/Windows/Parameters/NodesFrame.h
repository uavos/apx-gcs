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

#include <Fleet/Fleet.h>
#include <TreeModel/FactTreeView.h>
#include <QtWidgets>

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

    void addNodeTools(QMenu *menu, Fact *fact, QString nodeName);
    void updateMenuTitles(QMenu *menu);

    Unit *_unit;

private slots:
    void unitSelected(Unit *unit);
    void updateActions(void);

    void treeContextMenu(const QPoint &pos);

    void aUndo_triggered(void);
    void aDefaults_triggered(void);
};
