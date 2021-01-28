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
#ifndef Updater_H
#define Updater_H
#include <QtCore>

#include <ApxMisc/DelayedEvent.h>
#include <Fact/Fact.h>
#ifdef Q_OS_MAC
#include <SparkleAutoUpdater.h>
#endif
#ifdef Q_OS_LINUX
#include <AppImageAutoUpdater.h>
#endif
//=============================================================================
class Updater : public Fact
{
    Q_OBJECT
public:
    Updater(Fact *parent = nullptr);

    Fact *f_auto;
    Fact *f_check;

private:
#ifdef Q_OS_MAC
    std::unique_ptr<SparkleAutoUpdater> m_impl{};
#endif
#ifdef Q_OS_LINUX
    AppImageAutoUpdater *m_impl{};
#endif
    void initUpdaterImpl();

private slots:
    void updateAuto();

public slots:
    void check();
    void checkInBackground();
};
//=============================================================================
#endif
