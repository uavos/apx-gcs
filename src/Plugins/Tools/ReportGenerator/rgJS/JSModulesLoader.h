#pragma once

#include <QJSEngine>
#include <QtCore>

namespace RG {

class ModulesLoader
{
public:
    ModulesLoader(QJSEngine *eng);
    bool isFilesChanged();
    void resetEnginePointer(QJSEngine *engine);
    void reloadModules();

private:
    void recursivelyLoadModules(QDir directory);
    void recursivelyIterateModuleFiles(QDir directory, std::function<void(QString)> callback);
    void loadModule(QFileInfo file_info);
    void evaluate_file(QString path);

    void loadResourceBundle();

    bool recursivelyIsFilesChanged(QDir directory);

    QHash<QString, QDateTime> m_fileTimestamps;
    QDir m_resources_scripts_dir;
    QDir m_user_scripts_dir;
    QJSEngine *m_engine;
};

} // namespace RG
