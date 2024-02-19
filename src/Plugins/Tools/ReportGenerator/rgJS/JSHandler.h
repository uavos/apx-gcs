#pragma once

#include "JSModulesLoader.h"
#include <QJSEngine>
#include <QtCore>

class MandalaFact;

namespace RG {

class JSHandler
{
    static constexpr auto k_global_raw_telemetry_object_name = "RawTelemetry";

public:
    JSHandler();
    void syncMandala();
    void reloadHandler();
    void clearRawTelemetry();
    QString evaluateCommand(QString command);

private:
    void resetEngine();
    QJSValue mandalaTraveler(QJSValue start, QString path);
    void generateMandalaObject(QJSValue target, QVector<QPointF> *data, MandalaFact *f);
    void generateMeta(QJSValue target, MandalaFact *f);
    void tryAddEnumToMandalaObject(QJSValue target, MandalaFact *f);
    std::unique_ptr<QJSEngine> m_engine;
    ModulesLoader m_registry;
};

} // namespace RG
