#pragma once

#include <Mandala/Mandala.h>
#include <Telemetry/TelemetryReader.h>
#include <optional>
#include <QtCore>

class TelemetryExtractor : public QObject
{
    Q_OBJECT

public:
    static TelemetryExtractor &instance();
    std::optional<QVector<QPointF> *> by_id(quint64 uid);
    std::optional<QVector<QPointF> *> by_name(QString name);

private slots:
    void telemetry_data_changed();

private:
    TelemetryExtractor();
    void sync();
    void clear();

    QMap<QString, QVector<QPointF> *> m_name_data;
    QMap<quint64, QVector<QPointF> *> m_uid_data;
    TelemetryReader *m_reader;
};
