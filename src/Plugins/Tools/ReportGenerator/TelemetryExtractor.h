#pragma once

#include <Mandala/Mandala.h>
#include <optional>
#include <QtCore>

class TelemetryExtractor
{
public:
    TelemetryExtractor();

    void sync();
    void clear();
    std::optional<QVector<QPointF> *> by_id(quint64 uid);
    std::optional<QVector<QPointF> *> by_name(QString name);

private:
    QMap<QString, QVector<QPointF> *> m_name_data;
    QMap<quint64, QVector<QPointF> *> m_uid_data;
};
