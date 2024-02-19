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
    m_raw_report = m_raw_template;
    QRegExp regex(k_search_pattern);
    int pos = 0;
    while ((pos = regex.indexIn(m_raw_report, pos)) != -1) {
        QString matchedText = regex.cap(0);
        QString command = regex.cap(1);
        auto resolvedValue = m_js_handler->evaluateCommand(command);
        m_raw_report.replace(pos, matchedText.length(), resolvedValue);
        pos += resolvedValue.length();
    }
}

bool RG::Report::isTemplateLoaded()
{
    return !m_raw_template.isEmpty();
}
