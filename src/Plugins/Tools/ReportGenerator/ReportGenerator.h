#pragma once

#include "./Fact/Fact.h"
#include "Router.h"
#include <QtWidgets>

namespace RG {
/**
 * @brief A class that reads a template, inserts values returned by resolvers, and saves the report
 * 
 */
class ReportGenerator : public Fact
{
    Q_OBJECT

public:
    explicit ReportGenerator(Fact *parent = nullptr);

private slots:
    void loadTemplateSlot();
    void generateReportSlot();

private:
    void loadTemplate();
    void saveReport();
    void generateReport();
    void updateGenerateButtonState();

    Fact *f_choose_template;
    Fact *f_generate_report;
    Fact *f_open_browser;

    QString m_template_path;
    QString m_template_raw;
    QString m_report_raw;
    Router m_router;

    bool m_template_loaded;
};
}; // namespace RG
