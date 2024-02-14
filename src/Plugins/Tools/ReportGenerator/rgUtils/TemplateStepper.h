#pragma once

#include <functional>
#include <QtCore>

class TemplateStepper
{
public:
    static QString resolvePatternMatches(const QString &text,
                                         const QString &pattern,
                                         std::function<QString(QString)> valueResolver);
};
