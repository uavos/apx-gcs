#pragma once

#include <QtCore>

namespace RG {

class JSHandler;

class Report
{
    //Searches for ${...} pattern in template
    static constexpr auto k_search_pattern = "\\$\\{([^{}]*)\\}";

public:
    Report(JSHandler *handler);
    bool loadTemplateFromFile(QString path);
    bool saveReportToFile(QString path);
    void generateReport();
    bool isTemplateLoaded();

private:
    QString m_raw_template;
    QString m_raw_report;
    JSHandler *m_js_handler;
};

} // namespace RG
