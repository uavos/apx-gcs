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
#include "Simulator.h"
#include "SimMods.h"

#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppGcs.h>
#include <App/AppLog.h>
#include <QDesktopServices>

#include <tcp_ports.h>

APX_LOGGING_CATEGORY(SimLog, "Simulator")

Simulator::Simulator(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Simulator"),
           tr("Software in the loop simulation"),
           Group)
{
    setIcon("fan");
    //parent->insertIntoSection(FactSystem::ApplicationSection,this);

#if defined(Q_OS_MACOS)
    target_os = "darwin";
#elif defined(Q_OS_LINUX)
    target_os = "linux";
#endif

    sim_executable = "sim-" + target_os;

    AppLog::add(SimLog().categoryName(), "sim.txt", true);

    f_launch = new Fact(this, "launch", tr("Start"), tr("Start simulation"), Action | Apply, "play");
    connect(f_launch, &Fact::triggered, this, &Simulator::launch);

    f_stop = new Fact(this, "stop", tr("Stop"), tr("Stop APX app"), Action | Stop, "stop");
    f_stop->setEnabled(false);
    connect(f_stop, &Fact::triggered, this, &Simulator::stopShiva);

    f_instances = new Fact(this,
                           "instances",
                           tr("SIL instances"),
                           tr("Number of SIL nodes to run"),
                           Int | PersistentValue);
    f_instances->setIcon("numeric");
    f_instances->setMin(1);
    f_instances->setMax(32);
    f_instances->setDefaultValue(1);

    // Xplane group
    f_type = new Fact(this, "type", tr("Type"), tr("Simulator type"), Enum | PersistentValue);
    f_type->setIcon("package-variant");

    f_sxpl = new Fact(this, "sxpl", tr("Start XPlane"), tr("Run X-Plane on start"), Bool);
    connect(f_sxpl, &Fact::triggered, this, &Simulator::launchXplane);

    f_cmd = new Fact(this,
                     "cmd",
                     tr("Command"),
                     tr("Command line arguments"),
                     Text | PersistentValue);
    f_cmd->setDefaultValue("--window=200x200 --no_sound --no_joysticks --disable_networking "
                           "--no_aniso_filtering --limited_glsl --no_threaded_ogl");

    // jamming
    new SimMods(this);

    QTimer::singleShot(2000, this, &Simulator::detectXplane);
}
Simulator::~Simulator()
{
    //qDebug() << "kill sim";
    stopInstances();
    pShivaKill();
}

quint16 Simulator::port_uplink(int idx)
{
    return UDP_PORT_AP_UPLINK + idx * port_stride;
}
quint16 Simulator::port_sns(int idx)
{
    return UDP_PORT_AP_SNS + idx * port_stride;
}
quint16 Simulator::port_ctr(int idx)
{
    return UDP_PORT_SIM_CTR + idx * port_stride;
}

void Simulator::pShivaKill()
{
    // synchronous call: a detached killall could kill the instances started right after
    QProcess::execute("killall", QStringList() << sim_executable);
}

bool Simulator::extract_apxfw()
{
    ApxFw *apxfw = AppGcs::apxfw();

    QByteArray data;
    apxfw->loadFirmware("sim", "APX", "application-" + target_os, &data, nullptr);

    if (data.size() <= 0)
        return false;

    QDir dir(AppDirs::firmware().absoluteFilePath("sim"));
    dir.mkpath(".");
    QFile fsim(dir.absoluteFilePath(sim_executable));
    fsim.remove();
    if (!fsim.open(QFile::WriteOnly)) {
        qWarning() << "can't write" << fsim.fileName();
        return false;
    }
    fsim.write(data);
    fsim.close();
    fsim.setPermissions(QFileDevice::ReadOwner | QFileDevice::ExeOwner);
    return true;
}

void Simulator::detectXplane()
{
    QStringList st;
    QFileInfoList files;
    QStringList locations;
    locations << QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    locations << QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.x-plane";

    for (auto i : locations) {
        files = QDir(i, "x-plane_install*.txt").entryInfoList();
        if (!files.isEmpty())
            break;
    }

    for (const auto &fi : files) {
        QString s = fi.baseName();
        int ver = 0;
        QString sv = s.split('_').last();
        if (!sv.isEmpty())
            ver = sv.toUInt();
        if (ver < 9)
            ver = 9;
        //read dir
        QFile file(fi.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            apxConsoleW() << "Can't open file for reading:" << file.fileName();
            continue;
        }
        QString xpath;
        while (!file.atEnd()) {
            QString s = file.readLine().trimmed();
            if (s.isEmpty())
                continue;
            QDir dir(s);
            if (!dir.exists())
                continue;
            xpath = dir.absolutePath();
            xplaneDirs.append(xpath);
            s = QString("XPlane %1").arg(ver);
            if (!st.contains(s)) {
                st.append(s);
                continue;
            }
            int n = 2;
            QString sn = s;
            while (st.contains(sn)) {
                sn = QString("%1 (%2)").arg(s).arg(n++);
            }
            st.append(sn);
        }
        if (xpath.isEmpty()) {
            apxConsoleW() << "Error reading X-Plane config:" << file.fileName() << xpath;
        }
    }
    //qDebug() << st << xplaneDirs;
    f_type->setEnumStrings(st);
    if (st.isEmpty())
        f_type->setEnabled(false);
    else {
        f_type->setEnabled(true);
    }
}

