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

#if defined(Q_OS_MAC)
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
    connect(f_stop, &Fact::triggered, &pShiva, &QProcess::terminate);

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

    //shiva
    pShiva.setProgram(AppDirs::firmware().absoluteFilePath("sim/" + sim_executable));
    connect(&pShiva,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &Simulator::pShivaFinished);
    connect(&pShiva, &QProcess::errorOccurred, this, &Simulator::pShivaErrorOccurred);

    QTimer::singleShot(2000, this, &Simulator::detectXplane);
}
Simulator::~Simulator()
{
    //qDebug() << "kill sim";
    disconnect(&pShiva, nullptr, nullptr, nullptr);
    pShiva.terminate();
    pShiva.waitForFinished(1000);
    pShivaKill();
}

void Simulator::pShivaKill()
{
    if (QProcess::startDetached("killall", QStringList() << sim_executable) == 0) {
        //apxMsgW() << tr("SIL simulation session restart");
    }
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

    foreach (QFileInfo fi, files) {
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

            for (auto const &fiSource : dir.entryInfoList()) {
                QString destPath = d.absoluteFilePath(fiSource.fileName());
                QFileInfo fiDest(destPath);

                if (QFile::exists(destPath)) {
                    if (fiDest.lastModified() == fiSource.lastModified())
                        continue;

                    QFile::remove(destPath);
                }

                if (QFile::copy(fiSource.absoluteFilePath(), fiDest.absoluteFilePath())) {
                    QFile fDest(destPath);
                    fDest.open(QFile::ReadOnly);
                    fDest.setFileTime(fiSource.lastModified(), QFile::FileModificationTime);

                    apxMsg() << tr("XPL Plugin installed").append(":") << destPath;
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
void Simulator::launchShiva()
{
    apxMsg() << tr("Launching SIL").append("...");

    //launch shiva
    pShiva.kill();
    pShivaKill();

    if (!extract_apxfw()) {
        apxMsgW() << tr("Simulation firmware unavailable");
    } else {
        /*QStringList args;
    if (f_oDLHD->value().toBool())
        args << "-a";
    if (f_oINS->value().toBool())
        args << "-i";
    if (f_oNoise->value().toBool())
        args << "-n";
    args << "-s"
         << "-u";
    pShiva.setArguments(args);*/

        f_launch->setEnabled(false);
        f_stop->setEnabled(true);

        pShiva.start();
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
#if defined Q_OS_MAC
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
#if defined Q_OS_MAC
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

void Simulator::pShivaFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
    apxMsg() << tr("SIL simulation stopped");
    f_launch->setEnabled(true);
    f_stop->setEnabled(false);
    if (exitCode == 7)
        launch();
}
void Simulator::pShivaErrorOccurred(QProcess::ProcessError error)
{
    apxMsgW() << pShiva.errorString() << error;
}
