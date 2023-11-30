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

#include <Fact/Fact.h>
#include <QtCore>
Q_DECLARE_LOGGING_CATEGORY(SimLog)

class Simulator : public Fact
{
    Q_OBJECT

public:
    explicit Simulator(Fact *parent = nullptr);
    ~Simulator();

    Fact *f_launch;
    Fact *f_stop;

    Fact *f_type;

    Fact *f_sxpl;

    Fact *f_cmd;

private:
    QStringList xplaneDirs;
    QProcess pShiva;

    QString target_os;
    QString sim_executable;

    bool extract_apxfw();

    void pShivaKill();

    void _launchXplane(QString xplaneDir);

private slots:
    void detectXplane();
    void launch();
    void launchXplane();
    void launchShiva();

    void pShivaFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void pShivaErrorOccurred(QProcess::ProcessError error);
};
