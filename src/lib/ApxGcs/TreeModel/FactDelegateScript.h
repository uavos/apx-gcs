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

#include "FactDelegateDialog.h"

class NodeScript;
class SourceEdit;

class FactDelegateScript : public FactDelegateDialog
{
    Q_OBJECT
public:
    explicit FactDelegateScript(Fact *fact, QWidget *parent = 0);

protected:
    bool aboutToUpload(void) override;
    bool aboutToClose(void) override;

private:
    NodeScript *nodeScript;

    QAction *aCompile;
    QAction *aLoad;
    QAction *aSave;

    SourceEdit *editor;
    QListWidget *logList;

    QLineEdit *eTitle;

    void launch_vscode();

    //data
private slots:
    void aSave_triggered(void);
    void aLoad_triggered(void);

    void logView_itemClicked(QListWidgetItem *item);

    void updateLog();

    void updateEditorText();
    void updateFactValue();
};
