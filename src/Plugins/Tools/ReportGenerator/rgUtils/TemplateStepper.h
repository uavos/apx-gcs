#pragma once

#include <functional>
#include <QtCore>

class TemplateStepper
{
public:
    static QString replaceCommands(const QString &text,
                                   const QString &pattern,
                                   std::function<QString(QString)> resolver);
};
