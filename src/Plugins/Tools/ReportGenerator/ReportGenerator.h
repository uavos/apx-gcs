#pragma once

#include "./Fact/Fact.h"
#include "Router.h"
#include <QtWidgets>

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
    void pick_template_button_pressed();
    void generate_template_button_pressed();

private:
    void load_template();
    void save_report();
    void generate_report();

    Fact *f_choose_template;
    Fact *f_generate_report;
    Fact *f_open_browser;

    QString m_template_path;
    QString m_template_raw;
    QString m_report_raw;
    Router m_router;

    bool m_template_loaded;
};
