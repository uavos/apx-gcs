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
#include "TelemetryShare.h"
#include "Telemetry.h"
#include "TelemetryReader.h"

#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppLog.h>

TelemetryShare::TelemetryShare(Telemetry *telemetry, Fact *parent, Flags flags)
    : Share(parent,
            telemetry::APXTLM_FTYPE,
            tr("Telemetry data"),
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
            flags)
    , _telemetry(telemetry)
{
    QSettings sx;
    if (sx.contains("DefaultExportPath")) {
        _defaultDir.setPath(
            sx.value(QString("ShareExportPath_%1").arg(_exportFormats.first())).toString());
    }

    _exportFormats << "csv";

    /*QString sect = tr("Queue");
    qimp = new QueueJob(this, "qimp", tr("Import queue"), "", new TelemetryImport());
    qimp->setIcon("import");
    qimp->setSection(sect);
    connect(qimp, &Fact::progressChanged, this, &TelemetryShare::updateProgress);
    connect(qimp, &Fact::valueChanged, this, &TelemetryShare::updateStatus);

    connect(qimp, &QueueJob::finished, this, [this](Fact *, QVariantMap result) {
        if (result.value("key").toULongLong()) {
            emit imported("", result.value("title").toString());
        }
    });
    connect(qimp, &QueueJob::jobDone, this, [this](QVariantMap latestResult) {
        emit importJobDone(latestResult.value("key").toULongLong());
    });

    qexp = new QueueJob(this, "qexp", tr("Export queue"), "", new TelemetryExport());
    qexp->setIcon("export");
    qexp->setSection(sect);
    connect(qexp, &Fact::progressChanged, this, &TelemetryShare::updateProgress);

    f_stop = new Fact(this, "stop", tr("Stop"), tr("Stop conversion"), Action | Stop);
    connect(f_stop, &Fact::triggered, qimp, &QueueJob::stop);
    connect(f_stop, &Fact::triggered, qexp, &QueueJob::stop);*/

    // connect(telemetry->f_records,
    //         &TelemetryRecords::recordIdChanged,
    //         this,
    //         &TelemetryShare::updateActions);

    f_export->setEnabled(false);
    connect(_telemetry->f_reader,
            &TelemetryReader::rec_finished,
            this,
            &TelemetryShare::updateActions);

    descr_s = descr();
    updateActions();
}

void TelemetryShare::updateActions()
{
    f_export->setEnabled(QFile::exists(_telemetry->f_reader->recordFilePath()));
}

QString TelemetryShare::getDefaultTitle()
{
    return QFileInfo(_telemetry->f_reader->recordFilePath()).completeBaseName();
}

bool TelemetryShare::exportRequest(QString format, QString fileName)
{
    auto fiSrc = QFileInfo(_telemetry->f_reader->recordFilePath());
    auto fiDest = QFileInfo(fileName);
    if (!fiSrc.exists()) {
        apxMsgW() << tr("Missing data source").append(':') << fiSrc.absoluteFilePath();
        return false;
    }

    do {
        if (format == telemetry::APXTLM_FTYPE) {
            if (fiDest.exists())
                QFile::remove(fiDest.absoluteFilePath());

            if (QFile::copy(fiSrc.absoluteFilePath(), fileName))
                break;

            apxMsgW() << tr("Failed to copy").append(':') << fiSrc.absoluteFilePath();
            return false;
        }

        if (format == "csv") {
        }

        apxMsgW() << tr("Unsupported format").append(':') << format;
        return false;
    } while (0);

    // export success
    apxMsg() << tr("Exported").append(':') << fiDest.fileName();
    return true;
}
bool TelemetryShare::importRequest(QString format, QString fileName)
{
    //add to queue
    auto title = QFileInfo(fileName).completeBaseName();
    //new Fact(qimp, title, title, fileName);
    return true;
}

void TelemetryShare::updateProgress()
{
    int v = -1;
    /*if (qexp->size() > 0)
        v = qexp->progress();
    else
        v = qimp->progress();*/
    setProgress(v);
    // f_stop->setEnabled(v >= 0);
    updateDescr();
}
void TelemetryShare::updateStatus()
{
    //setValue(qimp->value());
}
void TelemetryShare::updateDescr()
{
    QString s;
    /*if (qimp->progress() >= 0)
        s = tr("Importing");
    else if (qexp->progress() >= 0)
        s = tr("Exporting");*/
    if (s.isEmpty())
        setDescr(descr_s);
    else
        setDescr(s.append(QString("... %1").arg(value().toString())));
}

void TelemetryShare::syncTemplates()
{
    if (!App::bundle())
        return;

    // TODO parse xml file and cache hash in config
    /*for (auto fi : _templatesDir.entryInfoList()) {
        auto hash = fi.completeBaseName();
        auto req = new DBReqTelemetryRecover(hash);
        connect(req,
                &DBReqTelemetryRecover::unavailable,
                this,
                &TelemetryShare::syncTemplate,
                Qt::QueuedConnection);
        req->exec();
    }*/
}

void TelemetryShare::syncTemplate(QString hash)
{
    auto fname = _templatesDir.absoluteFilePath(
        QString("%1.%2").arg(hash).arg(_importFormats.first()));
    importRequest(_importFormats.first(), fname);
}
