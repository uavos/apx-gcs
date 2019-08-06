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
