#include "Report.h"
#include "rgJS/JSHandler.h"

RG::Report::Report(JSHandler *handler)
    : m_js_handler{handler}
{}

bool RG::Report::loadTemplateFromFile(QString path)
{
    m_raw_template.clear();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    m_raw_template = in.readAll();

    file.close();

    runPreheatingStage();

    return true;
}

bool RG::Report::saveReportToFile(QString path)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        return false;
    }

    f.write(m_raw_report.toUtf8());
    f.close();
    return true;
}

void RG::Report::generateReport()
{
    m_js_handler->setScriptStage(ScriptStage::ReportGeneration);
    m_raw_report.clear();
    size_t cut_begin = 0;
    for (auto command : m_commands) {
        auto ret_value = m_js_handler->evaluateCommand(command.command_string);

        m_raw_report.append(m_raw_template.midRef(cut_begin, command.command_pos.x() - cut_begin));

        m_raw_report.append(ret_value);

        cut_begin = command.command_pos.y();
    }

    m_raw_report.append(m_raw_template.mid(cut_begin));
}

bool RG::Report::isTemplateLoaded()
{
    return !m_raw_template.isEmpty();
}

void RG::Report::runPreheatingStage()
{
    m_js_handler->reloadHandler();
    m_js_handler->setScriptStage(ScriptStage::Preheating);
    parseTemplate();
    for (auto it : m_commands) {
        m_js_handler->evaluateCommand(it.command_string);
    }
}

void RG::Report::parseTemplate()
{
    m_commands.clear();
    QRegExp regex(k_search_pattern);
    int pos = 0;
    while ((pos = regex.indexIn(m_raw_template, pos)) != -1) {
        QString matchedText = regex.cap(0);
        QString command = regex.cap(1);

        Command temp_command = {command, QPoint(pos, pos + matchedText.length())};
        pos += matchedText.length();

        m_commands.push_back(temp_command);
    };
}
