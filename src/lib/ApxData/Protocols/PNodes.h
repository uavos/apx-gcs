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

class PVehicle;
class PNode;

class PNodes : public PTreeBase
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit PNodes(PVehicle *parent);

    bool busy() const { return m_busy; }

private:
    bool m_busy{};

protected:
    void setBusy(bool v);

public slots:
    virtual void requestSearch() { _nimp(__FUNCTION__); }
    virtual void cancelRequests() { _nimp(__FUNCTION__); }

signals:
    void node_available(PNode *node);
    void node_response(PNode *node);

    void busyChanged();
};
