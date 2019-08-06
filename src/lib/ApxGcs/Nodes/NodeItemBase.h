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
#ifndef NodeItemBase_H
#define NodeItemBase_H
//=============================================================================
#include "NodesBase.h"
#include <QtCore>
//=============================================================================
class NodeItemBase : public NodesBase
{
    Q_OBJECT
    Q_PROPERTY(bool dictValid READ dictValid WRITE setDictValid NOTIFY dictValidChanged)
    Q_PROPERTY(bool upgrading READ upgrading WRITE setUpgrading NOTIFY upgradingChanged)

public:
    explicit NodeItemBase(Fact *parent,
                          const QString &name,
                          const QString &title,
                          Fact::Flags flags);

    //override
    virtual QVariant data(int col, int role) const;
    bool lessThan(Fact *rightFact) const;

private:
    static QStringList sortNames;
    int progress_s;

public slots:
    void updateDictValid();
    void updateUpgrading();
    void updateProgress();

    //---------------------------------------
    // PROPERTIES
public:
    bool dictValid() const;
    void setDictValid(const bool &v);
    bool upgrading() const;
    void setUpgrading(const bool &v);

protected:
    bool m_dictValid;
    bool m_upgrading;

signals:
    void dictValidChanged();
    void upgradingChanged();
};
//=============================================================================
#endif
