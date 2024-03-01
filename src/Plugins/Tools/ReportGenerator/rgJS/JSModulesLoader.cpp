#include "JSModulesLoader.h"
#include "App/AppDirs.h"
#include "JSModulesLoader.h"
#include <App/AppLog.h>
#include <QJSValueIterator>

RG::ModulesLoader::ModulesLoader(QJSEngine *eng)
    : m_engine{eng}
{
    m_resources_scripts_dir.setPath(AppDirs::res().filePath("scripts/ReportGenerator/"));
    m_user_scripts_dir.setPath(AppDirs::scripts().filePath("ReportGenerator"));

    if (!m_user_scripts_dir.exists())
        m_user_scripts_dir.mkpath(".");
}

bool RG::ModulesLoader::isFilesChanged()
{
    bool is_resources_modified = recursivelyIsFilesChanged(m_resources_scripts_dir);
    bool is_user_scripts_modified = recursivelyIsFilesChanged(m_user_scripts_dir);

    return is_resources_modified || is_user_scripts_modified;
}

void RG::ModulesLoader::resetEnginePointer(QJSEngine *engine)
{
    m_engine = engine;
}

void RG::ModulesLoader::reloadModules()
{
    loadResourceBundle();
    recursivelyLoadModules(m_user_scripts_dir);
}

bool RG::ModulesLoader::recursivelyIsFilesChanged(QDir directory)
{
    bool is_modified = m_fileTimestamps.empty();

    recursivelyIterateModuleFiles(directory, [&is_modified, &directory, this](QString file_name) {
        QFileInfo file_info(directory, file_name);

        QDateTime currentTimestamp = file_info.lastModified();
        if (m_fileTimestamps.contains(file_name)) {
            if (m_fileTimestamps.value(file_name) != currentTimestamp) {
                is_modified = true;
            } else {
                return;
            }
        }

        m_fileTimestamps.insert(file_name, currentTimestamp);
    });

    return is_modified;
}

void RG::ModulesLoader::recursivelyLoadModules(QDir directory)
{
    recursivelyIterateModuleFiles(directory, [this, &directory](QString file_name) {
        QFileInfo file_info(directory, file_name);

        loadModule(file_info);
    });
}

void RG::ModulesLoader::recursivelyIterateModuleFiles(QDir directory,
                                                      std::function<void(QString)> callback)
{
    QStringList files = directory.entryList(QDir::Files);
    for (const QString &file : files) {
        if (!file.endsWith(".js", Qt::CaseInsensitive)) {
            continue;
        }

        callback(file);
    }

    QStringList subDirectories = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &subDirectory : subDirectories) {
        recursivelyIterateModuleFiles(subDirectory, callback);
    }
}

void RG::ModulesLoader::loadModule(QFileInfo file_info)
{
    auto module_object = m_engine->importModule(file_info.absoluteFilePath());

    if (module_object.isError()) {
        qWarning() << "Error occurred while loading module " << file_info.fileName() << " : "
                   << module_object.toString();
        return;
    }

    m_engine->globalObject().setProperty(file_info.baseName(), module_object);
}

void RG::ModulesLoader::loadResourceBundle()
{
    auto require_path = m_resources_scripts_dir.filePath("require.js");
    auto bundle_path = m_resources_scripts_dir.filePath("bundle.gen.js");
    auto main_path = m_resources_scripts_dir.filePath("main.js");

    evaluate_file(require_path);
    evaluate_file(bundle_path);
    evaluate_file(main_path);
}

void RG::ModulesLoader::evaluate_file(QString path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file";
        return;
    }

    QByteArray require_content = file.readAll();

    file.close();

    auto eval_result = m_engine->evaluate(require_content);

    if (eval_result.isError()) {
        apxMsg() << "Failed to load resource script : " << path;
        apxMsg() << eval_result.toString();
    }
}
