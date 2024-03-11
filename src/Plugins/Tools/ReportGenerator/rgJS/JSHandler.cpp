#include "JSHandler.h"

#include "UserDefinedData.h"
#include <Telemetry/LookupTelemetry.h>
#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryReader.h>
#include <Telemetry/TelemetryShare.h>
#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>

RG::JSHandler::JSHandler(UserDefinedData *user_defined_data_fact)
    : m_engine{std::make_unique<QJSEngine>()}
    , m_registry{m_engine.get()}
    , m_user_defined{user_defined_data_fact}
{
    m_engine->installExtensions(QJSEngine::ConsoleExtension | QJSEngine::GarbageCollectionExtension);
}

void RG::JSHandler::syncMandala()
{
    auto mandala = Vehicles::instance()->f_replay->f_mandala;
    auto reader = Vehicles::instance()->f_replay->f_telemetry->f_reader;
    QJSValue mandala_js = m_engine->newObject();

    auto default_mandala_paths = mandala->valueFacts();

    for (const auto &mandala_fact : default_mandala_paths) {
        auto last = mandalaTraveler(mandala_js, mandala_fact->mpath());
        generateMandalaObject(last, nullptr, mandala_fact);
    }

    auto it = reader->fieldNames.constKeyValueBegin();
    auto end = reader->fieldNames.constKeyValueEnd();

    for (; it != end; ++it) {
        const auto &[fid, mandala_name] = *it;
        auto last = mandalaTraveler(mandala_js, mandala_name);
        auto *data = reader->fieldData.value(fid);
        MandalaFact *mandala_fact = nullptr;

        if (data) {
            mandala_fact = qobject_cast<MandalaFact *>(mandala->findChild(mandala_name));
        }

        generateMandalaObject(last, data, mandala_fact);
    }

    m_engine->globalObject().setProperty(k_global_raw_telemetry_object_name, mandala_js);
}

void RG::JSHandler::reloadHandler()
{
    m_user_defined->clearContext();

    if (!m_registry.isFilesChanged())
        return;

    resetEngine();
    m_registry.resetEnginePointer(m_engine.get());
    syncMandala();

    m_engine->globalObject().setProperty(k_global_raw_userdata_object_name,
                                         m_engine->newQObject(
                                             qobject_cast<QObject *>(m_user_defined)));

    m_registry.reloadModules();
}

void RG::JSHandler::clearRawTelemetry()
{
    m_engine->globalObject().setProperty(k_global_raw_telemetry_object_name, m_engine->newObject());
    m_engine->collectGarbage();
}

QString RG::JSHandler::evaluateCommand(QString command)
{
    auto evaluatedValue = m_engine->evaluate(command);

    if (evaluatedValue.hasProperty("is_calculated")) {
        return evaluatedValue.property("stringify").callWithInstance(evaluatedValue).toString();
    }

    return evaluatedValue.toString();
}

void RG::JSHandler::setScriptStage(ScriptStage stage)
{
    m_engine->globalObject().setProperty("ScriptStage",
                                         static_cast<std::underlying_type_t<ScriptStage>>(stage));
}

void RG::JSHandler::resetEngine()
{
    m_engine.reset();
    m_engine = std::make_unique<QJSEngine>();
    m_engine->installExtensions(QJSEngine::ConsoleExtension | QJSEngine::GarbageCollectionExtension);
}

QJSValue RG::JSHandler::mandalaTraveler(QJSValue start, QString path)
{
    auto mandala_path = path.split(".");
    auto last = start;
    for (const auto &it : mandala_path) {
        if (!last.hasProperty(it)) {
            last.setProperty(it, m_engine->newObject());
        }
        last = last.property(it);
    }
    return last;
}

void RG::JSHandler::generateMandalaObject(QJSValue target, QVector<QPointF> *data, MandalaFact *f)
{
    if (data == nullptr || f == nullptr || data->size() == 0) {
        target.setProperty("available", false);
        return;
    }

    auto meta = m_engine->newObject();

    auto js_tel_data = m_engine->newArray(data->size());

    quint32 ind = 0;
    for (const auto &it : *data) {
        auto moment = m_engine->newObject();
        moment.setProperty("value", it.y());
        moment.setProperty("time", it.x());
        js_tel_data.setProperty(ind, moment);
        ind++;
    }

    generateMeta(meta, f);

    target.setProperty("raw_data", js_tel_data);
    target.setProperty("available", true);
    target.setProperty("meta", meta);
}

void RG::JSHandler::generateMeta(QJSValue target, MandalaFact *f)
{
    if (f == nullptr)
        return;

    tryAddEnumToMandalaObject(target, f);
    target.setProperty("units", f->units());
    target.setProperty("uid", f->uid());
}

void RG::JSHandler::tryAddEnumToMandalaObject(QJSValue target, MandalaFact *f)
{
    auto &enum_strings = f->enumStrings();

    if (enum_strings.size() == 0)
        return;

    auto js_enum = m_engine->newObject();

    for (quint32 i = 0; i < enum_strings.size(); i++) {
        auto curr_string = enum_strings[i];
        js_enum.setProperty(i, curr_string);
        js_enum.setProperty(curr_string, i);
    }

    target.setProperty("enum", js_enum);
}
