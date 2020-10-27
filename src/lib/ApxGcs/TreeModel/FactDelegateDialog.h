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
#ifndef FactDelegateDialog_H
#define FactDelegateDialog_H
#include <Fact/Fact.h>
#include <QtWidgets>
//=============================================================================
class FactDelegateDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FactDelegateDialog(Fact *fact, QWidget *parent = 0);
    ~FactDelegateDialog();

    void setWidget(QWidget *w);

    virtual bool aboutToUpload(void) { return true; }
    virtual bool aboutToClose(void) { return true; }

private:
    static QHash<Fact *, FactDelegateDialog *> dlgMap;

protected:
    Fact *fact;
    QWidget *widget;

    QToolBar *toolBar;
    QVBoxLayout *vlayout;

    QAction *aUpload;
    QAction *aUndo;
    QAction *aSep;

    void addAction(QAction *a);

    void closeEvent(QCloseEvent *event);

private slots:
    void doSaveGeometry();
    void doRestoreGeometry();
};
//=============================================================================
#endif
