#include "ReportGenerator.h"
#include "App/AppDirs.h"
#include "App/AppLog.h"
#include "rgTelemetry/TelemetryExtractor.h"
#include "rgTelemetry/TelemetryResolver.h"
#include <Telemetry/LookupTelemetry.h>
#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryShare.h>
#include <Vehicles/Vehicles.h>
#include <QDebug>
#include <QDesktopServices>
#include <QtCore>

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

    connect(f_choose_template,
            &Fact::triggered,
            this,
            &ReportGenerator::pick_template_button_pressed);

    f_generate_report = new Fact(this,
                                 "generate_report",
                                 tr("Generate report"),
                                 tr(""),
                                 Action | Apply,
                                 "export");
    connect(f_generate_report,
            &Fact::triggered,
            this,
            &ReportGenerator::generate_template_button_pressed);

    f_generate_report->setEnabled(false);
}

void ReportGenerator::generate_template_button_pressed()
{
    if (!m_template_loaded)
        return;

    generate_report();
    save_report();
}

void ReportGenerator::load_template()
{
    m_template_loaded = false;
    m_template_raw = "";

    QFile file(m_template_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        apxMsg() << "Failed to open template file:" << file.errorString();
        return;
    }

    QTextStream in(&file);
    m_template_raw = in.readAll();

    file.close();

    m_template_loaded = true;
    f_generate_report->setEnabled(true);
}

void ReportGenerator::save_report()
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

void ReportGenerator::generate_report()
{
    QRegularExpression pattern("\\{([^{}]*)\\}");

    QRegularExpressionMatchIterator matches = pattern.globalMatch(m_template_raw);

    m_report_raw.clear();
    m_report_raw.reserve(m_template_raw.length());

    size_t start_iter = 0;
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        auto command = match.captured(1).trimmed();
        auto ret = m_router.resolve_path(command).value_or("COMMAND NOT FOUND");
        m_report_raw.append(m_template_raw.mid(start_iter, match.capturedStart(0) - start_iter));
        m_report_raw.append(ret);

        start_iter = match.capturedEnd(0);
    }

    m_report_raw.append(m_template_raw.mid(start_iter, m_template_raw.size() - start_iter));
}

void ReportGenerator::pick_template_button_pressed()
{
    QFileDialog dlg(nullptr, "Choose template", AppDirs::user().absolutePath());

    QStringList filters;
    filters << tr("All supported types") + " (*.html)";
    dlg.setNameFilters(filters);

    dlg.exec();

    if (!dlg.selectedFiles().size())
        return;

    m_template_path = dlg.selectedFiles().at(0);

    load_template();
}
