#pragma once

#include <optional>
#include <QtCore>

class TelemetryExtractor;

class TelemetryResolver
{
public:
    TelemetryResolver(TelemetryExtractor &extractor);

    std::optional<QString> get_value(QString field_name);

private:
    void set_calc_function(QString field_name, std::function<QString()> callback);
    void clear();

    QMap<QString, std::function<QString()>> m_data;
    TelemetryExtractor &m_extractor;
};
