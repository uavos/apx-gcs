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
#include "UnitMandala.h"
#include <App/App.h>

UnitMandala::UnitMandala(Fact *parent)
    : Fact(parent,
           "mxb",
           "Unit Mandala",
           tr("Unit data tree"),
           Group | FilterModel | ModifiedGroup,
           "hexagon-multiple")
{}

Fact *UnitMandala::fact(const QString &mpath, bool silent)
{
    if (mpath.isEmpty()) {
        qWarning() << "path is empty";
        return {};
    }

    // <sns|ctr|est|cmd>.<module>.<var>

    auto parts = mpath.split('.');
    Fact *p = this;
    uint level = 0;
    for (const auto &s : parts) {
        if (s.isEmpty()) {
            apxMsgW() << "Empty part in path:" << mpath;
            return nullptr;
        }
        level++;
        auto f = p->child(s);
        if (!f) {
            // attemt to create missing fact
            // qDebug() << "creating:" << QString("%1.%2").arg(p->path(this), s);
            if (parts.size() != 3) {
                // only create facts for 3-level paths
                apxMsgW() << "Invalid path:" << mpath;
                return nullptr;
            }
            f = new Fact(p, s);
            if (level < 3) {
                f->setFlags(Group | FilterModel | ModifiedGroup);
            } else {
                f->setFlags(ModifiedGroup);
                _valueFacts.append(f);
                setDescr(QString("[%1]").arg(_valueFacts.size()));
            }
        }
        p = f;
    }
    return p;
}
