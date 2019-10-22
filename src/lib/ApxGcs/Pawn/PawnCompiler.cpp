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
#include "PawnCompiler.h"
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
PawnCompiler::PawnCompiler(Fact *fact)
    : QObject(fact)
    , fact(fact)
{
    //const uint vm_data_size=1024;
    tmpFile.open();
    outFileName = tmpFile.fileName() + "-compiled.amx";
    pawncc.setProgram(QCoreApplication::applicationDirPath() + "/pawncc");
    pawncc.setProcessChannelMode(QProcess::MergedChannels);
}
//=============================================================================
bool PawnCompiler::compile()
{
    m_outData.clear();
    m_error = false;
    tmpFile.resize(0);
    tmpFile.flush();
    QFile::remove(outFileName);
    QString src = fact->value().toString();
    if (src.trimmed().isEmpty()) {
        emit compiled();
        return true;
    }
    //qDebug()<<src;
    QTextStream s(&tmpFile);
    s << src;
    s.flush();
    tmpFile.flush();
    //process
    QStringList args;
    args << "-d0";
    args << "-O3";
    args << "-v2";
    args << "-r";
    //fill mandala constants
    Vehicle *vehicle = fact->findParent<Vehicle *>();
    if (vehicle) {
        foreach (VehicleMandalaFact *f, vehicle->f_mandala->allFacts)
            args << "f_" + f->name() + "=" + QString::number(f->id());
        foreach (QString name, vehicle->f_mandala->constants.keys())
            args << name + "=" + vehicle->f_mandala->constants.value(name).toString();
    }
    args << "-i" + AppDirs::res().absoluteFilePath("scripts/pawn/include");
    args << "-i" + AppDirs::scripts().absoluteFilePath("pawn");
    args << "-i" + AppDirs::scripts().absoluteFilePath(".");
    args << "-o" + outFileName;
    args << tmpFile.fileName();
    pawncc.setArguments(args);
    pawncc.start();
    bool rv = true;
    if (!pawncc.waitForFinished()) {
        apxMsgW() << "pawncc error:" << pawncc.errorString();
        rv = false;
    } else if (pawncc.exitCode() != 0)
        rv = false;
    pawncc_log = pawncc.isOpen() ? pawncc.readAll() : QString();
    if (!rv)
        pawncc_log.append("\n\n" + pawncc.errorString());

    //log file
    QTemporaryFile logFile;
    logFile.setFileTemplate(QDir::tempPath() + "/pawncc_log");
    logFile.setAutoRemove(false);
    if (logFile.open()) {
        QTextStream s(&logFile);
        s << pawncc.program() + " " + pawncc.arguments().join(' ');
        s << "\n\n";
        s << pawncc_log;
        s.flush();
        logFile.flush();
        logFile.close();
    }

    if (rv) {
        //read out data
        QFile file(outFileName);
        if (file.open(QFile::ReadOnly)) {
            m_outData = file.readAll();
        }
    }
    m_error = !rv;
    emit compiled();
    //qDebug()<<"compile"<<rv<<m_outData.size();//<<getLog();
    return rv;
}
//=============================================================================
QString PawnCompiler::getLog()
{
    return pawncc.isOpen() ? pawncc_log : QString();
}
//=============================================================================
const QByteArray &PawnCompiler::outData() const
{
    return m_outData;
}
//=============================================================================
bool PawnCompiler::error()
{
    return m_error;
}
//=============================================================================
