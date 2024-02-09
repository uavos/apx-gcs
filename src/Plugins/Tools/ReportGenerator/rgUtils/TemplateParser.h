#pragma once

#include <QtCore>

/**
 * @brief Used to get positions of commands in template file
 * 
 */
class TemplateParser
{
public:
    static QVector<QPoint> getCommandPositions(QString template_raw);

private:
};
