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
    void telemetry_data_changed(quint64 cache_id);

private:
    void load_template();
    void request_commands();
    void save_report();

    Fact *f_choose_template;
    Fact *f_generate_report;

    QString m_template_path;
    QString m_template_raw;
    QString m_report_raw;
    Router m_router;
    QVector<QString> m_command_rets;
    QVector<QPoint> m_command_positions;

    bool m_template_loaded;
};
