#pragma once

#include "./Fact/Fact.h"
#include "rgJS/JSHandler.h"
#include "rgUtils/Report.h"
#include <QJSEngine>
#include <QtWidgets>

namespace RG {
class UserDefinedData;

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
    void telemetryChangedSlot();

private:
    void updateGenerateButtonState();

    Fact *f_choose_template;
    Fact *f_generate_report;
    Fact *f_open_browser;

    UserDefinedData *m_user_defined_data;

    JSHandler m_js_handler;
    Report m_report;
};
}; // namespace RG
