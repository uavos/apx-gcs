#include "ReportGenerator.h"
#include "App/AppDirs.h"
#include "App/AppLog.h"
#include "rgTelemetry/TelemetryExtractor.h"
#include "rgTelemetry/TelemetryResolver.h"
#include "rgUtils/TemplateParser.h"
#include <Telemetry/LookupTelemetry.h>
#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryShare.h>
#include <Vehicles/Vehicles.h>
#include <QDebug>
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

    connect(Vehicles::instance()->f_replay->f_telemetry->f_reader,
            &TelemetryReader::dataAvailable,
            this,
            &ReportGenerator::telemetry_data_changed);
}

void ReportGenerator::generate_template_button_pressed()
{
    if (!m_template_loaded)
        return;

    request_commands();

    m_report_raw.clear();
    m_report_raw.reserve(m_template_raw.size());

    size_t start_iter = 0;
    for (size_t idx = 0; idx < m_command_rets.size(); idx++) {
        auto pos = m_command_positions[idx];

        m_report_raw.append(m_template_raw.mid(start_iter, pos.x() - start_iter));
        m_report_raw.append(m_command_rets[idx]);
        start_iter = pos.y() + 1;
    }

    m_report_raw.append(m_template_raw.mid(start_iter, m_template_raw.size() - start_iter));

    save_report();
}

void ReportGenerator::telemetry_data_changed(quint64 cache_id)
{
    if (!m_template_loaded)
        return;

    request_commands();
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

void ReportGenerator::request_commands()
{
    m_command_rets.clear();
    m_command_positions = TemplateParser::getCommandPositions(m_template_raw);

    for (auto com_pos = m_command_positions.begin(); com_pos != m_command_positions.end();
         com_pos++) {
        auto start = com_pos->x() + 1;
        auto end = com_pos->y();

        auto command = m_template_raw.mid(start, end - start).trimmed();

        m_command_rets.push_back(m_router.resolve_path(command).value_or("COMMAND NOT FOUND"));
    }
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
