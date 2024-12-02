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

#include "PBase.h"

class PUnit;
class PNode;

class PNodes : public PTreeBase
{
    Q_OBJECT
    Q_PROPERTY(bool upgrading READ upgrading NOTIFY upgradingChanged)

public:
    explicit PNodes(PUnit *parent);

    // set to true if any child node is upgrading
    auto upgrading() const { return m_upgrading; }

private:
    bool m_upgrading{};

private slots:
    void updateUpgrading();

public slots:
    virtual void requestSearch() { _nimp(__FUNCTION__); }

signals:
    void node_available(PNode *node);
    void node_response(PNode *node);

    //properties
    void upgradingChanged();
};
