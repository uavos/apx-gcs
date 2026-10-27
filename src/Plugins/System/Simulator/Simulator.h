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
#include <QGeoCoordinate>
#include <QtCore>
Q_DECLARE_LOGGING_CATEGORY(SimLog)

class Unit;

class Simulator : public Fact
{
    Q_OBJECT

public:
    explicit Simulator(Fact *parent = nullptr);
    ~Simulator();

    Fact *f_launch;
    Fact *f_stop;

    Fact *f_instances;

    Fact *f_type;

    Fact *f_model;

    Fact *f_sxpl;

    Fact *f_cmd;

    // UDP ports offset between SIL instances
    // instance #0 uses default ports from tcp_ports.h (X-Plane plugin compatible)
    static constexpr const quint16 port_stride{100};

    // f_type option of the built-in flight dynamics, the others are X-Plane installations
    static constexpr const char *fdm_type{"apx-fdm"};
    static constexpr const char *fdm_executable{"apx-fdm"};

    // X-Plane plugin serves the default ports only, apx-fdm is limited by its kMaxUnits
    static constexpr const int max_instances_xplane{1};
    static constexpr const int max_instances_fdm{10};

    static quint16 port_uplink(int idx);
    static quint16 port_sns(int idx);
    static quint16 port_ctr(int idx);

private:
    QStringList xplaneDirs;

    // SIL instances, index is the instance number
    struct Instance
    {
        QProcess *process{};
        QElapsedTimer started;
        int restarts{};
    };
    QList<Instance> _instances;
    bool _user_stop{};

    // built-in flight dynamics (apx-fdm), serves all instances at once
    QProcess *_fdm{};

    // SIL node sometimes crashes right after start (posix platform startup race)
    static constexpr const int startup_timeout_ms{10000};
    static constexpr const int max_restarts{3};

    QString target_os;
    QString sim_executable;

    bool extract_apxfw();

    void pShivaKill();
    void fdmKill();

    void _launchXplane(QString xplaneDir);

    bool isFdm() const;
    void launchFdm();
    void stopFdm();
    bool missionStartPoint(QGeoCoordinate &p, double &hdg) const;
    bool unitStartPoint(Unit *unit, QGeoCoordinate &p, double &hdg) const;

    // the mission is not available until some unit is connected, so the point
    // of the last launch is remembered and used instead of the app defaults
    void saveStartPoint(const QGeoCoordinate &p, double hdg) const;
    bool loadStartPoint(QGeoCoordinate &p, double &hdg) const;

    void updateType();
    void updateModels();
    QString modelFile() const;

    // bundled models and the user ones, the user folder first: its file
    // replaces the bundled one of the same name, nothing is copied
    static QDir userModelsDir();
    static QList<QDir> modelsDirs();

    QDir instanceDir(int idx);
    void startInstance(int idx);
    void stopInstances();
    bool isRunning() const;
    void updateState();

private slots:
    void detectXplane();
    void launch();
    void launchXplane();
    void launchShiva();
    void stopShiva();

    void instanceFinished(int idx, int exitCode, QProcess::ExitStatus exitStatus);
    void instanceErrorOccurred(int idx, QProcess::ProcessError error);
};
