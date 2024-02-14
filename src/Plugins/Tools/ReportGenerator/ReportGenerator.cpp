#include "ReportGenerator.h"
#include "App/AppDirs.h"
#include "App/AppLog.h"
#include "rgTelemetry/TelemetryAccessor.h"
#include "rgTelemetry/TelemetryResolver.h"
#include "rgUtils/TemplateStepper.h"
#include <Telemetry/LookupTelemetry.h>
#include <Telemetry/Telemetry.h>
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
    , m_template_loaded(false)
{
    f_open_browser = new Fact(this,
                              "enable",
                              tr("Open Browser"),
                              tr("Open generated file in Browser"),
                              Fact::Bool,
                              "web");

    f_choose_template = new Fact(this,
                                 "template-picker",
                                 tr("Select template"),
                                 tr(""),
                                 Action | Apply,
                                 "upload");

    connect(f_choose_template, &Fact::triggered, this, &ReportGenerator::loadTemplateSlot);

    f_generate_report = new Fact(this,
                                 "generate_report",
                                 tr("Generate report"),
                                 tr(""),
                                 Action | Apply,
                                 "export");
    connect(f_generate_report, &Fact::triggered, this, &ReportGenerator::generateReportSlot);

    f_generate_report->setEnabled(false);
}

void ReportGenerator::generateReportSlot()
{
    if (!m_template_loaded)
        return;

    generateReport();
    saveReport();
}

void ReportGenerator::loadTemplate()
{
    m_template_loaded = false;
    m_template_raw.clear();

    QFile file(m_template_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        apxMsg() << "Failed to open template file:" << file.errorString();
        return;
    }

    QTextStream in(&file);
    m_template_raw = in.readAll();

    file.close();

    m_template_loaded = true;
    updateGenerateButtonState();
}

void ReportGenerator::saveReport()
{
    auto filename = QFileDialog::getSaveFileName();
    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly)) {
        apxMsg() << "Report cant be saved";
        return;
    }
    f.write(m_report_raw.toUtf8());
    f.close();

    apxMsg() << "Report successfully saved";

    if (f_open_browser->value().toBool())
        QDesktopServices::openUrl(QUrl::fromLocalFile(filename));
}

void ReportGenerator::generateReport()
{
    QString pattern{"\\$\\{([^{}]*)\\}"};
    m_report_raw = TemplateStepper::resolvePatternMatches(m_template_raw,
                                                          pattern,
                                                          [this](QString command) -> QString {
                                                              return m_router
                                                                  .resolvePath(command.trimmed())
                                                                  .value_or("COMMAND NOT FOUND");
                                                          });
}

void ReportGenerator::updateGenerateButtonState()
{
    f_generate_report->setEnabled(m_template_loaded);
}

void ReportGenerator::loadTemplateSlot()
{
    QString directory = QDir::currentPath();
    QString filter = "Template HTML files (*.template.html)";

    m_template_path = QFileDialog::getOpenFileName(nullptr,
                                                   "Select Template HTML File",
                                                   directory,
                                                   filter);

    if (m_template_path.isEmpty()) {
        m_template_loaded = false;
        updateGenerateButtonState();
        return;
    }

    loadTemplate();
}
}; // namespace RG