void Simulator::launch()
{
    apxMsg() << tr("Launching simulation").append("...");

    if (f_sxpl->value().toBool())
        launchXplane();

    launchShiva();
}
void Simulator::launchXplane()
{
    apxMsg() << tr("Launching X-Plane").append("...");

    QString xplaneDir;
    xplaneDir = xplaneDirs.value(f_type->value().toInt());
    while (!xplaneDir.isEmpty()) {
        //install xpl plugins
        QDir d(xplaneDir);
        if (d.cd("Resources") && d.cd("plugins")) {
            QDir dir = QDir(AppDirs::res().absoluteFilePath("xplane"), "*.xpl");
            if (dir.isEmpty())
                dir = QDir(AppDirs::libs().absolutePath(), "*.xpl");
            if (dir.isEmpty())
                apxMsgW() << tr("XPL Plugin not found");

            for (const auto &fiSource : dir.entryInfoList()) {
                QString destPath = d.absoluteFilePath(fiSource.fileName());
                QFileInfo fiDest(destPath);

                if (QFile::exists(destPath)) {
                    if (fiDest.lastModified() == fiSource.lastModified())
                        continue;

                    QFile::remove(destPath);
                }

                if (QFile::copy(fiSource.absoluteFilePath(), fiDest.absoluteFilePath())) {
                    QFile fDest(destPath);
                    if (!fDest.open(QFile::ReadOnly)) {
                        apxMsgW() << tr("XPL Plugin error").append(":") << destPath;
                    } else {
                        fDest.setFileTime(fiSource.lastModified(), QFile::FileModificationTime);
                        apxMsg() << tr("XPL Plugin installed").append(":") << destPath;
                    }
                } else {
                    apxMsgW() << tr("XPL Plugin error").append(":") << destPath;
                }
            }

            // now check if there are duplicates found
            auto xplFiles = QDir(d.absolutePath(), "ApxSIL_*.xpl").entryInfoList();
            if (xplFiles.size() > 1) {
                apxMsgW() << tr("Multiple XPL plugins found in X-Plane plugins directory. Please "
                                "remove duplicates.");
            }
        }

        break;
    }
    if (!xplaneDir.isEmpty())
        _launchXplane(xplaneDir);
}

QDir Simulator::instanceDir(int idx)
{
    QDir dir(AppDirs::firmware().absoluteFilePath(QString("sim/units/%1").arg(idx)));
    dir.mkpath(".");
    return dir;
}

void Simulator::startInstance(int idx)
{
    while (_instances.size() <= idx)
        _instances.append(Instance{});

    Instance &inst = _instances[idx];
    QProcess *p = inst.process;
    if (!p) {
        p = new QProcess(this);
        inst.process = p;
        connect(p,
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this,
                [this, idx](int exitCode, QProcess::ExitStatus exitStatus) {
                    instanceFinished(idx, exitCode, exitStatus);
                });
        connect(p, &QProcess::errorOccurred, this, [this, idx](QProcess::ProcessError error) {
            instanceErrorOccurred(idx, error);
        });
    }

    if (p->state() != QProcess::NotRunning)
        return;

    QDir dir = instanceDir(idx);

    p->setProgram(AppDirs::firmware().absoluteFilePath("sim/" + sim_executable));
    p->setWorkingDirectory(dir.absolutePath());

    // each instance listens on its own UDP ports
    QStringList args;
    args << "-u" << QString::number(port_uplink(idx));
    args << "-s" << QString::number(port_sns(idx));
    args << "-x" << QString::number(port_ctr(idx));
    p->setArguments(args);

    // node config is stored in $HOME/.config - separate it per instance
    // instance #0 keeps the user HOME to preserve the existing SIM unit config
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (idx > 0)
        env.insert("HOME", dir.absolutePath());
    p->setProcessEnvironment(env);

    // console output goes to a per-instance log file
    p->setProcessChannelMode(QProcess::MergedChannels);
    p->setStandardOutputFile(dir.absoluteFilePath("sim.log"), QIODevice::Truncate);

    qCDebug(SimLog) << "SIL" << idx << p->program() << args << dir.absolutePath();

    _user_stop = false;
    inst.started.start();
    p->start();
    updateState();
}

