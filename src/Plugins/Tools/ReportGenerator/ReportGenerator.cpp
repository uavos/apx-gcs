#include "ReportGenerator.h"
#include "App/AppDirs.h"
#include "App/AppLog.h"
#include <Telemetry/LookupTelemetry.h>
#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryReader.h>
#include <Telemetry/TelemetryShare.h>
#include <Vehicles/Vehicles.h>
#include <QDebug>
#include <QDesktopServices>
#include <QtCore>

namespace RG {
ReportGenerator::ReportGenerator(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Report generator"),
           tr("Report creation using templates"),
           Group,
           "file-document")
    , m_report(&m_js_handler)
    , f_open_browser{new Fact(this,
                              "enable",
                              tr("Open Browser"),
                              tr("Open generated file in Browser"),
                              Fact::Bool,
                              "web")}
    , f_choose_template{new Fact(this,
                                 "template-picker",
                                 tr("Select template"),
                                 tr(""),
                                 Action | Apply,
                                 "upload")}
    , f_generate_report{
          new Fact(this, "generate_report", tr("Generate report"), tr(""), Action | Apply, "export")}
{
    connect(f_choose_template, &Fact::triggered, this, &ReportGenerator::loadTemplateSlot);

    connect(f_generate_report, &Fact::triggered, this, &ReportGenerator::generateReportSlot);

    connect(Vehicles::instance()->f_replay->f_telemetry->f_reader,
            &TelemetryReader::dataAvailable,
            this,
            &ReportGenerator::telemetryChangedSlot);

    f_generate_report->setEnabled(false);
}

void ReportGenerator::generateReportSlot()
{
    if (!m_report.isTemplateLoaded())
        return;

    QString directory = AppDirs::user().absolutePath();
    QString filter = "Report HTML files (*.html)";
    auto filepath = QFileDialog::getSaveFileName(nullptr, "Save report", directory, filter);

    if (!filepath.endsWith(".html")) {
        filepath.append(".html");
    }

    m_js_handler.reloadHandler();
    m_report.generateReport();
    m_report.saveReportToFile(filepath);

    if (f_open_browser->value().toBool())
        QDesktopServices::openUrl(QUrl::fromLocalFile(filepath));
}

void ReportGenerator::telemetryChangedSlot()
{
    m_js_handler.syncMandala();
}

void ReportGenerator::updateGenerateButtonState()
{
    f_generate_report->setEnabled(m_report.isTemplateLoaded());
}

void ReportGenerator::loadTemplateSlot()
{
    QString directory = QDir::currentPath();
    QString filter = "Template HTML files (*.template.html)";

    auto m_template_path = QFileDialog::getOpenFileName(nullptr,
                                                        "Select Template HTML File",
                                                        directory,
                                                        filter);

    if (m_template_path.isEmpty()) {
        updateGenerateButtonState();
        return;
    }

    m_report.loadTemplateFromFile(m_template_path);
    updateGenerateButtonState();
}
}; // namespace RG
