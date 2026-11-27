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
#include <QAbstractItemView>
#include <QAccessibleWidget>
#include <QApplication>
#include <QtCore>

#include <App/AppDirs.h>
#include <App/AppGcs.h>
#include <App/AppLog.h>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_CompressHighFrequencyEvents);

    AppGcs app(argc,
               argv,
               "Ground Control",
               QUrl(QStringLiteral("qrc:/Apx/Application/Application.qml")));

#if defined(Q_OS_MACOS) && QT_VERSION < QT_VERSION_CHECK(6, 12, 1)
    // QTBUG-149612: an accessibility client asking an item view for its selected row
    // makes Qt read freed cells and crash - expose item views as plain widgets
    // (installed after QApplication, so it is asked before the Qt widgets factory)
    QAccessible::installFactory([](const QString &, QObject *object) -> QAccessibleInterface * {
        if (auto view = qobject_cast<QAbstractItemView *>(object))
            return new QAccessibleWidget(view);
        return nullptr;
    });
#endif

    int rv = app.exec();
    qInstallMessageHandler(nullptr);
    return rv;
}