void Simulator::stopInstances()
{
    for (auto &inst : _instances) {
        QProcess *p = inst.process;
        if (!p)
            continue;
        disconnect(p, nullptr, this, nullptr);
        if (p->state() != QProcess::NotRunning) {
            p->terminate();
            if (!p->waitForFinished(1000))
                p->kill();
        }
        p->deleteLater();
    }
    _instances.clear();
}

bool Simulator::isRunning() const
{
    for (const auto &inst : _instances) {
        if (inst.process && inst.process->state() != QProcess::NotRunning)
            return true;
    }
    return false;
}

void Simulator::updateState()
{
    bool running = isRunning();
    f_launch->setEnabled(!running);
    f_stop->setEnabled(running);
}

void Simulator::launchShiva()
{
    apxMsg() << tr("Launching SIL").append("...");

    // stop all instances, including stray ones
    stopInstances();
    pShivaKill();

    if (!extract_apxfw()) {
        apxMsgW() << tr("Simulation firmware unavailable");
        updateState();
        return;
    }

    int count = f_instances->value().toInt();
    if (count < 1)
        count = 1;

    for (int i = 0; i < count; ++i) {
        startInstance(i);
    }
}

void Simulator::stopShiva()
{
    _user_stop = true;
    for (const auto &inst : _instances) {
        if (inst.process && inst.process->state() != QProcess::NotRunning)
            inst.process->terminate();
    }
}

void Simulator::_launchXplane(QString xplaneDir)
{
    //mount image
    QFileInfoList fiList;
    fiList = QDir(xplaneDir, "*.img").entryInfoList();
    if (fiList.isEmpty())
        fiList = QDir(xplaneDir, "*.iso").entryInfoList();

    while (!fiList.isEmpty()) {
        QString imgFile = fiList.first().absoluteFilePath();
#if defined Q_OS_MACOS
        if (QProcess::execute("hdiutil", QStringList() << "attach" << imgFile) == 0)
            break;
#elif defined Q_OS_LINUX
        QDir("/media/cdrom").mkpath(".");
        if (QProcess::execute("mount",
                              QStringList() << imgFile << "/media/cdrom"
                                            << "-o loop,ro")
            == 0)
            break;
#endif
        apxMsgW() << tr("Failed to mount X-Plane image") << imgFile << "(/media/cdrom)";
        break;
    }

    //launch xplane
    QString xplaneApp("X-Plane");
#if defined Q_OS_MACOS
    xplaneApp.append(".app");
    xplaneApp = QDir(xplaneApp).filePath("Contents/MacOS/X-Plane");
#elif defined Q_OS_WIN
    xplaneApp.append(".exe");
#else
    xplaneApp.append("-x86_64");
#endif
    /*QUrl xplaneUrl(QUrl::fromLocalFile(xplaneDir + "/" + xplaneApp));
    if (!QDesktopServices::openUrl(xplaneUrl)) {
        apxMsgW() << tr("Failed to start X-Plane app") << xplaneUrl;
    }*/
    QStringList args = f_cmd->text().split(' ');
    QString exe = QDir(xplaneDir).absoluteFilePath(xplaneApp);
    if (!QProcess::startDetached(exe, args)) {
        apxMsgW() << tr("Failed to start X-Plane app") << exe;
    }
}

void Simulator::instanceFinished(int idx, int exitCode, QProcess::ExitStatus exitStatus)
{
    apxMsg() << tr("SIL simulation stopped").append(QString(" #%1").arg(idx));
    updateState();

    if (idx >= _instances.size())
        return;
    Instance &inst = _instances[idx];

    // restart requested by the node
    if (exitStatus == QProcess::NormalExit && exitCode == 7) {
        startInstance(idx);
        return;
    }

    // crashed on startup - retry
    if (!_user_stop && exitStatus == QProcess::CrashExit
        && inst.started.elapsed() < startup_timeout_ms && inst.restarts < max_restarts) {
        inst.restarts++;
        apxMsgW() << tr("SIL crashed on start, restarting")
                         .append(QString(" #%1 (%2)").arg(idx).arg(inst.restarts));
        startInstance(idx);
    }
}
void Simulator::instanceErrorOccurred(int idx, QProcess::ProcessError error)
{
    QProcess *p = _instances.value(idx).process;
    apxMsgW() << QString("SIL #%1:").arg(idx) << (p ? p->errorString() : QString()) << error;
    updateState();
}
