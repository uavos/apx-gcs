#pragma once

#include <QtCore>

namespace RG {

class JSHandler;

struct Command
{
    QString command_string;
    QPoint command_pos;
};

class Report
{
    static constexpr auto k_search_pattern = "\\$\\{([^{}]*)\\}";

public:
    Report(JSHandler *handler);
    bool loadTemplateFromFile(QString path);
    bool saveReportToFile(QString path);
    void generateReport();
    bool isTemplateLoaded();

    void runPreheatingStage();

private:
    void parseTemplate();

    QString m_raw_template;
    QString m_raw_report;
    JSHandler *m_js_handler;
    QVector<Command> m_commands;
};

} // namespace RG
