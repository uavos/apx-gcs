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
#include <QtCore>
//============================================================================
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Ground Control Software by UAVOS (C) Aliaksei Stratsilatau "
                                     "<sa@uavos.com>. Plugin test utility.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("plugins", "Plugin[s] to load and test.");
    parser.process(*qApp);

    const QStringList args = parser.positionalArguments();

    for (auto fname : args) {
        QLibrary lib(fname);
        if (!lib.load()) {
            const char *err = lib.errorString().toUtf8();
            qFatal("%s", err);
        }
    }

    return 0;
}
//============================================================================
