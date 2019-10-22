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
#include "TelemetryShare.h"
#include "LookupTelemetry.h"
#include "Telemetry.h"
#include "TelemetryReader.h"

#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <Sharing/TelemetryXmlExport.h>
#include <Sharing/TelemetryXmlImport.h>
#include <Vehicles/Vehicle.h>
//=============================================================================
TelemetryShare::TelemetryShare(Telemetry *telemetry, Fact *parent)
    : Share(parent,
            tr("Telemetry"),
            "telemetry",
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
            QStringList() << "csv")
    , telemetry(telemetry)
{
    QString sect = tr("Queue");
    qimp = new QueueJob(this, "qimp", tr("Import queue"), "", new TelemetryXmlImport());
    qimp->setIcon("import");
    qimp->setSection(sect);
    connect(qimp, &Fact::progressChanged, this, &TelemetryShare::updateProgress);
    connect(qimp, &Fact::statusChanged, this, &TelemetryShare::updateStatus);

    connect(qimp, &QueueJob::finished, this, [this](Fact *, QVariantMap result) {
        if (result.value("key").toULongLong()) {
            emit imported("", result.value("title").toString());
        }
    });
    connect(qimp, &QueueJob::jobDone, this, [this](QVariantMap latestResult) {
        emit importJobDone(latestResult.value("key").toULongLong());
    });

    qexp = new QueueJob(this, "qexp", tr("Export queue"), "", new TelemetryXmlExport());
    qexp->setIcon("export");
    qexp->setSection(sect);
    connect(qexp, &Fact::progressChanged, this, &TelemetryShare::updateProgress);

    f_stop = new Fact(this, "stop", tr("Stop"), tr("Stop conversion"), Action | Stop);
    connect(f_stop, &Fact::triggered, qimp, &QueueJob::stop);
    connect(f_stop, &Fact::triggered, qexp, &QueueJob::stop);

    connect(telemetry->f_lookup,
            &LookupTelemetry::recordIdChanged,
            this,
            &TelemetryShare::updateActions);

    descr_s = descr();
    updateProgress();
    updateStatus();
    updateActions();
}
//=============================================================================
QString TelemetryShare::defaultExportFileName() const
{
    const QVariantMap &info = telemetry->f_lookup->recordInfo();
    QString fname = QDateTime::fromMSecsSinceEpoch(telemetry->f_lookup->recordTimestamp())
                        .toString("yyyy_MM_dd_hh_mm_ss_zzz");
    QString callsign = info.value("callsign").toString();
    if (!callsign.isEmpty())
        fname.append("-").append(callsign);
    return fname;
}
ShareXmlExport *TelemetryShare::exportRequest(QString title, QString fileName)
{
    quint64 key = telemetry->f_lookup->recordId();
    if (!key) {
        apxMsgW() << tr("Missing data in database");
        return nullptr;
    }
    //add to queue
    Fact *f = new Fact(nullptr, title, title, fileName);
    f->setValue(key);
    f->setParentFact(qexp);
    return nullptr;
}
ShareXmlImport *TelemetryShare::importRequest(QString title, QString fileName)
{
    //add to queue
    new Fact(qimp, title, title, fileName);
    return nullptr;
}
//=============================================================================
//=============================================================================
void TelemetryShare::updateActions()
{
    quint64 id = telemetry->f_lookup->recordId();
    f_export->setEnabled(id);
}
void TelemetryShare::updateProgress()
{
    int v = -1;
    if (qexp->size() > 0)
        v = qexp->progress();
    else
        v = qimp->progress();
    setProgress(v);
    f_stop->setEnabled(v >= 0);
    updateDescr();
}
void TelemetryShare::updateStatus()
{
    setStatus(qimp->status());
}
void TelemetryShare::updateDescr()
{
    QString s;
    if (qimp->progress() >= 0)
        s = tr("Importing");
    else if (qexp->progress() >= 0)
        s = tr("Exporting");
    if (s.isEmpty())
        setDescr(descr_s);
    else
        setDescr(s.append(QString("... %1").arg(status())));
}
//=============================================================================
