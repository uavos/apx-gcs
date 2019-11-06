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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "RunGuard.h"
#include <App/AppDirs.h>
#include <App/AppGcs.h>
#include <App/AppLog.h>
#include <QApplication>
#include <QGLWidget>
#include <QProcessEnvironment>
#include <QQuickStyle>
#include <QStyleFactory>
#include <QtCore>
#include <QtQuick>
//============================================================================
void checkPaths();
//============================================================================
/*void crash_handler(int sig) {
 fprintf(stdout,"\nCRASH\n");
 if(serial1)delete serial1;
 if(serial2)delete serial2;
 exit(-1);
}*/
//============================================================================
int main(int argc, char *argv[])
{
#ifdef Q_OS_MAC
    //qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "false");
    //qputenv("QT_SCALE_FACTOR", "1");

    //QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    //QApplication::setAttribute(Qt::AA_NativeWindows);
#endif

    //app initialization
    //QApplication::setAttribute(Qt::AA_PluginApplication); //macos faster?
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);

    //high DPI scaling
    //QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, false);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, false);
    //QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_Use96Dpi);

    //performance graphics
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication::setAttribute(Qt::AA_CompressHighFrequencyEvents);
    //QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);

    //qputenv("QSG_RENDER_LOOP", "basic");

    //signal(SIGSEGV, crash_handler); //installing signal handler
    //signal(SIGTERM, crash_handler); //installing signal handler

    //qputenv("QT_QUICK_CONTROLS_STYLE", "Android");

    /*QString qmlcache = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first();
    QDir qmlCacheDir(qmlcache);
    qDebug() << "removing" << qmlCacheDir.absolutePath();
    qmlCacheDir.removeRecursively();*/

    //QApplication::setGraphicsSystem(QLatin1String("opengl"));
    //QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    //qputenv("QSG_INFO","1");
    //qputenv("QML_FIXED_ANIMATION_STEP","1");
    //qputenv("QT_QPA_EGLFS_FORCE888","1");

    /*QGLFormat f = QGLFormat::defaultFormat();
    f.setSwapInterval(0);
    f.setSampleBuffers(true);
    f.setSamples(4); //8);
    f.setStencilBufferSize(8);
    f.setDepthBufferSize(8);
    QGLFormat::setDefaultFormat(f);*/

    AppGcs app(argc,
               argv,
               "Ground Control",
               QUrl(QStringLiteral("qrc:/Apx/Application/Application.qml")));

    //check instances
    /*if(!QSettings().value("multipleInstances").toBool()){
    RunGuard guard("instance.gcs.uavos.com");
    if(!guard.tryToRun()){
      apxConsoleW()<<QObject::tr("Another application instance is running");
      if(!QSettings().value("multipleInstances").toBool()){
        return 0;
      }
    }
  }


  // directories..
  checkPaths();

  if(QCoreApplication::arguments().contains("-x"))
    QSettings().setValue("maximized", true);

*/

    int rv = app.exec();
    qInstallMessageHandler(nullptr);
    return rv;
}
/*catch (std::exception & e)
{
  fprintf(stdout,"\nCATCH1\n");
  delete serial1;
  delete serial2;
}
catch (...)
{
  fprintf(stdout,"\nCATCH2\n");
  // someone threw something undecypherable
  delete serial1;
  delete serial2;
}*/
//============================================================================
//============================================================================
void linkFiles(QDir src, QDir dest)
{
    if (!dest.exists())
        dest.mkpath(".");
    foreach (QFileInfo fi, src.entryInfoList(QDir::Files)) {
        QFileInfo fiDest(dest.filePath(fi.fileName()));
        if (fiDest.exists() || fiDest.isSymLink()) {
            if (fiDest.isSymLink() && fiDest.symLinkTarget() == fi.absoluteFilePath())
                continue;
            QFile::remove(fiDest.absoluteFilePath());
        }
        QFile::link(fi.absoluteFilePath(), fiDest.absoluteFilePath());
        apxConsole() << QObject::tr("Link updated").append(": ") << fiDest.absoluteFilePath();
    }
}
void linkDir(QDir src, QDir dest, QString suffix)
{
    if (!dest.exists())
        dest.mkpath(".");
    QFileInfo fiDest(dest.filePath(src.dirName() + suffix));
    if (fiDest.exists() || fiDest.isSymLink()) {
        if (fiDest.isSymLink() && fiDest.symLinkTarget() == src.absolutePath())
            return;
        QFile::remove(fiDest.absoluteFilePath());
    }
    QFile::link(src.absolutePath(), fiDest.absoluteFilePath());
    apxConsole() << QObject::tr("Link updated").append(": ") << fiDest.absoluteFilePath();
}
bool copyPath(QDir src, QDir dest, bool deleteOnCopy = false)
{
    if (!src.exists())
        return false;
    if (!dest.exists())
        dest.mkpath(".");

    //qDebug()<<"copyPath"<<src.path()<<dest.path();
    foreach (QFileInfo fi, src.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        dest.mkpath(fi.fileName());
        if (!src.cd(fi.fileName()))
            return false;
        if (!dest.cd(fi.fileName()))
            return false;
        if (!copyPath(src, dest, deleteOnCopy))
            return false; //recursion
        src.cdUp();
        dest.cdUp();
    }
    //qDebug()<<"files"<<src.absolutePath();
    uint fcnt = 0;
    foreach (QFileInfo fi, src.entryInfoList(QDir::Files)) {
        //qDebug()<<"file"<<fi.absoluteFilePath();
        const QString fndest(dest.absoluteFilePath(fi.fileName()));
        if (QFile::exists(fndest))
            continue;
        //QFile::copy(fi.absoluteFilePath(),fndest);
        //if(deleteOnCopy)QFile::remove(fi.absoluteFilePath());
        if (deleteOnCopy)
            QFile::rename(fi.absoluteFilePath(), dest.absoluteFilePath(fi.fileName()));
        else
            QFile::copy(fi.absoluteFilePath(), fndest);
        fcnt++;
    }
    // Possible race-condition mitigation?
    dest.refresh();
    if (!dest.exists())
        return false;
    if (fcnt > 0) {
        apxConsole() << QObject::tr("Updated %1 user files in %2").arg(fcnt).arg(dest.absolutePath());
    }
    return true;
}
bool movePath(QDir src, QDir dest)
{
    if ((!dest.exists()) || dest.isEmpty()) {
        dest.mkpath(".");
        QDir d(dest);
        d.cdUp();
        d.rmdir(dest.dirName());
        if (QDir().rename(src.absolutePath(), dest.absolutePath())) {
            apxConsole() << QObject::tr("Moved files: '%1' to '%2'").arg(src.path()).arg(dest.path());
            return true;
        }
    }
    if (copyPath(src, dest, true)) {
        if (src.rmpath("."))
            return true;
    }
    return false;
}
void fixDeprecatedPath(QString srcPath, QDir dest)
{
    QDir src(QDir(QDir::home().absoluteFilePath(".gcu")).absoluteFilePath(srcPath));
    if (!src.exists())
        return;
    if (movePath(src, dest))
        return;
    apxConsoleW() << QObject::tr("Deprecated user path").append(": ").append(src.absolutePath());
}
void checkPaths()
{
    //qDebug()<<"checkPaths";

    if (!AppDirs::db().exists())
        AppDirs::db().mkpath(".");

    //fix old paths
    fixDeprecatedPath("config/uav.conf.d", AppDirs::configs());
    fixDeprecatedPath("flightplans", AppDirs::missions());
    //fixDeprecatedPath("maps",QDir(AppDirs::maps().absoluteFilePath("google-tiles")));
    //fixDeprecatedPath("nodes",AppDirs::nodes());
    fixDeprecatedPath("plugins", AppDirs::userPlugins());
    fixDeprecatedPath("scripts", AppDirs::scripts());
    //fixDeprecatedPath("data",AppDirs::telemetry());

    // link sample files
    linkFiles(AppDirs::res().absoluteFilePath("nodes/sample-configs"), AppDirs::configs());
    linkFiles(AppDirs::res().absoluteFilePath("missions"), AppDirs::missions());
    //linkFiles(AppDirs::res().absoluteFilePath("telemetry"),AppDirs::telemetry());
    linkDir(AppDirs::res().absoluteFilePath("scripts/pawn"), AppDirs::scripts(), "-examples");

    //warn if exists old dir
    QDir src(QDir::home().absoluteFilePath(".gcu"));
    if (!src.exists())
        return;
    if (src.isEmpty() && src.rmdir("."))
        return;
    apxConsoleW() << QObject::tr("Deprecated storage directory")
                         .append(": ")
                         .append(src.absolutePath());
    apxConsoleW() << QObject::tr("New storage directory")
                         .append(": ")
                         .append(AppDirs::user().absolutePath());
}
//============================================================================
