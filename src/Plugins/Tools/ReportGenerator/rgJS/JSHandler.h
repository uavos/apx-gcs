#pragma once

#include "JSModulesLoader.h"
#include <QJSEngine>
#include <QtCore>

class MandalaFact;

namespace RG {

class UserDefinedData;

enum class ScriptStage { Preheating, ReportGeneration };

class JSHandler
{
    static constexpr auto k_global_raw_telemetry_object_name = "RawTelemetry";
    static constexpr auto k_global_raw_userdata_object_name = "UserData";

public:
    JSHandler(UserDefinedData *user_defined_data_fact);
    void syncMandala();
    void reloadHandler();
    void clearRawTelemetry();
    QString evaluateCommand(QString command);
    void setScriptStage(ScriptStage stage);

private:
    void resetEngine();
    QJSValue mandalaTraveler(QJSValue start, QString path);
    void generateMandalaObject(QJSValue target, QVector<QPointF> *data, MandalaFact *f);
    void generateMeta(QJSValue target, MandalaFact *f);
    void tryAddEnumToMandalaObject(QJSValue target, MandalaFact *f);
    std::unique_ptr<QJSEngine> m_engine;
    ModulesLoader m_registry;
    UserDefinedData *m_user_defined;
};

} // namespace RG
