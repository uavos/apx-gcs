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
#include "AppBase.h"
#include <ApxLog.h>
#include <version.h>
#include <QIcon>
#include <QSettings>
//=============================================================================
#ifdef Q_OS_MAC
#include <mach-o/arch.h>
static QByteArray getCpuId()
{
    const NXArchInfo *info = NXGetLocalArchInfo();
    return QByteArray(reinterpret_cast<const char *>(info), sizeof(NXArchInfo));
}
#elif defined Q_OS_LINUX
static void getCpuIdArray(unsigned int *p, unsigned int ax)
{
    __asm __volatile("movl %%ebx, %%esi\n\t"
                     "cpuid\n\t"
                     "xchgl %%ebx, %%esi"
                     : "=a"(p[0]), "=S"(p[1]), "=c"(p[2]), "=d"(p[3])
                     : "0"(ax));
}
static QByteArray getCpuId()
{
    unsigned int info[4] = {0, 0, 0, 0};
    getCpuIdArray(info, 0);
    return QByteArray(reinterpret_cast<const char *>(info), sizeof(info));
}
#elif defined Q_OS_WIN
#include <machine_id.h>
static QByteArray getCpuId()
{
    unsigned int info[4] = {0, 0, 0, 0};
    __cpuid(info, 0);
    return QByteArray(reinterpret_cast<const char *>(info), sizeof(info));
}
#endif
//=============================================================================
AppBase *AppBase::_instance = nullptr;
//=============================================================================
AppBase::AppBase(int &argc, char **argv, const QString &name)
    : QApplication(argc, argv)
{
    _instance = this;

    setObjectName("application");

    QCoreApplication::setOrganizationName("uavos");
    QCoreApplication::setOrganizationDomain("uavos.com");
    QCoreApplication::setApplicationName(name);
    QCoreApplication::setApplicationVersion(VERSION);
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QApplication::setWindowIcon(QIcon("qrc:///icons/uavos-logo.icns"));

    // app constants
    m_version = VERSION;
    apxConsole() << tr("Version").append(":") << m_version;
    m_branch = BRANCH;
    apxConsole() << tr("Branch").append(":") << m_branch;

    m_hash = GIT_HASH;
    m_time = GIT_TIME;
    m_year = GIT_YEAR;

    //identity
    m_hostname = QSysInfo::machineHostName();

    //machine ID
    QByteArray uid = QSysInfo::machineUniqueId();
    if (uid.isEmpty()) {
        uid = getCpuId();
    }
    uid.append(m_hostname.toUtf8());
    //m_machineUID=uid.toHex().toUpper();
    m_machineUID = QCryptographicHash::hash(uid, QCryptographicHash::Sha1).toHex().toUpper();

    //guess user name
    QString sname = "user";
    foreach (QString s, QProcess::systemEnvironment()) {
        if (!s.startsWith("USER"))
            continue;
        s = s.mid(s.indexOf('=') + 1).trimmed();
        if (s.isEmpty())
            break;
        sname = s;
        break;
    }
    m_username = sname;
}
//=============================================================================
QString AppBase::aboutString()
{
    const QString additionalInfo
        = QString("<br>"
                  "%1: <a href='http://docs.uavos.com/'><span "
                  "style='color:#aaf;'>docs.uavos.com</span></a>"
                  "<br>"
                  "%2: <a href='https://uavos.github.io/apx-releases/CHANGELOG.html'><span "
                  "style='color:#aaf;'>view</span></a>"
                  "<br>"
                  "%3: <a href='https://github.com/uavos/apx-releases/releases'><span "
                  "style='color:#aaf;'>download</span></a>"
                  "<br>"
                  "")
              .arg(tr("Documentation"))
              .arg(tr("Changelog"))
              .arg(tr("Releases"));
    QStringList stver;
    stver << tr("Branch '%1' dated %2").arg(branch()).arg(git_time());
    stver << tr("Based on Qt %1 (%2, %3 bit)")
                 .arg(QLatin1String(qVersion()),
                      compilerString(),
                      QString::number(QSysInfo::WordSize));
    stver << tr("Built on %1 %2").arg(QLatin1String(__DATE__), QLatin1String(__TIME__));
    stver << tr("Hash %1").arg(git_hash());
    stver << tr("Machine UID %1 (%2@%3)").arg(machineUID()).arg(username()).arg(hostname());
    const QString s
        = QString("<h3>%1</h3>"
                  "%2"
                  "<hr size=1/>"
                  "%3"
                  "<hr size=1/>"
                  "Copyright 2007-%4<br>"
                  "Aliaksei Stratsilatau &lt;sa@uavos.com&gt;<br>"
                  "All rights reserved.<br/>"
                  "<br/>"
                  "The program is provided AS IS with NO WARRANTY OF ANY KIND, "
                  "INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A "
                  "PARTICULAR PURPOSE.<br/>")
              .arg(QString("%1 %2").arg(QCoreApplication::applicationName()).arg(version()),
                   stver.join("<br/>"),
                   additionalInfo,
                   git_year());
    return s;
}
QString AppBase::compilerString()
{
#if defined(Q_CC_CLANG) // must be before GNU, because clang claims to be GNU too
    QString isAppleString;
#if defined(__apple_build_version__) // Apple clang has other version numbers
    isAppleString = QLatin1String(" (Apple)");
#endif
    return QLatin1String("Clang ") + QString::number(__clang_major__) + QLatin1Char('.')
           + QString::number(__clang_minor__) + isAppleString;
#elif defined(Q_CC_GNU)
    return QLatin1String("GCC ") + QLatin1String(__VERSION__);
#elif defined(Q_CC_MSVC)
    if (_MSC_VER > 1999)
        return QLatin1String("MSVC <unknown>");
    if (_MSC_VER >= 1910)
        return QLatin1String("MSVC 2017");
    if (_MSC_VER >= 1900)
        return QLatin1String("MSVC 2015");
#else
    return QLatin1String("<unknown compiler>");
#endif
}
//=============================================================================
